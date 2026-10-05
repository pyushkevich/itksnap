/**
 * TestColorLabelTableFile.cxx
 *
 * Verifies that ColorLabelTable recognizes and loads label description files
 * written with Windows line endings or without the header comment, and that
 * it does not mistake other files for label descriptions. ValidateFile() is
 * what decides whether a file dropped on the main window is loaded as label
 * descriptions or offered as an image.
 */

#include "ColorLabelTable.h"

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>

static int errors = 0;

static void
check(bool condition, const std::string &message)
{
  if (!condition)
  {
    std::cerr << "FAILED: " << message << std::endl;
    errors++;
  }
}

static std::string
writeFile(const std::string &dir, const std::string &name, const std::string &content)
{
  std::string   path = dir + "/" + name;
  std::ofstream out(path.c_str(), std::ios::binary);
  out << content;
  return path;
}

int
main(int argc, char *argv[])
{
  if (argc < 2)
  {
    std::cerr << "Usage: " << argv[0] << " <temp_dir>" << std::endl;
    return EXIT_FAILURE;
  }
  std::string dir = argv[1];

  const std::string sep = "################################################";
  const std::string header = "# ITK-SnAP Label Description File";
  const std::string entries[] = { "0 0 0 0 0 0 0 \"Clear Label\"",
                                  "1 97 23 22 1 1 1 \"TMv - 1\"",
                                  "2 124 100 148 1 1 1 \"SSp-m6b - 2\"" };

  std::string unixFile = sep + "\n" + header + "\n" + sep + "\n\n";
  std::string windowsFile = sep + "\r\n" + header + "\r\n" + sep + "\r\n\r\n";
  std::string headerlessFile = sep + "\n" + sep + "\n";
  for (const std::string &e : entries)
  {
    unixFile += e + "\n";
    windowsFile += e + "\r\n";
    headerlessFile += e + "\n";
  }

  struct Case
  {
    const char *name;
    std::string content;
    bool        isLabelFile;
  } cases[] = {
    { "labels_unix.txt", unixFile, true },
    { "labels_windows.txt", windowsFile, true },
    { "labels_no_header.txt", headerlessFile, true },
    { "notes.txt", "# Some notes\nThis is not a label file\n", false },
    { "empty.txt", "", false },
    { "image.nrrd", "NRRD0004\n# Complete NRRD file format specification\ntype: short\n", false },
  };

  for (const Case &c : cases)
  {
    std::string path = writeFile(dir, c.name, c.content);

    ColorLabelTable::Pointer table = ColorLabelTable::New();
    check(table->ValidateFile(path.c_str()) == c.isLabelFile,
          std::string("ValidateFile(") + c.name + ") should return " +
            (c.isLabelFile ? "true" : "false"));

    if (c.isLabelFile)
    {
      try
      {
        table->LoadFromFile(path.c_str());
        check(table->GetNumberOfValidLabels() == 3,
              std::string(c.name) + " should load 3 labels (including clear label)");
        check(table->GetColorLabel(2).GetLabel() == std::string("SSp-m6b - 2"),
              std::string(c.name) + " should load the name of label 2 intact");
      }
      catch (std::exception &exc)
      {
        check(false, std::string("LoadFromFile(") + c.name + ") threw: " + exc.what());
      }
    }
  }

  if (errors == 0)
    std::cout << "All label description file tests passed" << std::endl;
  return errors == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
