/**
 * Test directory creation and Registry IO on paths containing non-ASCII characters.
 *
 * On Windows this depends on the UTF-8 process code page requested by
 * Utilities/Win32/itksnap.manifest. The test works inside its own temporary directory,
 * so it is meaningful regardless of the user name of the account running it.
 */

#include "Registry.h"
#include <itksys/SystemTools.hxx>
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

/**
 * Check that a path exists without going through itksys or the narrow API, so that this
 * answers "does a file with exactly this Unicode name exist" rather than "does a file
 * whose name is these bytes in some code page exist".
 */
bool
ExistsAccordingToNativeAPI(const std::string &utf8_path)
{
#ifdef WIN32
  int n_wchars = MultiByteToWideChar(CP_UTF8, 0, utf8_path.c_str(), (int) utf8_path.length(), NULL, 0);
  if (n_wchars <= 0)
    return false;

  std::wstring wide_path(n_wchars, 0);
  MultiByteToWideChar(CP_UTF8, 0, utf8_path.c_str(), (int) utf8_path.length(), &wide_path[0], n_wchars);

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

int
main(int argc, char *argv[])
{
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

  // The key check: a legacy code page still creates a directory, just a mis-named one.
  ok &= Check(ExistsAccordingToNativeAPI(test_dir),
              "the directory exists under its intended Unicode name");

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
