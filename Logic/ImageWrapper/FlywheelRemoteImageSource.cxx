#include "FlywheelRemoteImageSource.h"
#include "AbstractSSHAuthDelegate.h"
#include "RemoteFileCache.h"
#include "RESTClient.h"
#include "IRISException.h"
#include "SystemInterface.h"
#include "UIReporterDelegates.h"
#include "json/json.h"
#include <itksys/SystemTools.hxx>
#include <sstream>
#include <map>
#include <mutex>
#include <vector>
#include <cstdio>

#ifdef _WIN32
#  include <windows.h>
#else
#  include <unistd.h>
#endif


// -----------------------------------------------------------------------
//  Internal helpers
// -----------------------------------------------------------------------
namespace {

std::mutex g_APIKeyMutex;
std::map<std::string, std::string> g_APIKeyCache;

// Cache for label→ID resolutions: key is list_url+"|"+label, value is _id.
// Persists for the app lifetime; eliminates repeated hierarchy traversals
// when loading a workspace where many images share the same session path.
std::mutex g_LabelIDMutex;
std::map<std::string, std::string> g_LabelIDCache;

std::string GetCachedAPIKey(const std::string &server)
{
  std::lock_guard<std::mutex> lock(g_APIKeyMutex);
  auto it = g_APIKeyCache.find(server);
  return (it != g_APIKeyCache.end()) ? it->second : "";
}

void SetCachedAPIKey(const std::string &server, const std::string &key)
{
  std::lock_guard<std::mutex> lock(g_APIKeyMutex);
  g_APIKeyCache[server] = key;
}

void ClearCachedAPIKey(const std::string &server)
{
  std::lock_guard<std::mutex> lock(g_APIKeyMutex);
  g_APIKeyCache.erase(server);
}

std::string MakeFWTempDir()
{
  std::string base = SystemInterface::GetSystemInfoDelegate()->GetTempDirectory();
  if (base.empty())
    throw IRISException("SystemInfoDelegate::GetTempDirectory() returned an empty path");

#ifdef _WIN32
  char name[MAX_PATH + 1] = "";
  if (!GetTempFileNameA(base.c_str(), "itksnap_fw", 0, name))
    throw IRISException("Cannot generate temp filename for Flywheel download in: %s", base.c_str());
  std::string dir = std::string(name) + "_d";
  DeleteFileA(name);
  if (!CreateDirectoryA(dir.c_str(), nullptr))
    throw IRISException("Cannot create temp directory: %s", dir.c_str());
  return dir;
#else
  std::string tmpl = base + "/itksnap_fw_XXXXXX";
  std::vector<char> buf(tmpl.begin(), tmpl.end());
  buf.push_back('\0');
  char *dir = mkdtemp(buf.data());
  if (!dir)
    throw IRISException("Cannot create temp directory under: %s", base.c_str());
  return dir;
#endif
}

// Flywheel API keys are displayed as "host:secret" in the UI and by the Python
// SDK, but only the secret portion is sent, prefixed with "Bearer ".
std::string MakeBearerHeader(const std::string &api_key)
{
  auto colon = api_key.find(':');
  std::string secret = (colon != std::string::npos)
    ? api_key.substr(colon + 1)
    : api_key;
  return "Bearer " + secret;
}

// Execute a GET against the Flywheel REST API with the given API key.
// Returns the response body; sets http_code.
std::string APIGet(const std::string &url, const std::string &api_key, long &http_code)
{
  RESTClient<> client;
  client.SetRequestHeader("Authorization", MakeBearerHeader(api_key).c_str());
  client.Get(url.c_str());
  http_code = client.GetHTTPCode();
  const char *out = client.GetOutput();
  return out ? std::string(out) : std::string();
}

// Walk a JSON array returned by the API and find the "_id" of the element
// whose "label" field matches @p label.  Throws IRISException if not found.
std::string FindIDByLabel(const std::string &json_str,
                          const std::string &label,
                          const std::string &item_type,
                          const std::string &parent_name)
{
  Json::Reader reader;
  Json::Value  root;
  if (!reader.parse(json_str, root, false))
    throw IRISException("Flywheel: failed to parse JSON response for %s list",
                        item_type.c_str());
  if (!root.isArray())
    throw IRISException("Flywheel: expected JSON array for %s list, got: %.80s",
                        item_type.c_str(), json_str.c_str());

  for (const auto &item : root)
    {
    if (item.get("label", "").asString() == label)
      {
      std::string id = item.get("_id", "").asString();
      if (id.empty())
        throw IRISException("Flywheel: %s '%s' has no _id field",
                            item_type.c_str(), label.c_str());
      return id;
      }
    }

  throw IRISException("Flywheel: %s '%s' not found in '%s'",
                      item_type.c_str(), label.c_str(), parent_name.c_str());
  return ""; // unreachable
}

// Authenticated GET that resolves one label→ID step in the hierarchy.
// Clears the cached API key and throws on 401.
std::string ResolveLevel(const std::string &list_url,
                         const std::string &api_key,
                         const std::string &label,
                         const std::string &item_type,
                         const std::string &parent_name,
                         const std::string &server)
{
  std::string cache_key = list_url + "|" + label;
  {
  std::lock_guard<std::mutex> lock(g_LabelIDMutex);
  auto it = g_LabelIDCache.find(cache_key);
  if (it != g_LabelIDCache.end())
    return it->second;
  }

  long code = 0;
  std::string body = APIGet(list_url, api_key, code);

  if (code == 401)
    {
    ClearCachedAPIKey(server);
    throw IRISException("Flywheel: invalid API key for %s (HTTP 401). "
                        "The key has been cleared — please try again.",
                        server.c_str());
    }
  if (code != 200)
    throw IRISException("Flywheel: HTTP %ld fetching %s list from %s",
                        code, item_type.c_str(), list_url.c_str());

  std::string id = FindIDByLabel(body, label, item_type, parent_name);

  {
  std::lock_guard<std::mutex> lock(g_LabelIDMutex);
  g_LabelIDCache[cache_key] = id;
  }
  return id;
}

// Ticket-based download from /api/{container_type}/{container_id}/files/{filename}.
// Works for both "acquisitions" and "analyses".
// @p cache_url  The original fw:// URL used as the cache key.
std::string DownloadDirect(const std::string         &server,
                           const std::string         &container_type,
                           const std::string         &container_id,
                           const std::string         &filename,
                           const std::string         &api_key,
                           const std::string         &cache_url,
                           RemoteFileCache           *file_cache,
                           DownloadProgressCallback  *progress_cb)
{
  std::string base         = "https://" + server;
  std::string download_url = base + "/api/" + container_type + "/"
                             + container_id + "/files/" + filename;

  // Cache lookup
  RemoteFileCache::HTTPCacheEntry cached;
  if (file_cache)
    cached = file_cache->LookupHTTP(cache_url);

  // Request a short-lived download ticket so the Authorization header is never
  // forwarded when Flywheel redirects to its pre-signed S3 URL.
  long ticket_code = 0;
  std::string ticket_body = APIGet(download_url + "?ticket=", api_key, ticket_code);

  if (ticket_code == 401)
    {
    ClearCachedAPIKey(server);
    throw IRISException("Flywheel: invalid API key for %s (HTTP 401). "
                        "The key has been cleared — please try again.",
                        server.c_str());
    }
  if (ticket_code != 200)
    throw IRISException("Flywheel: HTTP %ld requesting download ticket for %s",
                        ticket_code, download_url.c_str());

  Json::Reader reader;
  Json::Value  root;
  if (!reader.parse(ticket_body, root, false) || !root.isObject())
    throw IRISException("Flywheel: unexpected response requesting download ticket for %s",
                        download_url.c_str());
  std::string ticket = root.get("ticket", "").asString();
  if (ticket.empty())
    throw IRISException("Flywheel: download ticket response contained no ticket field");

  std::string ticketed_url = download_url + "?ticket=" + ticket;

  // Stream to a local temp file, following the S3 redirect transparently.
  std::string tmpdir = MakeFWTempDir();
  std::string dest   = tmpdir + "/" + filename;

  FILE *outfile = fopen(dest.c_str(), "wb");
  if (!outfile)
    throw IRISException("Flywheel: cannot create local file '%s'", dest.c_str());

  RESTClient<> dl_client;
  dl_client.SetFollowRedirects(true);
  dl_client.SetOutputFile(outfile);

  if (!cached.local_path.empty())
    {
    if (!cached.etag.empty())
      dl_client.SetRequestHeader("If-None-Match", cached.etag.c_str());
    if (!cached.last_modified.empty())
      dl_client.SetRequestHeader("If-Modified-Since", cached.last_modified.c_str());
    }

  if (progress_cb && *progress_cb)
    dl_client.SetProgressCallback(
      progress_cb,
      [](void *data, size_t done, size_t total)
      {
      auto *cb = static_cast<DownloadProgressCallback *>(data);
      (*cb)(done, total);
      });

  dl_client.Get(ticketed_url.c_str());
  fclose(outfile);

  long code = dl_client.GetHTTPCode();

  if (code == 304)
    {
    itksys::SystemTools::RemoveADirectory(tmpdir);
    return cached.local_path;
    }
  if (code != 200)
    throw IRISException("Flywheel: HTTP %ld downloading %s", code, ticketed_url.c_str());

  if (file_cache)
    return file_cache->StoreHTTP(cache_url, dest,
                                 dl_client.GetResponseHeader("etag"),
                                 dl_client.GetResponseHeader("last-modified"));
  return dest;
}

// Label-based discovery: resolve group/project/subject/session/container by
// label, then delegate to DownloadDirect.
//
// parts layout (9 elements):
//   [0]="find"  [1]=group  [2]=project  [3]=subject  [4]=session
//   [5]="acquisitions"|"analyses"  [6]=label  [7]="files"  [8]=filename
std::string DownloadByFind(const std::string               &server,
                           const std::vector<std::string>  &parts,
                           const std::string               &api_key,
                           const std::string               &cache_url,
                           RemoteFileCache                 *file_cache,
                           DownloadProgressCallback        *progress_cb)
{
  std::string base             = "https://" + server;
  const std::string &group     = parts[1];
  const std::string &project   = parts[2];
  const std::string &subject   = parts[3];
  const std::string &session   = parts[4];
  const std::string &ctype     = parts[5]; // "acquisitions" or "analyses"
  const std::string &label     = parts[6];
  const std::string &filename  = parts[8];

  // Singular form for error messages ("acquisition", "analysis")
  std::string ctype_singular = (ctype == "analyses") ? "analysis" : "acquisition";

  std::string project_id =
    ResolveLevel(base + "/api/groups/" + group + "/projects",
                 api_key, project, "project", group, server);

  std::string subject_id =
    ResolveLevel(base + "/api/projects/" + project_id + "/subjects",
                 api_key, subject, "subject", project, server);

  std::string session_id =
    ResolveLevel(base + "/api/subjects/" + subject_id + "/sessions",
                 api_key, session, "session", subject, server);

  std::string container_id =
    ResolveLevel(base + "/api/sessions/" + session_id + "/" + ctype,
                 api_key, label, ctype_singular, session, server);

  return DownloadDirect(server, ctype, container_id, filename,
                        api_key, cache_url, file_cache, progress_cb);
}

} // anonymous namespace


// -----------------------------------------------------------------------
//  FlywheelRemoteImageSource::Download
// -----------------------------------------------------------------------

std::string
FlywheelRemoteImageSource::Download(const std::string &url)
{
  // ---- 1. Parse: strip fw:// and split into server + path segments ----
  if (url.substr(0, 5) != "fw://")
    throw IRISException("FlywheelRemoteImageSource: URL must start with fw://: %s",
                        url.c_str());

  std::string rest = url.substr(5);
  auto slash = rest.find('/');
  if (slash == std::string::npos)
    throw IRISException("Flywheel: malformed URL (missing path after host): %s", url.c_str());

  std::string server = rest.substr(0, slash);
  std::string path   = rest.substr(slash + 1);

  std::vector<std::string> parts;
  {
  std::istringstream ss(path);
  std::string seg;
  while (std::getline(ss, seg, '/'))
    if (!seg.empty())
      parts.push_back(seg);
  }

  // ---- 2. Get or prompt for API key ----
  std::string api_key = GetCachedAPIKey(server);
  if (api_key.empty())
    {
    if (!m_AuthDelegate ||
        !m_AuthDelegate->PromptForAPIKey(server, "", api_key))
      throw IRISUserCancelException("Flywheel authentication cancelled");
    SetCachedAPIKey(server, api_key);
    }

  // ---- 3. Dispatch on URL form ----
  //
  //  Direct:  fw://server/acquisitions/<id>/files/<file>   (parts.size()==4)
  //           fw://server/analyses/<id>/files/<file>        (parts.size()==4)
  //
  //  Find:    fw://server/find/<group>/<project>/<subject>/<session>/
  //                       acquisitions|analyses/<label>/files/<file>
  //                                                         (parts.size()==9)

  bool is_direct = parts.size() == 4
                   && parts[2] == "files"
                   && (parts[0] == "acquisitions" || parts[0] == "analyses");

  bool is_find   = parts.size() == 9
                   && parts[0] == "find"
                   && (parts[5] == "acquisitions" || parts[5] == "analyses")
                   && parts[7] == "files";

  if (is_direct)
    return DownloadDirect(server, parts[0], parts[1], parts[3],
                          api_key, url, m_FileCache, &m_ProgressCallback);

  if (is_find)
    return DownloadByFind(server, parts, api_key, url,
                          m_FileCache, &m_ProgressCallback);

  throw IRISException(
    "Flywheel: unrecognised URL format: %s\n"
    "Valid forms:\n"
    "  fw://server/acquisitions/<id>/files/<file>\n"
    "  fw://server/analyses/<id>/files/<file>\n"
    "  fw://server/find/<group>/<project>/<subject>/<session>/acquisitions/<label>/files/<file>\n"
    "  fw://server/find/<group>/<project>/<subject>/<session>/analyses/<label>/files/<file>",
    url.c_str());
}
