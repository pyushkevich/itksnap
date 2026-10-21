/**
 * Test directory creation and Registry IO on paths containing non-ASCII characters.
 *
 * The test is set up like ITK-SNAP itself: it calls setlocale(LC_ALL, ".UTF8") as main()
 * does, and on Windows it is linked with the same application manifest
 * (Utilities/Win32/itksnap.manifest). On Windows it also checks
 *  - that SystemInterface starts when %APPDATA% contains non-ASCII characters, as it does
 *    for a user name with non-ASCII characters (ITK-SNAP used to refuse to start), and
 *  - what the manifest is needed for: the UTF-8 process code page, a non-ASCII path
 *    passed on the command line, and GetLongPathNameA, which DecodeFilename() in main.cxx
 *    applies to every path given on the command line.
 * The test works inside its own temporary directory, so it is meaningful regardless of the
 * user name of the account running it.
 */

#include "Registry.h"
#include "SystemInterface.h"
#include "UIReporterDelegates.h"
#include <itksys/SystemTools.hxx>
#include <algorithm>
#include <clocale>
#include <exception>
#include <iostream>
#include <string>

#ifdef WIN32
#  include <windows.h>
#else
#  include <sys/stat.h>
#endif

// U+00FC and U+65E5 U+672C, as explicit UTF-8 bytes so that the test does not depend on
// the encoding the compiler assumes for this source file.
static const char *UTF8_UMLAUT = "\xC3\xBC";
static const char *UTF8_NIHON = "\xE6\x97\xA5\xE6\x9C\xAC";

#ifdef WIN32
/** Convert UTF-8 to UTF-16 explicitly, independent of the process code page */
std::wstring
Utf8ToWide(const std::string &utf8)
{
  int n_wchars = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), (int)utf8.length(), NULL, 0);
  if (n_wchars <= 0)
    return std::wstring();

  std::wstring wide(n_wchars, 0);
  MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), (int)utf8.length(), &wide[0], n_wchars);
  return wide;
}
#endif

/**
 * Check that a path exists without going through itksys or the narrow API, so that this
 * answers "does a file with exactly this Unicode name exist" rather than "does a file
 * whose name is these bytes in some code page exist".
 */
bool
ExistsAccordingToNativeAPI(const std::string &utf8_path)
{
#ifdef WIN32
  std::wstring wide_path = Utf8ToWide(utf8_path);
  if (wide_path.empty())
    return false;

  return GetFileAttributesW(wide_path.c_str()) != INVALID_FILE_ATTRIBUTES;
#else
  struct stat buf;
  return stat(utf8_path.c_str(), &buf) == 0;
#endif
}

bool
Check(bool condition, const char *description)
{
  std::cout << (condition ? "PASSED: " : "FAILED: ") << description << std::endl;
  return condition;
}

#ifdef WIN32
/** The path the way a shell passes it on the command line: with backslashes */
std::string
ToBackslashes(std::string path)
{
  std::replace(path.begin(), path.end(), '/', '\\');
  return path;
}

/**
 * Child side of PathSurvivesCommandLine(): argv[2] must still name the existing directory,
 * both for the wide API and for GetLongPathNameA, as used by DecodeFilename() in main.cxx.
 */
int
CheckPathFromCommandLine(const char *arg)
{
  return (ExistsAccordingToNativeAPI(arg) && GetLongPathNameA(arg, NULL, 0) > 0) ? 0 : 1;
}

/**
 * Run this executable again with a non-ASCII path on its command line, the way a shell or
 * Explorer passes a file to ITK-SNAP. The command line is UTF-16; the child's argv is
 * decoded with the process code page, which the manifest sets to UTF-8.
 */
bool
PathSurvivesCommandLine(const std::string &utf8_path)
{
  wchar_t exe[4096];
  DWORD   n_exe = GetModuleFileNameW(NULL, exe, 4096);
  if (n_exe == 0 || n_exe >= 4096)
    return false;

  std::wstring cmd = L"\"" + std::wstring(exe) + L"\" --check-argv \"" +
                     Utf8ToWide(ToBackslashes(utf8_path)) + L"\"";

  STARTUPINFOW        si = { sizeof(si) };
  PROCESS_INFORMATION pi;
  if (!CreateProcessW(NULL, &cmd[0], NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
    return false;

  WaitForSingleObject(pi.hProcess, INFINITE);
  DWORD rc = 1;
  GetExitCodeProcess(pi.hProcess, &rc);
  CloseHandle(pi.hThread);
  CloseHandle(pi.hProcess);
  return rc == 0;
}

/** Minimal delegate; SystemInterface only requires one to be set */
class TestSystemInfoDelegate : public SystemInfoDelegate
{
public:
  std::string GetApplicationDirectory() override { return std::string(); }
  std::string GetApplicationFile() override { return std::string(); }
  std::string GetApplicationPermanentDataLocation() override { return std::string(); }
  std::string GetUserDocumentsLocation() override { return std::string(); }
  std::string GetTempDirectory() override { return std::string(); }
  std::string EncodeServerURL(const std::string &url) override { return url; }
  void        LoadResourceAsImage2D(std::string, GrayscaleImage *) override {}
  void        LoadResourceAsRegistry(std::string, Registry &) override {}
  void        WriteRGBAImage2D(std::string, RGBAImageType *) override {}
};

/**
 * Point %APPDATA% at a non-ASCII directory and construct SystemInterface, which creates
 * the application data directory there. This is what failed for Windows user names
 * containing non-ASCII characters.
 */
bool
SystemInterfaceStartsWithAppData(const std::string &utf8_appdata)
{
  if (!SetEnvironmentVariableW(L"APPDATA", Utf8ToWide(utf8_appdata).c_str()))
    return false;

  try
  {
    SystemInterface si;
    std::string     expected = utf8_appdata + "/itksnap.org/ITK-SNAP";
    return si.GetApplicationDataDirectory() == expected && ExistsAccordingToNativeAPI(expected);
  }
  catch (std::exception &exc)
  {
    std::cout << "  exception while creating SystemInterface: " << exc.what() << std::endl;
    return false;
  }
}
#endif

int
main(int argc, char *argv[])
{
#ifdef WIN32
  // Same locale as ITK-SNAP's main(), which Registry's file IO relies on
  std::setlocale(LC_ALL, ".UTF8");

  // Child process started by PathSurvivesCommandLine()
  if (argc == 3 && std::string(argv[1]) == "--check-argv")
    return CheckPathFromCommandLine(argv[2]);
#endif

  if (argc < 2)
  {
    std::cerr << "usage: nonascii_path_test <temp_dir>" << std::endl;
    return 1;
  }

  // Build a directory name containing non-ASCII characters inside the temp directory
  std::string base_dir = argv[1];
  itksys::SystemTools::ConvertToUnixSlashes(base_dir);
  std::string test_dir = base_dir + "/nonascii_M" + UTF8_UMLAUT + "ller_" + UTF8_NIHON;

  // ... and a file with a non-ASCII name inside of it
  std::string test_file = test_dir + "/Pr" + UTF8_UMLAUT + "ferenzen.xml";

  // A value with non-ASCII characters, standing in for a label name
  std::string test_value = std::string("Gr") + UTF8_UMLAUT + "n " + UTF8_NIHON;

  std::cout << "Test directory (UTF-8): " << test_dir << std::endl;

  bool ok = true;

  // The cast is needed because MakeDirectory returns kwsys::Status, whose operator bool
  // is explicit.
  ok &= Check(static_cast<bool>(itksys::SystemTools::MakeDirectory(test_dir.c_str())),
              "itksys::SystemTools::MakeDirectory reports success");

  ok &= Check(itksys::SystemTools::FileIsDirectory(test_dir.c_str()),
              "itksys::SystemTools::FileIsDirectory finds the directory");

  // Check the name through the wide API, so that a mis-decoded (mojibake) name is caught
  // rather than reported as success
  ok &= Check(ExistsAccordingToNativeAPI(test_dir),
              "the directory exists under its intended Unicode name");

#ifdef WIN32
  // The case ITK-SNAP used to refuse: %APPDATA% containing non-ASCII characters
  TestSystemInfoDelegate sidel;
  SystemInterface::SetSystemInfoDelegate(&sidel);
  ok &= Check(SystemInterfaceStartsWithAppData(test_dir + "/AppData_" + UTF8_NIHON),
              "SystemInterface starts with a non-ASCII %APPDATA%");

  // What the application manifest is needed for
  ok &= Check(GetACP() == CP_UTF8, "the process code page is UTF-8 (application manifest)");
  ok &= Check(GetLongPathNameA(ToBackslashes(test_dir).c_str(), NULL, 0) > 0,
              "GetLongPathNameA resolves the UTF-8 path, as DecodeFilename() requires");
  ok &= Check(PathSurvivesCommandLine(test_dir),
              "a non-ASCII path passed on the command line reaches argv intact");
#endif

  // Registry writes through the CRT rather than itksys, exercising the other path
  Registry reg_out;
  reg_out["TestEntry"] << test_value;
  reg_out["Nested.Folder.Entry"] << 42;

  bool write_ok = true;
  try
  {
    reg_out.WriteToXMLFile(test_file.c_str());
  }
  catch (std::exception &exc)
  {
    std::cout << "  exception while writing: " << exc.what() << std::endl;
    write_ok = false;
  }
  catch (...)
  {
    // Registry::IOException derives from std::string, not std::exception
    std::cout << "  non-standard exception while writing" << std::endl;
    write_ok = false;
  }
  ok &= Check(write_ok, "Registry::WriteToXMLFile writes to a non-ASCII path");

  ok &= Check(ExistsAccordingToNativeAPI(test_file),
              "the preference file exists under its intended Unicode name");

  ok &= Check(itksys::SystemTools::FileExists(test_file.c_str(), true),
              "itksys::SystemTools::FileExists finds the preference file");

  // Read it back and make sure both the path and the non-ASCII content round-tripped
  bool     read_ok = true;
  Registry reg_in;
  try
  {
    reg_in.ReadFromXMLFile(test_file.c_str());
  }
  catch (std::exception &exc)
  {
    std::cout << "  exception while reading: " << exc.what() << std::endl;
    read_ok = false;
  }
  catch (...)
  {
    // Registry::IOException derives from std::string, not std::exception
    std::cout << "  non-standard exception while reading" << std::endl;
    read_ok = false;
  }
  ok &= Check(read_ok, "Registry::ReadFromXMLFile reads from a non-ASCII path");

  if (read_ok)
  {
    ok &= Check(reg_in["TestEntry"][std::string()] == test_value,
                "a non-ASCII registry value round-trips unchanged");
    // -1 as the default: a literal 0 would be ambiguous between the int and
    // const char * overloads of RegistryValue::operator[]
    ok &= Check(reg_in["Nested.Folder.Entry"][-1] == 42, "a nested registry entry round-trips");
  }

  // Clean up, ignoring failures so that a cleanup problem does not mask the result
  itksys::SystemTools::RemoveADirectory(test_dir.c_str());

  std::cout << (ok ? "All checks passed." : "One or more checks FAILED.") << std::endl;
  return ok ? 0 : 1;
}
