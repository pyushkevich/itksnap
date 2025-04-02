#include "RESTClient.h"
#include <sstream>
#include <fstream>
#include <cstdarg>
#include "IRISException.h"
#include "itksys/SystemTools.hxx"
#include "itksys/MD5.h"
#include "FormattedTable.h"

using itksys::SystemTools;

#ifdef WIN32
#  define CURL_STATICLIB
#endif
#include <curl/curl.h>

namespace RESTClient_internal
{

template <typename ServerTraits>
int
progress_callback(void *clientp, double dltotal, double dlnow, double ultotal, double ulnow)
{
  long bytes_total = dltotal + ultotal;
  long bytes_done = dlnow + ulnow;
  // std::cout << "progress callback " << bytes_done << "," << bytes_total << std::endl;

  // Sometimes this is called with zeros
  if (bytes_total == 0)
    return 0;

  typedef std::pair<void *, typename RESTClient<ServerTraits>::ProgressCallbackFunction> CallbackInfo;
  CallbackInfo *cbi = static_cast<CallbackInfo *>(clientp);
  cbi->second(cbi->first, bytes_done * 1.0 / bytes_total);
  return 0;
}

} // namespace RESTClient_internal

using namespace std;

template <typename ServerTraits>
RESTClient<ServerTraits>::RESTClient()
{
  // Initialize CURL
  m_Curl = curl_easy_init();

  // Sharing business
  m_Share = curl_share_init();
  curl_share_setopt((CURLSH *)m_Share, CURLSHOPT_SHARE, CURL_LOCK_DATA_SSL_SESSION);
  curl_easy_setopt(m_Curl, CURLOPT_SHARE, m_Share);

  // Error buffer
  m_ErrorBuffer = new char[CURL_ERROR_SIZE];
  m_ErrorBuffer[0] = 0;
  curl_easy_setopt(m_Curl, CURLOPT_ERRORBUFFER, m_ErrorBuffer);

  m_UploadMessageBuffer[0] = 0;
  m_MessageBuffer[0] = 0;
  m_OutputFile = NULL;
  m_ReceiveCookieMode = false;

  m_CallbackInfo.first = NULL;
  m_CallbackInfo.second = NULL;
}

template <typename ServerTraits>
RESTClient<ServerTraits>::~RESTClient()
{
  curl_easy_cleanup(m_Curl);
  curl_share_cleanup((CURLSH *)m_Share);
  delete m_ErrorBuffer;
}

template <typename ServerTraits>
void
RESTClient<ServerTraits>::SetVerbose(bool verbose)
{
  curl_easy_setopt(m_Curl, CURLOPT_VERBOSE, (long)verbose);
}

template <typename ServerTraits>
void
RESTClient<ServerTraits>::SetOutputFile(FILE *outfile)
{
  m_OutputFile = outfile;
}

template <typename ServerTraits>
bool
RESTClient<ServerTraits>::Authenticate(const char *baseurl, const char *token)
{
  // Create and perform the request
  ostringstream o_url;
  o_url << baseurl << "/api/login";
  curl_easy_setopt(m_Curl, CURLOPT_URL, o_url.str().c_str());

  // Store the URL for the future
  ofstream f_url(this->GetServerURLFile().c_str());
  f_url << baseurl;
  f_url.close();

  // Data to post
  char post_buffer[1024];
  snprintf(post_buffer, sizeof(post_buffer), "token=%s", token);
  curl_easy_setopt(m_Curl, CURLOPT_POSTFIELDS, post_buffer);

  // Cookie file
  string cookie_jar = this->GetCookieFile();
  curl_easy_setopt(m_Curl, CURLOPT_COOKIEJAR, cookie_jar.c_str());

  // Capture output
  m_Output.clear();
  curl_easy_setopt(m_Curl, CURLOPT_WRITEFUNCTION, RESTClient::WriteCallback);
  curl_easy_setopt(m_Curl, CURLOPT_WRITEDATA, &m_Output);

  // Make request
  CURLcode res = curl_easy_perform(m_Curl);

  if (res != CURLE_OK)
    throw IRISException("CURL library error: %s\n%s", curl_easy_strerror(res), m_ErrorBuffer);

  // Return success or failure
  string success_pattern = "logged in as ";
  return m_Output.compare(0, success_pattern.length(), success_pattern) == 0;
}

template <typename ServerTraits>
void
RESTClient<ServerTraits>::SetServerURL(const char *baseurl)
{
  // Store the URL for the future
  ofstream f_url(GetServerURLFile().c_str());
  f_url << baseurl;
  f_url.close();
}

template <typename ServerTraits>
void
RESTClient<ServerTraits>::SetReceiveCookieMode(bool mode)
{
  m_ReceiveCookieMode = true;
}

template <typename ServerTraits>
bool
RESTClient<ServerTraits>::Get(const char *rel_url, ...)
{
  bool         rc;
  std::va_list args;
  va_start(args, rel_url);

  try
  {
    rc = this->GetVA(rel_url, args);
  }
  catch (IRISException &exc)
  {
    va_end(args);
    throw;
  }

  va_end(args);
  return rc;
}

template <typename ServerTraits>
bool
RESTClient<ServerTraits>::GetVA(const char *rel_url, std::va_list args)
{
  // Expand the URL
  char url_buffer[4096];
  vsnprintf(url_buffer, 4096, rel_url, args);

  // Use the same code as POST, but with null string
  return this->Post(url_buffer, NULL);
}

template <typename ServerTraits>
bool
RESTClient<ServerTraits>::Post(const char *rel_url, const char *post_string, ...)
{
  bool         rc;
  std::va_list args;
  va_start(args, post_string);

  try
  {
    rc = this->PostVA(rel_url, post_string, args);
  }
  catch (IRISException &exc)
  {
    va_end(args);
    throw;
  }

  va_end(args);
  return rc;
}

template <typename ServerTraits>
bool
RESTClient<ServerTraits>::PostVA(const char *rel_url, const char *post_string, std::va_list args)
{
  // Calling vsnprintf multiple times with the same args fails on Windows. Instead we
  // can concatenate the URL string and the post string, call vasnprintf once, and then
  // split the buffers. We just need a separator that would not be expected.
  const char *sep = "@@@SEP@@@";
  std::string joint_pattern = post_string
                                ? std::string(rel_url) + std::string(sep) + std::string(post_string)
                                : std::string(rel_url);

  // Expand the URL
  char url_buffer[4096];
  vsnprintf(url_buffer, 4096, joint_pattern.c_str(), args);

  // Split into url and post sections
  std::string url_filled, post_filled;
  if (post_string)
  {
    std::string ub(url_buffer);
    size_t      pos = ub.find(sep);
    url_filled = ub.substr(0, pos);
    post_filled = ub.substr(pos + strlen(sep));
  }
  else
    url_filled = std::string(url_buffer);

  // The URL to post to
  string url = this->GetServerURL() + "/" + url_filled;
  curl_easy_setopt(m_Curl, CURLOPT_URL, url.c_str());

  // The cookie JAR
  string cookie_jar = this->GetCookieFile();
  if (m_ReceiveCookieMode)
    curl_easy_setopt(m_Curl, CURLOPT_COOKIEJAR, cookie_jar.c_str());
  else
    curl_easy_setopt(m_Curl, CURLOPT_COOKIEFILE, cookie_jar.c_str());

  // The POST data
  if (post_string)
    curl_easy_setopt(m_Curl, CURLOPT_POSTFIELDS, post_filled.c_str());

  // Capture output
  m_Output.clear();

  // If there is no output file, use the default callbacl
  if (!m_OutputFile)
  {
    curl_easy_setopt(m_Curl, CURLOPT_WRITEFUNCTION, RESTClient::WriteCallback);
    curl_easy_setopt(m_Curl, CURLOPT_WRITEDATA, &m_Output);
  }
  else
  {
    curl_easy_setopt(m_Curl, CURLOPT_WRITEFUNCTION, RESTClient::WriteToFileCallback);
    curl_easy_setopt(m_Curl, CURLOPT_WRITEDATA, m_OutputFile);

    // Set the callback functions
    if (m_CallbackInfo.first)
    {
      curl_easy_setopt(m_Curl, CURLOPT_PROGRESSFUNCTION, RESTClient_internal::progress_callback<ServerTraits>);
      curl_easy_setopt(m_Curl, CURLOPT_PROGRESSDATA, &m_CallbackInfo);
      curl_easy_setopt(m_Curl, CURLOPT_NOPROGRESS, 0);
      curl_easy_setopt(m_Curl, CURLOPT_VERBOSE, 1L);
    }
  }

  // Make request
  CURLcode res = curl_easy_perform(m_Curl);

  if (res != CURLE_OK)
    throw IRISException("CURL library error: %s\n%s", curl_easy_strerror(res), m_ErrorBuffer);

  // Capture the response code
  m_HTTPCode = 0L;
  curl_easy_getinfo(m_Curl, CURLINFO_RESPONSE_CODE, &m_HTTPCode);

  // Get the code
  return m_HTTPCode == 200L;
}

template <typename ServerTraits>
void
RESTClient<ServerTraits>::SetProgressCallback(void *cb_data, ProgressCallbackFunction fn)
{
  m_CallbackInfo = make_pair(cb_data, fn);
}

template <class ServerTraits>
bool
RESTClient<ServerTraits>::PostMultipart(const char *rel_url, RESTMultipartData *data, ...)
{
  // Expand the URL
  std::va_list args;
  va_start(args, data);
  char url_buffer[4096];
  vsnprintf(url_buffer, 4096, rel_url, args);
  va_end(args);

  // The URL to post to
  string url = this->GetServerURL() + "/" + url_buffer;
  curl_easy_setopt(m_Curl, CURLOPT_URL, url.c_str());

  // The cookie JAR
  string cookie_jar = this->GetCookieFile();
  curl_easy_setopt(m_Curl, CURLOPT_COOKIEFILE, cookie_jar.c_str());

  // Use the new mime interface
  curl_mime *mime = curl_mime_init(m_Curl);

  // Add the file
  for(auto &mp : data->m_Parts)
  {
    auto *part = curl_mime_addpart(mime);
    curl_mime_name(part, mp.first.c_str());
    curl_mime_type(part, mp.second.mime_type.c_str());
    if(mp.second.filename.size())
      curl_mime_filename(part, mp.second.filename.c_str());
    if(mp.second.byte_array_size > 0)
    {
      curl_mime_data(part, mp.second.byte_array, mp.second.byte_array_size);
    }
    else
    {
      curl_mime_data(part, mp.second.value.c_str(), CURL_ZERO_TERMINATED);
    }
  }

  // Set the mime request
  curl_easy_setopt(m_Curl, CURLOPT_MIMEPOST, mime);

  // Capture output
  m_Output.clear();
  curl_easy_setopt(m_Curl, CURLOPT_WRITEFUNCTION, RESTClient::WriteCallback);
  curl_easy_setopt(m_Curl, CURLOPT_WRITEDATA, &m_Output);

  // Set the callback functions
  if (m_CallbackInfo.first)
  {
    curl_easy_setopt(m_Curl, CURLOPT_PROGRESSFUNCTION, RESTClient_internal::progress_callback<ServerTraits>);
    curl_easy_setopt(m_Curl, CURLOPT_PROGRESSDATA, &m_CallbackInfo);
    curl_easy_setopt(m_Curl, CURLOPT_NOPROGRESS, 0);
  }

  // Make request
  CURLcode res = curl_easy_perform(m_Curl);

  if (res != CURLE_OK)
    throw IRISException("CURL library error: %s\n%s", curl_easy_strerror(res), m_ErrorBuffer);

  // Get the upload statistics
  double upload_size, upload_time;
  curl_easy_getinfo(m_Curl, CURLINFO_SIZE_UPLOAD, &upload_size);
  curl_easy_getinfo(m_Curl, CURLINFO_TOTAL_TIME, &upload_time);
  snprintf(m_UploadMessageBuffer,
           sizeof(m_UploadMessageBuffer),
           "%.1f Mb in %.1f s",
           upload_size / 1.0e6,
           upload_time);

  /* then cleanup the formpost chain */
  curl_mime_free(mime);

  // Capture the response code
  m_HTTPCode = 0L;
  curl_easy_getinfo(m_Curl, CURLINFO_RESPONSE_CODE, &m_HTTPCode);

  // Get the code
  return m_HTTPCode == 200L;
}

template <typename ServerTraits>
bool
RESTClient<ServerTraits>::UploadFile(const char              *rel_url,
                                     const char              *filename,
                                     std::map<string, string> extra_fields,
                                     ...)
{
  // Expand the URL
  std::va_list args;
  va_start(args, extra_fields);
  char url_buffer[4096];
  vsnprintf(url_buffer, 4096, rel_url, args);

  // The URL to post to
  string url = this->GetServerURL() + "/" + url_buffer;
  curl_easy_setopt(m_Curl, CURLOPT_URL, url.c_str());

  // The cookie JAR
  string cookie_jar = this->GetCookieFile();
  curl_easy_setopt(m_Curl, CURLOPT_COOKIEFILE, cookie_jar.c_str());

  // Get the full path and just the name from the filename
  string fn_full_path = SystemTools::CollapseFullPath(filename);
  string fn_name = SystemTools::GetFilenameName(filename);

  // Use the multi-part (-F) commands
  struct curl_httppost *formpost = NULL;
  struct curl_httppost *lastptr = NULL;
  struct curl_slist    *headerlist = NULL;
  static const char     buf[] = "Expect:";

  /* Fill in the file upload field */
  curl_formadd(
    &formpost, &lastptr, CURLFORM_COPYNAME, "myfile", CURLFORM_FILE, fn_full_path.c_str(), CURLFORM_END);

  /* Fill in the filename field */
  curl_formadd(
    &formpost, &lastptr, CURLFORM_COPYNAME, "filename", CURLFORM_COPYCONTENTS, fn_name.c_str(), CURLFORM_END);

  /* Fill in the submit field too, even if this is rarely needed */
  curl_formadd(
    &formpost, &lastptr, CURLFORM_COPYNAME, "submit", CURLFORM_COPYCONTENTS, "send", CURLFORM_END);

  /* Add the extra form fields */
  for (std::map<string, string>::const_iterator it = extra_fields.begin(); it != extra_fields.end();
       ++it)
  {
    char post_buffer[4096];
    vsnprintf(post_buffer, 4096, it->second.c_str(), args);

    curl_formadd(&formpost,
                 &lastptr,
                 CURLFORM_COPYNAME,
                 it->first.c_str(),
                 CURLFORM_COPYCONTENTS,
                 post_buffer,
                 CURLFORM_END);
  }

  // Done expanding args
  va_end(args);

  /* initialize custom header list (stating that Expect: 100-continue is not wanted */
  headerlist = curl_slist_append(headerlist, buf);

  curl_easy_setopt(m_Curl, CURLOPT_HTTPHEADER, headerlist);
  curl_easy_setopt(m_Curl, CURLOPT_HTTPPOST, formpost);

  // Capture output
  m_Output.clear();
  curl_easy_setopt(m_Curl, CURLOPT_WRITEFUNCTION, RESTClient::WriteCallback);
  curl_easy_setopt(m_Curl, CURLOPT_WRITEDATA, &m_Output);

  // Set the callback functions
  if (m_CallbackInfo.first)
  {
    curl_easy_setopt(m_Curl, CURLOPT_PROGRESSFUNCTION, RESTClient_internal::progress_callback<ServerTraits>);
    curl_easy_setopt(m_Curl, CURLOPT_PROGRESSDATA, &m_CallbackInfo);
    curl_easy_setopt(m_Curl, CURLOPT_NOPROGRESS, 0);
  }

  // Make request
  CURLcode res = curl_easy_perform(m_Curl);

  if (res != CURLE_OK)
    throw IRISException("CURL library error: %s\n%s", curl_easy_strerror(res), m_ErrorBuffer);

  // Get the upload statistics
  double upload_size, upload_time;
  curl_easy_getinfo(m_Curl, CURLINFO_SIZE_UPLOAD, &upload_size);
  curl_easy_getinfo(m_Curl, CURLINFO_TOTAL_TIME, &upload_time);
  snprintf(m_UploadMessageBuffer,
           sizeof(m_UploadMessageBuffer),
           "%.1f Mb in %.1f s",
           upload_size / 1.0e6,
           upload_time);

  /* then cleanup the formpost chain */
  curl_formfree(formpost);

  /* free slist */
  curl_slist_free_all(headerlist);

  // Capture the response code
  m_HTTPCode = 0L;
  curl_easy_getinfo(m_Curl, CURLINFO_RESPONSE_CODE, &m_HTTPCode);

  // Get the code
  return m_HTTPCode == 200L;
}

template <typename ServerTraits>
const char *
RESTClient<ServerTraits>::GetOutput()
{
  return m_Output.c_str();
}

template <class ServerTraits>
const char *
RESTClient<ServerTraits>::GetErrorString() const
{
  if(strlen(m_ErrorBuffer))
    return m_ErrorBuffer;
  else if(m_HTTPCode != 200L && m_Output.size())
    return m_Output.c_str();
  else return "";
}

template <class ServerTraits>
long
RESTClient<ServerTraits>::GetHTTPCode() const
{
  return m_HTTPCode;
}

template <typename ServerTraits>
std::string
RESTClient<ServerTraits>::GetFormattedCSVOutput(bool header)
{
  FormattedTable ft;
  ft.ParseCSV(m_Output);
  ostringstream oss;
  ft.Print(oss);
  return oss.str();
}

template <typename ServerTraits>
const char *
RESTClient<ServerTraits>::GetResponseText()
{
  snprintf(
    m_MessageBuffer, sizeof(m_MessageBuffer), "Response %ld, Text: %s", m_HTTPCode, m_Output.c_str());
  return m_MessageBuffer;
}

template <typename ServerTraits>
const char *
RESTClient<ServerTraits>::GetUploadStatistics()
{
  return m_UploadMessageBuffer;
}

template <typename ServerTraits>
string
RESTClient<ServerTraits>::GetDataDirectory()
{
  // Compute the platform-independent home directory
  vector<string> split_path;
  SystemTools::SplitPath(ServerTraits::DirectoryPrefix, split_path, true);
  string ddir = SystemTools::JoinPath(split_path);
  SystemTools::MakeDirectory(ddir.c_str());
  return ddir;
}

template <typename ServerTraits>
string
RESTClient<ServerTraits>::GetCookieFile()
{
  // MD5 encode the server
  string server = RESTClient::GetServerURL();

  char hex_code[33];
  hex_code[32] = 0;
  itksysMD5 *md5 = itksysMD5_New();
  itksysMD5_Initialize(md5);
  itksysMD5_Append(md5, (unsigned char *)server.c_str(), server.size());
  itksysMD5_FinalizeHex(md5, hex_code);
  itksysMD5_Delete(md5);

  string cfile = RESTClient::GetDataDirectory() + "/cookie_" + hex_code + ".jar";
  return SystemTools::ConvertToOutputPath(cfile);
}

template <typename ServerTraits>
string
RESTClient<ServerTraits>::GetServerURLFile()
{
  string sfile = RESTClient::GetDataDirectory() + "/server";
  return SystemTools::ConvertToOutputPath(sfile);
}

template <typename ServerTraits>
string
RESTClient<ServerTraits>::GetServerURL()
{
  // If environment variable is set, then use it
  const char *server_env = SystemTools::GetEnv("ITKSNAP_WT_DSS_SERVER");
  if (server_env)
    return server_env;

  string sfile = RESTClient::GetServerURLFile();
  if (!SystemTools::FileExists(sfile))
    throw IRISException("A server has not been configured yet - please sign in");
  try
  {
    string   url;
    ifstream ifs(sfile.c_str());
    ifs >> url;
    ifs.close();
    return url;
  }
  catch (std::exception &exc)
  {
    throw IRISException(
      "Failed to read server URL from %s, system exception: %s", sfile.c_str(), exc.what());
  }
}

template <typename ServerTraits>
size_t
RESTClient<ServerTraits>::WriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
  string *buffer = static_cast<string *>(userp);
  buffer->append((char *)contents, size * nmemb);
  return size * nmemb;
}

template <typename ServerTraits>
size_t
RESTClient<ServerTraits>::WriteToFileCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
  FILE *file = static_cast<FILE *>(userp);
  return fwrite(contents, size, nmemb, file);
}


static void
REST_DebugDump(const char *text, FILE *stream, unsigned char *ptr, size_t size, char nohex)
{
  size_t i;
  size_t c;

  unsigned int width = 0x10;

  if (nohex)
    /* without the hex output, we can fit more on screen */
    width = 0x40;

  fprintf(stream, "%s, %10.10ld bytes (0x%8.8lx)\n", text, (long)size, (long)size);

  for (i = 0; i < size; i += width)
  {

    fprintf(stream, "%4.4lx: ", (long)i);

    if (!nohex)
    {
      /* hex not disabled, show it */
      for (c = 0; c < width; c++)
        if (i + c < size)
          fprintf(stream, "%02x ", ptr[i + c]);
        else
          fputs("   ", stream);
    }

    for (c = 0; (c < width) && (i + c < size); c++)
    {
      /* check for 0D0A; if found, skip past and start a new line of output */
      if (nohex && (i + c + 1 < size) && ptr[i + c] == 0x0D && ptr[i + c + 1] == 0x0A)
      {
        i += (c + 2 - width);
        break;
      }
      fprintf(stream, "%c", (ptr[i + c] >= 0x20) && (ptr[i + c] < 0x80) ? ptr[i + c] : '.');
      /* check again for 0D0A, to avoid an extra \n if it's at width */
      if (nohex && (i + c + 2 < size) && ptr[i + c + 1] == 0x0D && ptr[i + c + 2] == 0x0A)
      {
        i += (c + 3 - width);
        break;
      }
    }
    fputc('\n', stream); /* newline */
  }
  fflush(stream);
}


/**
 * Use this fuction to trace CURL communications:
 *
 *   // Trace on
 *   FILE *trace_file = fopen("/tmp/curl_trace.txt","wt");
 *   curl_easy_setopt(m_Curl, CURLOPT_DEBUGFUNCTION, &RESTClient::DebugCallback);
 *   curl_easy_setopt(m_Curl, CURLOPT_DEBUGDATA, trace_file);
 *   curl_easy_setopt(m_Curl, CURLOPT_VERBOSE, 1L);
 *   ...
 *   fclose(trace_file)
 */
int
REST_DebugCallback(void *handle, curl_infotype type, char *data, size_t size, void *userp)
{
  FILE       *trace_file = (FILE *)userp;
  const char *text;
  (void)handle; /* prevent compiler warning */

  switch (type)
  {
    case CURLINFO_TEXT:
      fprintf(trace_file, "== Info: %s", data);
    /* FALLTHROUGH */
    default: /* in case a new one is introduced to shock us */
      return 0;

    case CURLINFO_HEADER_OUT:
      text = "=> Send header";
      break;
    case CURLINFO_DATA_OUT:
      text = "=> Send data";
      break;
    case CURLINFO_SSL_DATA_OUT:
      text = "=> Send SSL data";
      break;
    case CURLINFO_HEADER_IN:
      text = "<= Recv header";
      break;
    case CURLINFO_DATA_IN:
      text = "<= Recv data";
      break;
    case CURLINFO_SSL_DATA_IN:
      text = "<= Recv SSL data";
      break;
  }

  REST_DebugDump(text, trace_file, (unsigned char *)data, size, true);
  return 0;
}

template class RESTClient<DSSServerTraits>;
template class RESTClient<DLSServerTraits>;
