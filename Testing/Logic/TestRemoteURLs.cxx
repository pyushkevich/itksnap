// Test: parsing of remote URLs, without any network access.
//
//   1. IsRemoteImageURL, ResolveITKSnapURL (the itksnap-* browser schemes) and
//      HasWorkspaceExtension, CollapseFullPathOrURL
//   2. UrlDecode
//   3. CreateRemoteImageSource picks the source for each scheme
//   4. ParseSCPURL: user, encoded user, port, path; malformed URLs
//   5. RemoteFilenameFromURL: query, fragment, encoded characters, and URLs
//      without a filename
//   6. GuessFormatForFileName on URLs with a query, fragment or encoded dot
//
// Usage: TestRemoteURLs
#include "ImageIORemote.h"
#include "IRISException.h"
#include "GuidedNativeImageIO.h"
#include <iostream>
#include <string>

static int n_failures = 0;

static void Check(bool cond, const std::string &what)
{
  std::cout << (cond ? "  ok:   " : "  FAIL: ") << what << std::endl;
  if(!cond)
    n_failures++;
}

static void CheckEqual(const std::string &got, const std::string &expected, const std::string &what)
{
  Check(got == expected, what + " -> \"" + got + "\"" +
        (got == expected ? "" : " (expected \"" + expected + "\")"));
}

// Whether the function throws an IRISException
template <class F>
static bool Throws(F f)
{
  try { f(); }
  catch(IRISException &) { return true; }
  return false;
}

// Name of the remote source class created for a URL, or "" if none
static std::string SourceFor(const std::string &url)
{
  try { return CreateRemoteImageSource(url)->GetNameOfClass(); }
  catch(IRISException &) { return ""; }
}

int main()
{
  // ── 1. Remote URL detection and itksnap-* schemes ──────────────────────
  std::cout << "IsRemoteImageURL / ResolveITKSnapURL:" << std::endl;
  Check(IsRemoteImageURL("scp://host/data/img.nii.gz"), "scp URL is remote");
  Check(IsRemoteImageURL("itksnap-https://host/img.nii.gz"), "itksnap-https URL is remote");
  Check(!IsRemoteImageURL("/data/img.nii.gz"), "absolute path is not remote");
  Check(!IsRemoteImageURL("C:\\data\\img.nii.gz"), "Windows path is not remote");
  Check(!IsRemoteImageURL("C:/data/img.nii.gz"), "Windows path with slashes is not remote");
  CheckEqual(ResolveITKSnapURL("itksnap-sftp://user@host/a/b.nii.gz"), "sftp://user@host/a/b.nii.gz", "itksnap-sftp");
  CheckEqual(ResolveITKSnapURL("itksnap-scp://host/a.nii"), "scp://host/a.nii", "itksnap-scp");
  CheckEqual(ResolveITKSnapURL("itksnap-https://h/f.nii.gz?x=1#y"), "https://h/f.nii.gz?x=1#y", "itksnap-https keeps query and fragment");
  CheckEqual(ResolveITKSnapURL("itksnap-fw://server/group/project"), "fw://server/group/project", "itksnap-fw");
  CheckEqual(ResolveITKSnapURL("https://h/f.nii.gz"), "https://h/f.nii.gz", "plain URL unchanged");
  CheckEqual(ResolveITKSnapURL("/data/itksnap-x.nii"), "/data/itksnap-x.nii", "local path unchanged");
  Check(HasWorkspaceExtension("https://h/a/study.itksnap"), "workspace URL");
  Check(HasWorkspaceExtension("https://h/a/study.ITKSNAP?token=x#y"), "workspace URL with query and fragment");
  Check(!HasWorkspaceExtension("https://h/a/img.nii.gz?f=study.itksnap"), "image URL with .itksnap in the query");
  Check(HasWorkspaceExtension("/data/study.itksnap"), "local workspace");
  Check(!HasWorkspaceExtension("/data/img.nii.gz"), "local image");
  Check(!HasWorkspaceExtension("itksnap"), "short name");
  CheckEqual(CollapseFullPathOrURL("https://h/a/../b.nii.gz?x=1"), "https://h/a/../b.nii.gz?x=1", "URL not collapsed");
  CheckEqual(CollapseFullPathOrURL("/data/a/../b.nii.gz"), "/data/b.nii.gz", "local path collapsed");

  // ── 2. Percent decoding ───────────────────────────────────────────────
  std::cout << "UrlDecode:" << std::endl;
  CheckEqual(UrlDecode("scan%20T1.nii.gz"), "scan T1.nii.gz", "%20");
  CheckEqual(UrlDecode("a%2Fb%2fc"), "a/b/c", "upper and lower case hex");
  CheckEqual(UrlDecode("a+b"), "a+b", "'+' is literal");
  CheckEqual(UrlDecode("100%"), "100%", "trailing %");
  CheckEqual(UrlDecode("%4"), "%4", "incomplete escape");
  CheckEqual(UrlDecode("%zz"), "%zz", "invalid escape");
  CheckEqual(UrlDecode("%C3%A9"), "\xC3\xA9", "UTF-8 bytes");

  // ── 3. Source for each scheme ─────────────────────────────────────────
  std::cout << "CreateRemoteImageSource:" << std::endl;
  CheckEqual(SourceFor("scp://h/f.nii"), "SCPRemoteImageSource", "scp");
  CheckEqual(SourceFor("sftp://h/f.nii"), "SCPRemoteImageSource", "sftp");
  CheckEqual(SourceFor("http://h/f.nii"), "HTTPRemoteImageSource", "http");
  CheckEqual(SourceFor("https://h/f.nii"), "HTTPRemoteImageSource", "https");
  CheckEqual(SourceFor("fw://server/g/p"), "FlywheelRemoteImageSource", "fw");
  CheckEqual(SourceFor("itksnap-https://h/f.nii"), "", "itksnap-* must be resolved first");
  CheckEqual(SourceFor("ftp://h/f.nii"), "", "unsupported scheme");
  CheckEqual(SourceFor("/data/f.nii"), "", "local path");

  // ── 4. scp/sftp URLs ──────────────────────────────────────────────────
  std::cout << "ParseSCPURL:" << std::endl;
  SCPURL u = ParseSCPURL("scp://host/data/img.nii.gz");
  Check(u.username.empty() && u.host == "host" && u.port == 0 && u.path == "/data/img.nii.gz",
        "host and path");
  u = ParseSCPURL("sftp://alice@host.example.org:2222/home/alice/scan%20T1.nii.gz");
  Check(u.username == "alice" && u.host == "host.example.org" && u.port == 2222
        && u.path == "/home/alice/scan T1.nii.gz", "user, port and encoded path");
  u = ParseSCPURL("scp://alice%40lab.org@host/f.nii");
  Check(u.username == "alice@lab.org" && u.host == "host", "encoded @ in user name");
  u = ParseSCPURL("scp://a@b@host/f.nii");
  Check(u.username == "a@b" && u.host == "host", "last @ separates user from host");
  Check(Throws([] { ParseSCPURL("scp://host"); }), "no path is an error");
  Check(Throws([] { ParseSCPURL("scp:///data/f.nii"); }), "no host is an error");
  Check(Throws([] { ParseSCPURL("scp://user@/data/f.nii"); }), "user but no host is an error");
  Check(Throws([] { ParseSCPURL("scp://host:abc/f.nii"); }), "non-numeric port is an error");
  Check(Throws([] { ParseSCPURL("scp://host:/f.nii"); }), "empty port is an error");
  Check(Throws([] { ParseSCPURL("scp://host:70000/f.nii"); }), "port out of range is an error");
  Check(Throws([] { ParseSCPURL("https://host/f.nii"); }), "other scheme is an error");

  // ── 5. Local filename of a download ───────────────────────────────────
  std::cout << "RemoteFilenameFromURL:" << std::endl;
  CheckEqual(RemoteFilenameFromURL("https://h/a/b/img.nii.gz"), "img.nii.gz", "plain");
  CheckEqual(RemoteFilenameFromURL("https://h/scan%20T1.nii.gz?token=x&y=1"), "scan T1.nii.gz",
             "query removed, %20 decoded");
  CheckEqual(RemoteFilenameFromURL("https://h/img.nii.gz#frag"), "img.nii.gz", "fragment removed");
  CheckEqual(RemoteFilenameFromURL("https://h/MRIcrop-orig.gipl%2Egz"), "MRIcrop-orig.gipl.gz", "encoded dot");
  CheckEqual(RemoteFilenameFromURL("https://h/dir%2F..%2Fimg.nii"), "img.nii", "encoded slashes drop the directories");
  Check(Throws([] { RemoteFilenameFromURL("https://h/data/"); }), "trailing slash is an error");
  Check(Throws([] { RemoteFilenameFromURL("https://h/data/.."); }), "'..' is an error");
  Check(Throws([] { RemoteFilenameFromURL("https://h/data/%2E%2E"); }), "encoded '..' is an error");
  Check(Throws([] { RemoteFilenameFromURL("https://h/data/?x=1"); }), "query with no filename is an error");

  // ── 6. Image format guessed from a URL ────────────────────────────────
  std::cout << "GuessFormatForFileName:" << std::endl;
  auto guess = [](const std::string &f) { return GuidedNativeImageIO::GuessFormatForFileName(f, false); };
  Check(guess("https://h/img.nii.gz?token=a%2Fb&x=1#frag") == GuidedNativeImageIO::FORMAT_NIFTI,
        "NIfTI URL with query and fragment");
  Check(guess("https://h/MRIcrop-orig.gipl%2Egz") == GuidedNativeImageIO::FORMAT_GIPL, "GIPL URL with encoded dot");
  Check(guess("/data/img.nii.gz") == GuidedNativeImageIO::FORMAT_NIFTI, "local NIfTI");
  Check(guess("https://h/data/") == GuidedNativeImageIO::FORMAT_COUNT, "URL with no filename");

  std::cout << (n_failures ? "FAILED: " : "PASSED: ") << n_failures << " failure(s)" << std::endl;
  return n_failures ? 1 : 0;
}
