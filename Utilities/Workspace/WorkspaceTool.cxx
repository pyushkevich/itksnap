/*=========================================================================

  Program:   ITK-SNAP
  Language:  C++
  Copyright (c) 2017 Paul A. Yushkevich
  
  This file is part of ITK-SNAP 

  ITK-SNAP is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

  -----

  Copyright (c) 2003 Insight Software Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notices for more information. 

=========================================================================*/
#include <iostream>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <string>
#include <cstdarg>

#ifdef WIN32
#define CURL_STATICLIB 
#endif
#include <curl/curl.h>

#include "CommandLineHelper.h"
#include "Registry.h"
#include "GuidedNativeImageIO.h"
#include "itksys/MD5.h"
#include "itksys/Directory.hxx"
#include "itksys/RegularExpression.hxx"
#include "IRISException.h"
#include "ColorLabelTable.h"

using namespace std;
using itksys::SystemTools;
using itksys::RegularExpression;
using itksys::Directory;

#ifdef WIN32

void sleep(int n_sec)
{
  Sleep(1000 * n_sec);
}

#else

#endif

int usage(int rc) 
{
  cout << "itksnap_ws : ITK-SNAP Workspace Tool" << endl;
  cout << "Usage: " << endl;
  cout << "  itksnap_ws [commands]" << endl;
  cout << "I/O commands: " << endl;
  cout << "  -i <workspace>                    : Read workspace file" << endl;
  cout << "  -o <workspace>                    : Write workspace file (without touching external images)" << endl;
  cout << "  -a <dest_dir>                     : Package workspace into uploadable archive in dest_dir" << endl;
  cout << "  -p <prefix>                       : Set the output prefix for the next command only" << endl;
  cout << "  -P                                : No printing of prefix for output commands" << endl;
  cout << "Informational commands: " << endl;
  cout << "  -dump                             : Dump workspace in human-readable format" << endl;
  cout << "  -get <key>                        : Get the value of a specified key" << endl;
  cout << "Commands for adding/setting/select image layers: " << endl;
  cout << "  -layers-add-anat <file>           : Add an image as next anatomical layer" << endl;
  cout << "  -layers-add-seg <file>            : Add an image as next segmentation layer" << endl;
  cout << "  -layers-set-main <file>           : Set the main anatomical image" << endl;
  cout << "  -layers-set-seg <file>            : Set a single segmentation image - all other segs are trashed" << endl;
  cout << "  -layers-list                      : List all the layers in the workspace" << endl;
  cout << "  -layers-list-files <tag>          : List just the files associated with a specified tag" << endl;
  cout << "  -layers-pick <layer_id>           : Pick image layer for subsequent prop commands" << endl;
  cout << "  -layers-pick-by-tag <tag>         : Pick layer by tag, error if more than one has tag" << endl;
  cout << "Commands for modifying picked layer properties:" << endl;
  cout << "  -props-get-filename               : Print the filename of the picked layer" << endl;
  cout << "  -props-set-nickname <name>        : Set the nickname of the selected layer" << endl;
  cout << "  -props-set-colormap <preset>      : Set colormap to a given system preset" << endl;
  cout << "  -props-set-contrast <map_spec>    : Set the contrast mapping specification" << endl;
  cout << "  -props-set-sticky <on|off>        : Set the stickiness of the layer" << endl;
  cout << "  -props-set-alpha <value>          : Set the alpha (transparency) of the layer" << endl;
  cout << "Tag assignment commands (apply to picked layer/object): " << endl;
  cout << "  -tags-add <tag>                   : Add a tag to the picked object" << endl;
  cout << "  -tags-add-excl <tag>              : Add a tag that is exclusive to the object" << endl;
  cout << "  -tags-del <tag>                   : Remove tag from the object" << endl;
  cout << "Segmentation label commands" << endl;
  cout << "  -labels-set <file>                : Read the labels from the labelfile specified" << endl;
  cout << "  -labels-clear                     : Remove all labels except the default clear label" << endl;
  cout << "  -labels-add <file> [offst] [ptrn] : Add labels from file, optionally shifting by offset and" << endl;
  cout << "                                      renaming with C printf pattern (e.g. 'left %s')" << endl;
  cout << "Distributed segmentation server user commands: " << endl;
  cout << "  -dss-auth <url> [user] [passwd]   : Sign in to the server. This will create a token" << endl;
  cout << "                                      that may be used in future -dss calls" << endl;
  cout << "  -dss-services-list                : List all available segmentation services" << endl;
  cout << "  -dss-tickets-create <service_id>  : Create a new ticket using current workspace (id is githash)" << endl;
  cout << "  -dss-tickets-list                 : List all of your tickets" << endl;
  cout << "  -dss-tickets-log <id>             : Get the error/warning/info log for ticket 'id'" << endl;
  cout << "  -dss-tickets-progress <id>        : Get the total progress for ticket 'id'" << endl;
  cout << "  -dss-tickets-wait <id> [timeout]  : Wait for the ticket 'id' to complete" << endl;
  cout << "  -dss-tickets-download <id> <dir>  : Download the result for ticket 'id' to directory 'dir'" << endl;
  cout << "Distributed segmentation server provider commands: " << endl;
  cout << "  -dssp-services-list               : List all the services you are listed as provider for" << endl;
  cout << "  -dssp-services-claim <service_hash> <provider> <instance_id> [timeout]" << endl;
  cout << "                                    : Claim the next available ticket for given service." << endl;
  cout << "                                      'instance_id' is a unique identifier for this service instance" << endl;
  cout << "                                      if 'timeout' specified, command will halt until a ticket " << endl;
  cout << "                                      can be claimed or 'timeout' seconds pass." << endl;
  cout << "  -dssp-tickets-download <id> <dir> : Download the files for claimed ticket id to dir" << endl;
  cout << "  -dssp-tickets-fail <id> <msg>     : Mark the ticket as failed with provided message" << endl;
  cout << "  -dssp-tickets-success <id>        : Mark the ticket as successfully completed " << endl;
  cout << "  -dssp-tickets-set-progress ...    : Set progress for ticket 'id'. Progress is specified as" << endl;
  cout << "      <id> <start> <end> <value>      chunk start, chunk end, and progress within chunk (all in [0 1])" << endl;
  cout << "  -dssp-tickets-log ...             : Add a log message for ticket 'id'. Type is info|warning|error" << endl; 
  cout << "      <id> <type> <msg>               " << endl;
  cout << "  -dssp-tickets-attach ...          : Attach a file to the ticket <id>. The file will be linked to the next" << endl;
  cout << "      <id> <desc> <file> [mimetype]   log command issued for this ticket" << endl;
  cout << "  -dssp-tickets-upload <id>         : Send the current workspace as the result for ticket 'id'" << endl;
  cout << "Specifying Layer IDs:" << endl;
  cout << "  ###                               : Selects any layer by number (e.g., 003)" << endl;
  cout << "  M|main                            : Selects the main layer " << endl;
  cout << "  S|seg                             : Selects the segmentation layer " << endl;
  cout << "  <O|overlay>:N                     : Selects N-th overlay layer (N may be -1 for last)" << endl;
  cout << "  <A|anat>:N                        : Selects N-th anatomical (main|overlay) layer" << endl;
  cout << "Contrast Mapping Specification:" << endl;
  cout << "  LINEAR N1 N2                      : Linear contrast between specified numbers" << endl;
  cout << "  AUTO                              : Automatic, as determined by ITK-SNAP" << endl;
  cout << "  DEFAULT                           : Default state, linear from 0 to 1" << endl;
  cout << "  CURVE N t1 y1 ... tN yN           : Fully specified curve with N points" << endl; 
  return rc;
}

/**
 * CSV parser
 */
class CSVParser
{
public:
  /** Parse the contents of a CSV encoded string with quote escapes */
  void Parse(const std::string &str)
    {
    m_Data.clear();
    m_Columns = 0;

    istringstream iss(str);
    string line, word;
    while(getline(iss, line))
      {
      int iCol = 0;

      bool in_quoted = false;
      for(int k = 0; k < line.length(); k++)
        {
        // Get the next character
        char c = line[k];

        // If the character is a quote, it either starts/ends a quote or is 
        // an escaped quite character
        if(c == '"')
          {
          if(k+1 < line.length() && line[k+1] == '"')
            {
            // This is just an escaped quote, treat it like a normal character
            ++k;
            }
          else
            {
            // Toggle the in_quoted state and continue to the next character
            in_quoted = !in_quoted;
            continue;
            }
          }

        else if((c == ',' && !in_quoted) || c == '\n' || c == '\r')
          {
          // This is the end of a field
          m_Data.push_back(word);
          word = string();
          iCol++;
          continue;
          }

        // Normal character
        word.push_back(c);
        }

      // Update the column count
      if(m_Columns < iCol)
        m_Columns = iCol;
      }
    }

  int GetNumberOfColumns() const
    { return m_Columns; }

  const vector<string> &GetParsedStrings()  const
    { return m_Data; }

protected:

  int m_Columns;
  vector<string> m_Data;

};

/**
 * A helper class for formatting tables. You just specify the number of columns
 * and then add elements with the << operator (header and data elements are 
 * added the same way). Then pipe it to cout
 */
class FormattedTable
{
public:
  /** 
   * Construct the table with a predefined number of columns
   */
  FormattedTable(int n_col)
    {
    m_Columns = n_col;
    m_Width.resize(n_col, 0);
    m_RowEnded = true;
    }

  /**
   * Construct the table without a predefined number of columns. Instead
   * the table will be constructed using calls to EndRow()
   */
  FormattedTable()
    {
    m_Columns = 0;
    m_RowEnded = true;
    }

  template <class TAtomic> FormattedTable& operator << (const TAtomic &datum)
    {
    // Convert the datum to a string and measure its length
    ostringstream oss;
    oss << datum;
    int w = oss.str().length();

    // We need to add a new row in two cases: there is currently no row, or the
    // current row has been filled to m_Columns
    if(m_RowEnded)
      {
      m_Data.push_back(RowType());
      if(m_Columns > 0)
        m_Data.back().reserve(m_Columns);
      m_RowEnded = false;
      }

    // Now we have a row to add to that is guaranteed to be under m_Columns
    m_Data.back().push_back(oss.str());

    // If the number of columns is fixed, then call EndRow() automatically
    if(m_Columns > 0 && m_Data.back().size() >= m_Columns)
      m_RowEnded = true;

    // Now update the column width information
    if(m_Width.size() < m_Data.back().size())
      m_Width.push_back(w);
    else
      m_Width[m_Data.back().size() - 1] = std::max(w, m_Width[m_Data.back().size() - 1]);

    return *this;
    }

  void EndRow() 
    {
    m_RowEnded = true;
    }

  void PrintRow(ostream &os, int iRow, const string &prefix = "", std::vector<bool> col_filter = std::vector<bool>()) const
    {
    const RowType &row = m_Data[iRow];
    os << prefix << left;
    for(int iCol = 0; iCol < row.size(); iCol++)
      {
      if(col_filter.size() == 0 || col_filter[iCol])
        os << setw(m_Width[iCol] + 2) << row[iCol];
      }
    os << endl;
    }

  void Print(ostream &os, const string &prefix = "", std::vector<bool> col_filter = std::vector<bool>()) const
    {
    for(int iRow = 0; iRow < m_Data.size(); iRow++)
      this->PrintRow(os, iRow, prefix, col_filter);
    }

  int Rows() const { return m_Data.size(); }

  int Columns() const{ return m_Width.size(); }

  const std::string &operator() (int iRow, int iCol) const { return m_Data[iRow][iCol]; }

  /** Parse the contents of a CSV encoded string with quote escapes */
  void ParseCSV(const std::string &str)
    {
    istringstream iss(str);
    string line, word;
    while(getline(iss, line))
      {
      bool in_quoted = false;
      for(int k = 0; k < line.length(); k++)
        {
        // Get the next character
        char c = line[k];

        // If the character is a quote, it either starts/ends a quote or is 
        // an escaped quite character
        if(c == '"')
          {
          if(k+1 < line.length() && line[k+1] == '"')
            {
            // This is just an escaped quote, treat it like a normal character
            ++k;
            }
          else
            {
            // Toggle the in_quoted state and continue to the next character
            in_quoted = !in_quoted;
            continue;
            }
          }

        else if((c == ',' && !in_quoted) || c == '\n' || c == '\r')
          {
          // This is the end of a field
          (*this) << word;
          word.clear();

          // Endrow
          if(c != ',' && m_Columns == 0)
            EndRow();

          continue;
          }

        // Normal character
        word.push_back(c);
        }
      }
    }

protected:
  typedef vector<string> RowType;
  typedef vector<RowType> TableType;

  // The data in the table
  TableType m_Data;

  // The string width measurements in the table
  vector<int> m_Width;

  // The number of columns (may change dynamically)
  int m_Columns;

  // Was row just ended
  bool m_RowEnded;
};

/*
{
  ft.Print(oss);
  return oss;
}*/


/**
 * This class encapsulates an ITK-SNAP workspace. It is just a wrapper around 
 * a registry object, but with extra functions that support workspaces
 */
class Workspace
{
public:

  /**
   * Read the workspace from a file, determine if it has been moved or copied
   * since it was saved originally.
   */
  void ReadFromXMLFile(const char *proj_file)
    {
    // Read the contents of the project from the file
    m_Registry.ReadFromXMLFile(proj_file);

    // Get the full name of the project file
    m_WorkspaceFilePath = SystemTools::CollapseFullPath(proj_file);

    // Get the directory in which the project will be saved
    m_WorkspaceFileDir = SystemTools::GetParentDirectory(m_WorkspaceFilePath);

    // Read the location where the file was saved initially
    m_WorkspaceSavedDir = m_Registry["SaveLocation"][""];

    // If the locations are different, we will attempt to find relative paths first
    m_Moved = (m_WorkspaceFileDir != m_WorkspaceSavedDir);
    }

  /**
   * Write the workspace to an XML file. The workspace data structure
   * is updated to reflect the new file (i.e., this is a SaveAs pattern).
   *
   * This method assumes that all the referenced image paths are correct. It does
   * not automatically try to account for a moved workspace
   *
   * You probably want to call FixMovedWorkspace() before doing SaveAs, since when the
   * Workspace is loaded, the paths may no longer be valid
   */
  void SaveAsXMLFile(const char *proj_file)
    {
    // Get the full name of the project file
    string proj_file_full = SystemTools::CollapseFullPath(proj_file);

    // Get the directory in which the project will be saved
    string project_dir = SystemTools::GetParentDirectory(proj_file_full.c_str());

    // Update the save location
    m_Registry["SaveLocation"] << project_dir;

    // Update all the paths
    this->SetAllLayerPathsToActualPaths();

    // Write the registry
    m_Registry.WriteToXMLFile(proj_file_full.c_str());

    // Update the internal values
    m_Moved = false;
    m_WorkspaceSavedDir = project_dir;
    m_WorkspaceFileDir = project_dir;
    m_WorkspaceFilePath = proj_file_full;
    }

  /**
   * Get number of image layers in the workspace
   */
  int GetNumberOfLayers()
    {
    // Unfortunately we have to count the folders each time we want to return the number
    // of layers. This is the unfortunate consequence of the lame way in which Registry
    // supports arrays of folders.
    int n_layers = 0;
    while(m_Registry.HasFolder(Registry::Key("Layers.Layer[%03d]", n_layers)))
      n_layers++;

    return n_layers;
    }

  /**
   * Get the folder for the n-th layer
   */
  Registry &GetLayerFolder(int layer_index)
    {
    string key = Registry::Key("Layers.Layer[%03d]", layer_index);
    return m_Registry.Folder(key);
    }

  /**
   * Check if the provided key specifies a valid layer - a valid layer is specified
   * as "Layers.Layer[xxx]" and contains an absolute filename and a role as the minimum
   * required entries
   */
  bool IsKeyValidLayer(const string &key)
    {
    RegularExpression re("Layers.Layer\\[[0-9]+\\]");
    if(!re.find(key))
      return false;

    if(!m_Registry.HasFolder(key))
      return false;

    Registry &folder = m_Registry.Folder(key);
    
    return folder.HasEntry("AbsolutePath") && folder.HasEntry("Role");
    }

  /**
   * Find a physical file corresponding to a file referenced by the project, accouting
   * for the possibility that the project may have been moved or copied
   */
  string GetLayerActualPath(Registry &folder)
    {
    // Get the filenames for the layer
    string layer_file_full = folder["AbsolutePath"][""];

    // If the project has moved, try finding a relative location
    if(m_Moved)
      {
      string relative_path = SystemTools::RelativePath(
            m_WorkspaceSavedDir.c_str(), layer_file_full.c_str());

      string moved_file_full = SystemTools::CollapseFullPath(
            relative_path.c_str(), m_WorkspaceFileDir.c_str());

      if(SystemTools::FileExists(moved_file_full.c_str(), true))
        layer_file_full = moved_file_full;
      }

    // Return the file - no guarantee that it exists...
    return layer_file_full;
    }

  /**
   * Convert all layer paths in the workspace to actual paths - should be done
   * when saving a project
   */
  void SetAllLayerPathsToActualPaths()
  {
    for(int i = 0; i < this->GetNumberOfLayers(); i++)
      {
      Registry &folder = this->GetLayerFolder(i);
      folder["AbsolutePath"] << this->GetLayerActualPath(folder);
      }
  }

  /**
   * Get the IO hints for the layer, or NULL pointer if there are no IO hints stored
   */
  Registry * GetLayerIOHints(Registry &folder)
    {
    // Load the IO hints for the image from the project - but only if this
    // folder is actually present (otherwise some projects from before 2016
    // will not load hints)
    Registry *io_hints = NULL;
    if(folder.HasFolder("IOHints"))
      io_hints = &folder.Folder("IOHints");
    return io_hints;
    }

  void PrintLayerList(std::ostream &os, const string &line_prefix = "")
    {
    // Iterate over all the layers stored in the workspace
    int n_layers = this->GetNumberOfLayers();

    // Use a formatted table
    FormattedTable table(5);

    // Print the header information
    table << "Layer" << "Role" << "Nickname" << "Filename" << "Tags";

    // Load all of the layers in the current project
    for(int i = 0; i < n_layers; i++)
      {
      // Copy the folder corresponding to the layer - to avoid modifying the original
      Registry f_layer = this->GetLayerFolder(i);

      // Get the role 
      string role = f_layer["Role"][""];
      role = role.substr(0,role.find("Role"));

      // Use the %03d formatting for layer numbers, to match that in the registry
      char ifmt[16];
      sprintf(ifmt, "%03d", i);

      // Print the layer information
      table 
        << ifmt 
        << role 
        << f_layer["LayerMetaData.CustomNickName"][""] 
        << this->GetLayerActualPath(f_layer)
        << f_layer["Tags"][""];
      }

    table.Print(os, line_prefix);
    }

  /** List all layer files associated with a tag */
  void ListLayerFilesForTag(const string &tag, ostream &sout, const string &prefix)
    {
    // Iterate over all the layers stored in the workspace
    int n_layers = this->GetNumberOfLayers();

    // Load all of the layers in the current project
    for(int i = 0; i < n_layers; i++)
      {
      // Copy the folder corresponding to the layer - to avoid modifying the original
      Registry &f_layer = this->GetLayerFolder(i);

      // Get the tags for this folder
      set<string> tags = this->GetTags(f_layer);

      // Find the tag
      if(tags.find(tag) != tags.end())
        {
        // Print the filename of this layer
        sout << prefix << this->GetLayerActualPath(f_layer) << endl;
        }
      }
    }

  /** Get a list of tags from a particular folder */
  set<string> GetTags(Registry &folder)
    {
    set<string> tagset;
    if(folder.HasEntry("Tags"))
      {
      istringstream iss(folder["Tags"][""]);
      string tag;
      while(getline(iss,tag,','))
        tagset.insert(tag);
      }

    return tagset;
    }

  /** Put tags into a folder. Assumes that there are no commas in the tags */
  void PutTags(Registry &folder, const set<string> &tags)
    {
    ostringstream oss;
    for(set<string>::const_iterator it = tags.begin(); it != tags.end(); ++it)
      {
      if(it->find(',') != string::npos)
        throw IRISException("Commas are not allowed in tags. Offending tag: %s", it->c_str());

      if(it != tags.begin())
        oss << ",";
      oss << *it;
      }
    folder["Tags"] << oss.str();
    }

  /** Add a tag to a particular folder */
  void AddTag(Registry &folder, const string &newtag)
    {
    set<string> tags = this->GetTags(folder);
    tags.insert(newtag);
    this->PutTags(folder, tags);
    }

  /** Remove a tag from a folder */
  void RemoveTag(Registry &folder, const string &tag)
    {
    set<string> tags = this->GetTags(folder);
    tags.erase(tag);
    this->PutTags(folder, tags);
    }

  /** Remove given tag from all subfolders in the given folder */ 
  void ScrubTag(Registry &folder, const string &tag)
    {
    // Go over all subfolders of the given folder, recursively
    Registry::StringListType keys;
    folder.GetFolderKeys(keys);
    for(Registry::StringListType::const_iterator it = keys.begin(); it != keys.end(); ++it)
      this->ScrubTag(folder.Folder(*it), tag);

    // Do we have a tag entry
    if(folder.HasEntry("Tags"))
      this->RemoveTag(folder, tag);
    }

  /**
   * Recursive tag search
   */
  void FindTag(Registry &folder, const string &tag, list<string> &found_keys, const string &prefix)
    {
    // Go over all subfolders of the given folder, recursively
    Registry::StringListType keys;
    folder.GetFolderKeys(keys);
    for(Registry::StringListType::const_iterator it = keys.begin(); it != keys.end(); ++it)
      this->FindTag(folder.Folder(*it), tag, found_keys, (prefix.length() ? prefix + "." + *it : *it));

    // Do we have a tag entry
    if(folder.HasEntry("Tags"))
      {
      set<string> tags = this->GetTags(folder);
      if(tags.find(tag) != tags.end())
        {
        found_keys.push_back(prefix);
        }
      }
    }

  /** 
   * Find a folder for a given tag. This will crash if the tag is missing or 
   * if more than one object has the given tag
   */
  string FindFolderForUniqueTag(const string &tag)
    {
    list<string> found_keys;
    this->FindTag(m_Registry, tag, found_keys, "");
    if(found_keys.size() == 0)
      throw IRISException("No folders with tag %s found in the workspace", tag.c_str());
    else if(found_keys.size() > 1)
      throw IRISException("Multiple folders with tag %s found in the workspace", tag.c_str());

    return found_keys.back();
    }

  /**
   * Find layer by role. If pos_in_role is negative, this looks from
   * the end for that role
   */
  string FindLayerByRole(const string &role, int pos_in_role)
    {
    // Iterate over all the layers stored in the workspace
    int n_layers = this->GetNumberOfLayers();

    // Direction in which we traverse the layers
    int start = (pos_in_role >= 0) ? 0 : n_layers - 1;
    int end = (pos_in_role >= 0) ? n_layers : 0;
    int step = (pos_in_role >= 0) ? 1 : -1;

    // The role counter - how many have we seen for the specified role
    int role_count = (pos_in_role >= 0) ? 0 : 1;

    // Load all of the layers in the current project
    for(int i = start; i != end; i += step)
      {
      // Get the folder corresponding to the layer
      Registry &f_layer = this->GetLayerFolder(i);

      // Is this the correct role?
      string l_role = f_layer["Role"][""];
      if(l_role == role
        || (role == "AnatomicalRole" && (l_role == "MainRole" || l_role == "OverlayRole")) 
        || (role == "AnyRole"))
        {
        if(role_count == abs(pos_in_role))
          return Registry::Key("Layers.Layer[%03d]", i);
        else
          role_count++;
        }
      }

    // We got to the end and have not found this role. That's a problem. We will
    // return an empty string
    return string();
    }

  /** 
   * Translate a shorthand layer specifier to a folder ID. Will throw an exception if
   * the layer specifier cannot be found or is out of range
   */
  string LayerSpecToKey(const string &layer_spec)
    {
    // Basic pattern (001)
    RegularExpression reNum("^([0-9]+)$");

    // String:Number pattern (A:-2)
    RegularExpression reStrNum("^([a-zA-Z]+):(-*[0-9]+)$");

    // Test for the numerical pattern
    if(reNum.find(layer_spec.c_str()))
      {
      int layer_index = atoi(reNum.match(1).c_str());
      string key = Registry::Key("Layers.Layer[%03d]", layer_index);
      if(m_Registry.HasFolder(key))
        return key;
      }

    // Test for 'main' or 'M'
    else if(layer_spec == "M" || layer_spec == "m" || layer_spec == "main")
      {
      string key = this->FindLayerByRole("MainRole", 0);
      if(key.length() && m_Registry.HasFolder(key))
        return key;
      }

    // Test for 'seg' or 'S'
    else if(layer_spec == "S" || layer_spec == "s" || layer_spec == "seg")
      {
      string key = this->FindLayerByRole("SegmentationRole", 0);
      if(key.length() && m_Registry.HasFolder(key))
        return key;
      }

    // Test for 'overlay' or 'O'
    else if(reStrNum.find(layer_spec.c_str()))
      {
      string str_spec = reStrNum.match(1);
      int index = atoi(reStrNum.match(2).c_str());
      if(str_spec == "A" || str_spec == "a" || str_spec == "anat")
        {
        string key = this->FindLayerByRole("AnatomicalRole", index);
        if(key.length() && m_Registry.HasFolder(key))
          return key;
        }
      else if(str_spec == "O" || str_spec == "o" || str_spec == "overlay")
        {
        string key = this->FindLayerByRole("OverlayRole", index);
        if(key.length() && m_Registry.HasFolder(key))
          return key;
        }
      }

    throw IRISException("Layer specification %s not found in workspace", layer_spec.c_str());
    }

  /**
   * Get the folder id for the main image or throw exception if it does not exist
   */
  string GetMainLayerKey()
  {
    return this->LayerSpecToKey("M");
  }

  /**
   * Set the main layer dimensions in the registry. This should be called whenever the
   * main layer is assigned or changed
   */
  void UpdateMainLayerFieldsFromImage(Registry &main_layer_folder)
  {
    string filename = main_layer_folder["AbsolutePath"][""];

    // Try reading the file. This is not strictly required to create a workspace but without
    // setting the image dimensions, older versions of SNAP will refuse to read some metadata
    // from project files, which is a problem
    // TODO: there has to be a way to supply some hints!
    SmartPtr<GuidedNativeImageIO> io = GuidedNativeImageIO::New();
    Registry hints;
    io->ReadNativeImageHeader(filename.c_str(), hints);
    Vector3i dims(0);
    for(int k = 0; k < io->GetIOBase()->GetNumberOfDimensions(); k++)
      dims[k] = io->GetIOBase()->GetDimensions(k);

    main_layer_folder["ProjectMetaData.Files.Grey.Dimensions"] << dims;
  }

  /** Add a layer to the workspace in a given role */
  string AddLayer(string role, const string &filename)
    {
    // Validity checks

    // May not have another layer in the main role
    if((role == "MainRole") && this->FindLayerByRole(role, 0).length() > 0)
        throw IRISException("A workspace cannot have more than one image in the %s role", role.c_str());

    // Interpret the anatomical role
    if(role == "AnatomicalRole")
      role = this->FindLayerByRole("MainRole", 0).length() > 0 ? "OverlayRole" : "MainRole";

    // May not add anything until the main image has been added
    if(role != "MainRole" && this->FindLayerByRole("MainRole", 0).length() == 0)
      throw IRISException("Cannot add image in %s role to a workspace without main image", role.c_str());

    // Append a bare-bones folder
    string key = Registry::Key("Layers.Layer[%03d]", this->GetNumberOfLayers());

    // Create a folder for this key
    Registry &folder = m_Registry.Folder(key);

    // Add the filename and role
    folder["AbsolutePath"] << filename;
    folder["Role"] << role;

    // If the role is 'main' then we need to write the dimensions of the image into projectmetadata
    if(role == "MainRole")
      this->UpdateMainLayerFieldsFromImage(folder);

    return key;
    }

  /** Assign a layer to a specific role (main/seg) */
  string SetLayer(string role, const string &filename)
    {
    string key = this->FindLayerByRole(role, 0);

    // If this role does not already exist, we use the 'Add' functionality
    if(key.length() == 0)
      key = this->AddLayer(role, filename);

    // Otherwise, we trash the old folder associated with this role
    Registry &folder = m_Registry.Folder(key);
    folder.Clear();

    // Add the filename and role
    folder["AbsolutePath"] << filename;
    folder["Role"] << role;

    // If the role is 'main' then we need to write the dimensions of the image into projectmetadata
    if(role == "MainRole")
      this->UpdateMainLayerFieldsFromImage(folder);

    // Return the key
    return key;
    }

  /** Write a control point sequence to the folder for a contrast mapping */
  void WriteLayerContrastToRegistry(Registry &folder, int n, double *tarray, double *yarray)
    {
    folder.Clear();
    folder["NumberOfControlPoints"] << n;
    for(int i = 0; i < n; i++)
      {
      Registry &sub = folder.Folder(Registry::Key("ControlPoint[%d]",i));
      sub["tValue"] << tarray[i];
      sub["xValue"] << yarray[i];
      }
    }

  /** Set some property of the layer */
  template <class T>
  void SetLayerProperty(const string &layer_key, const string &prop_key, const T& value)
    {
    if(!layer_key.length() || !m_Registry.HasFolder(layer_key))
      throw IRISException("Layer folder %s does not exist", layer_key.c_str());

    m_Registry.Folder(layer_key)[prop_key] << value;
    }

  /** Set layer nickname */
  void SetLayerNickname(const string &layer_key, const string &value)
    {
    this->SetLayerProperty(layer_key, "LayerMetaData.CustomNickName", value);
    }

  /** Set layer nickname */
  void SetLayerColormapPreset(const string &layer_key, const string &value)
    {
    this->SetLayerProperty(layer_key, "LayerMetaData.DisplayMapping.ColorMap.Preset", value);
    }

  /** Set the contrast mapping for the selected layer */
  void SetLayerContrastToLinear(const string &layer_key, double t0, double t1)
    {
    if(!layer_key.length() || !m_Registry.HasFolder(layer_key))
      throw IRISException("Layer folder %s does not exist", layer_key.c_str());

    // Get the folder containing the curve
    Registry &layer_folder = m_Registry.Folder(layer_key);
    Registry &contrast_folder = layer_folder.Folder("LayerMetaData.DisplayMapping.Curve");
    contrast_folder.Clear();

    // Set up the control points
    double tarray[3], yarray[3];
    tarray[0] = t0;               yarray[0] = 0.0;
    tarray[1] = (t1 + t0) / 2.0;  yarray[1] = 0.5;
    tarray[2] = t1;               yarray[2] = 1.0;

    // Write to the folder
    this->WriteLayerContrastToRegistry(contrast_folder, 3, tarray, yarray);
    }

  /** Set labels from a label file */
  void SetLabels(const string &label_file)
    {
    // Get the main layer
    Registry &main = m_Registry.Folder(this->GetMainLayerKey());

    // Get the subfolder that corresponds to the labels
    Registry &label_reg = main.Folder("ProjectMetaData.IRIS.LabelTable");

    // Load the label descriptions
    SmartPtr<ColorLabelTable> clt = ColorLabelTable::New();
    clt->LoadFromFile(label_file.c_str());

    // Create a registry for the labels
    label_reg.Clear();
    clt->SaveToRegistry(label_reg);
    }

  /**
   * Add labels with an offset and a prefix/suffix.
   * Format of rename_pattern is "left %s" for example
   */
  void AddLabels(const string &label_file, int offset, const string &rename_pattern)
    {
    // Get the main layer
    Registry &main = m_Registry.Folder(this->GetMainLayerKey());

    // Read the existing labels
    SmartPtr<ColorLabelTable> clt = ColorLabelTable::New();
    clt->LoadFromRegistry(main.Folder("ProjectMetaData.IRIS.LabelTable"));

    // Load the additional labels
    SmartPtr<ColorLabelTable> delta = ColorLabelTable::New();
    delta->LoadFromFile(label_file.c_str());

    // Loop over the additional labels
    const ColorLabelTable::ValidLabelMap &valmap = delta->GetValidLabels();
    for(ColorLabelTable::ValidLabelConstIterator it = valmap.begin(); it != valmap.end(); it++)
      {
      if(it->first > 0)
        {
        ColorLabel cl = it->second;
        char buffer[1024];
        sprintf(buffer, rename_pattern.c_str(), cl.GetLabel());
        cl.SetLabel(buffer);
        clt->SetColorLabel(it->first + offset, cl);
        }
      }

    // Store the new labels
    clt->SaveToRegistry(main.Folder("ProjectMetaData.IRIS.LabelTable"));
    }

  /** Reset the labels - leaves only the clear label */
  void ClearLabels()
  {
    // Get the main layer
    Registry &main = m_Registry.Folder(this->GetMainLayerKey());

    // Read the existing labels
    SmartPtr<ColorLabelTable> clt = ColorLabelTable::New();
    clt->RemoveAllLabels();

    // Clear the label table
    clt->SaveToRegistry(main.Folder("ProjectMetaData.IRIS.LabelTable"));
  }

  /** Get the project registry by reference */
  const Registry &GetRegistry() const { return m_Registry; }

  /** Get the project registry by reference */
  Registry &GetRegistry() { return m_Registry; }

  /** Get a folder within the registry - this is just a helper */
  Registry &GetFolder(const string &key) { return m_Registry.Folder(key); }

  /** Get a folder within the registry - this is just a helper */
  bool HasFolder(const string &key) { return m_Registry.HasFolder(key); }

  /** Get the absolute path to the directory where the project was loaded from */
  string GetWorkspaceActualDirectory()  const { return m_WorkspaceFileDir; }


protected:

  // The Registry object containing workspace data
  Registry m_Registry;

  // Has the workspace moved from its original location
  bool m_Moved;

  // The full path and containing directory of the workspace file
  string m_WorkspaceFilePath, m_WorkspaceFileDir;

  // The directory where workspace was last saved
  string m_WorkspaceSavedDir;



};



/**
 * This class encapsulates the client side of the ALFABIS RESTful API.
 * It uses HTTP and CURL to communicate with the server
 */
class RESTClient
{
public:

  RESTClient()
    {
    m_Curl = curl_easy_init();
    m_UploadMessageBuffer[0] = 0;
    m_MessageBuffer[0] = 0; 
    m_OutputFile = NULL;
    }

  ~RESTClient()
    {
    curl_easy_cleanup(m_Curl);
    }

  void SetVerbose(bool verbose)
    {
    curl_easy_setopt(m_Curl, CURLOPT_VERBOSE, (long) verbose);
    }

  /** 
   * Set a FILE * to which to write the output of the Get/Post. This overrides
   * the default behaviour to capture the output in a string that can be accessed
   * with GetOutput(). This is useful for downloading binary files
   */
  void SetOutputFile(FILE *outfile)
    {
    m_OutputFile = outfile;
    }

  /**
   * Send the authentication data to the server and capture the cookie with the
   * session ID. The cookie and the server URL will be stored in a .alfabis directory
   * in user's home, so that subsequent calls do not require authentication
   */
  void Authenticate(const char *baseurl, const char *token)
    {
    // Create and perform the request
    ostringstream o_url; o_url << baseurl << "/api/login";
    curl_easy_setopt(m_Curl, CURLOPT_URL, o_url.str().c_str());

    // Data to post
    char post_buffer[1024];
    sprintf(post_buffer, "token=%s", token);
    curl_easy_setopt(m_Curl, CURLOPT_POSTFIELDS, post_buffer);

    // Cookie file
    string cookie_jar = this->GetCookieFile();
    curl_easy_setopt(m_Curl, CURLOPT_COOKIEJAR, cookie_jar.c_str());

    // Make request
    CURLcode res = curl_easy_perform(m_Curl);

    if(res != CURLE_OK)
      throw IRISException("CURL library error: %s", curl_easy_strerror(res));

    // Store the URL for the future
    ofstream f_url(this->GetServerURLFile().c_str());
    f_url << baseurl;
    f_url.close();
    }

  /** 
   * Basic GET command. Returns true if HTTP code of 200 is received.
   *  - The string output can be retrieved with GetOutput()
   *  - The error message for non 200 code can be retrieved with GetResponseText()
   */
  bool Get(const char *rel_url, ...)
    {
    // Handle the ...
    std::va_list args;
    va_start(args, rel_url);

    // Expand the URL
    char url_buffer[4096];
    vsprintf(url_buffer, rel_url, args);

    // Done with the ...
    va_end(args);

    // Use the same code as POST, but with null string
    return this->Post(url_buffer, NULL);
    }

  /** 
   * Basic POST command - give a relative URL and fields to send. Both the
   * rel_url and the post_string can have printf-like expressions, which are
   * evaluated in sequential order
   */
  bool Post(const char *rel_url, const char *post_string, ...)
    {
    // Handle the ...
    std::va_list args;
    va_start(args, post_string);

    // Expand the URL
    char url_buffer[4096];
    vsprintf(url_buffer, rel_url, args);

    // The URL to post to
    string url = this->GetServerURL() + "/" + url_buffer;
    curl_easy_setopt(m_Curl, CURLOPT_URL, url.c_str());

    // The cookie JAR
    string cookie_jar = this->GetCookieFile();
    curl_easy_setopt(m_Curl, CURLOPT_COOKIEFILE, cookie_jar.c_str());

    // The POST data
    if(post_string)
      {
      char post_buffer[4096];
      vsprintf(post_buffer, post_string, args);
      curl_easy_setopt(m_Curl, CURLOPT_POSTFIELDS, post_buffer);

      cout << "POST " << url << " VALUES " << post_buffer << endl;
      }

    // Done with ...
    va_end (args);

    // Capture output
    m_Output.clear();

    // If there is no output file, use the default callbacl
    if(!m_OutputFile)
      {
      curl_easy_setopt(m_Curl, CURLOPT_WRITEFUNCTION, RESTClient::WriteCallback);
      curl_easy_setopt(m_Curl, CURLOPT_WRITEDATA, &m_Output);
      }
    else
      {
      curl_easy_setopt(m_Curl, CURLOPT_WRITEFUNCTION, RESTClient::WriteToFileCallback);
      curl_easy_setopt(m_Curl, CURLOPT_WRITEDATA, m_OutputFile);
      }

    // Make request
    CURLcode res = curl_easy_perform(m_Curl);

    if(res != CURLE_OK)
      throw IRISException("CURL library error: %s", curl_easy_strerror(res));

    // Capture the response code
    m_HTTPCode = 0L;
    curl_easy_getinfo(m_Curl, CURLINFO_RESPONSE_CODE, &m_HTTPCode);

    // Get the code
    return m_HTTPCode == 200L;
    }

  bool UploadFile(const char *rel_url, const char *filename, 
    std::map<string,string> extra_fields, ...)
    {
    // Expand the URL
    std::va_list args;
    va_start(args, extra_fields);
    char url_buffer[4096];
    vsprintf(url_buffer, rel_url, args);

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
    struct curl_httppost *formpost=NULL;
    struct curl_httppost *lastptr=NULL;
    struct curl_slist *headerlist=NULL;
    static const char buf[] = "Expect:";

    /* Fill in the file upload field */ 
    curl_formadd(&formpost, &lastptr,
      CURLFORM_COPYNAME, "myfile",
      CURLFORM_FILE, fn_full_path.c_str(),
      CURLFORM_END);

    /* Fill in the filename field */ 
    curl_formadd(&formpost, &lastptr,
      CURLFORM_COPYNAME, "filename",
      CURLFORM_COPYCONTENTS, fn_name.c_str(),
      CURLFORM_END);

    /* Fill in the submit field too, even if this is rarely needed */ 
    curl_formadd(&formpost, &lastptr,
      CURLFORM_COPYNAME, "submit",
      CURLFORM_COPYCONTENTS, "send",
      CURLFORM_END);

    /* Add the extra form fields */
    for(std::map<string,string>::const_iterator it = extra_fields.begin();
      it != extra_fields.end(); ++it)
      {
      char post_buffer[4096];
      vsprintf(post_buffer, it->second.c_str(), args);

      curl_formadd(&formpost, &lastptr,
        CURLFORM_COPYNAME, it->first.c_str(),
        CURLFORM_COPYCONTENTS, post_buffer,
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
    
    // Make request
    CURLcode res = curl_easy_perform(m_Curl);

    if(res != CURLE_OK)
      throw IRISException("CURL library error: %s", curl_easy_strerror(res));

    // Get the upload statistics
    double upload_size, upload_time;
    curl_easy_getinfo(m_Curl, CURLINFO_SIZE_UPLOAD, &upload_size);
    curl_easy_getinfo(m_Curl, CURLINFO_TOTAL_TIME, &upload_time);
    sprintf( m_UploadMessageBuffer, "%.1f Mb in %.1f s", upload_size / 1.0e6, upload_time);

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

  const char *GetOutput()
    {
    return m_Output.c_str();
    }

  std::string GetFormattedCSVOutput(bool header)
    {
    FormattedTable ft;
    ft.ParseCSV(m_Output);
    ostringstream oss;
    ft.Print(oss);
    return oss.str();
    }

  const char *GetResponseText()
    {
    sprintf(m_MessageBuffer, "Response %ld, Text: %s", m_HTTPCode, m_Output.c_str());
    return m_MessageBuffer;
    }

  const char *GetUploadStatistics()
    {
    return m_UploadMessageBuffer;
    }

protected:

  /** The CURL handle */
  CURL *m_Curl;

  /** Optional file for output */
  FILE *m_OutputFile;

  /** Output stream */
  string m_Output;

  /** HTTP Code received */
  long m_HTTPCode;

  /** Message buffer */
  char m_MessageBuffer[1024];

  /** Upload message buffer */
  char m_UploadMessageBuffer[1024];

  string GetDataDirectory()
    {
    // Compute the platform-independent home directory
    vector<string> split_path;
    SystemTools::SplitPath("~/.alfabis",split_path,true);
    string ddir = SystemTools::JoinPath(split_path);
    SystemTools::MakeDirectory(ddir.c_str());
    return ddir;
    }

  string GetCookieFile() 
    {
    string cfile = this->GetDataDirectory() + "/cookie.jar";
    return SystemTools::ConvertToOutputPath(cfile);
    }

  string GetServerURLFile()
    {
    string sfile = this->GetDataDirectory() + "/server";
    return SystemTools::ConvertToOutputPath(sfile);
    }

  string GetServerURL()
    {
    string sfile = this->GetServerURLFile();
    if(!SystemTools::FileExists(sfile))
      throw IRISException("A server has not been configured yet - please sign in");
    try
      {
      string url;
      ifstream ifs(sfile.c_str());
      ifs >> url;
      ifs.close();
      return url;
      }
    catch(std::exception &exc)
      {
      throw IRISException("Failed to read server URL from %s, system exception: %s",
        sfile.c_str(), exc.what());
      }
    }

  static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
    {
    string *buffer = static_cast<string *>(userp);
    buffer->append((char *) contents, size * nmemb);
    return size * nmemb;
    }

  static size_t WriteToFileCallback(void *contents, size_t size, size_t nmemb, void *userp)
    {
    FILE *file = static_cast<FILE *>(userp);
    return fwrite(contents, size, nmemb, file);
    }

  static void dump(const char *text,
      FILE *stream, unsigned char *ptr, size_t size,
      char nohex)
      {
      size_t i;
      size_t c;

      unsigned int width=0x10;

      if(nohex)
        /* without the hex output, we can fit more on screen */ 
        width = 0x40;

      fprintf(stream, "%s, %10.10ld bytes (0x%8.8lx)\n",
        text, (long)size, (long)size);

      for(i=0; i<size; i+= width) {

        fprintf(stream, "%4.4lx: ", (long)i);

        if(!nohex) {
          /* hex not disabled, show it */ 
          for(c = 0; c < width; c++)
            if(i+c < size)
              fprintf(stream, "%02x ", ptr[i+c]);
            else
              fputs("   ", stream);
        }

        for(c = 0; (c < width) && (i+c < size); c++) {
          /* check for 0D0A; if found, skip past and start a new line of output */ 
          if(nohex && (i+c+1 < size) && ptr[i+c]==0x0D && ptr[i+c+1]==0x0A) {
            i+=(c+2-width);
            break;
          }
          fprintf(stream, "%c",
            (ptr[i+c]>=0x20) && (ptr[i+c]<0x80)?ptr[i+c]:'.');
          /* check again for 0D0A, to avoid an extra \n if it's at width */ 
          if(nohex && (i+c+2 < size) && ptr[i+c+1]==0x0D && ptr[i+c+2]==0x0A) {
            i+=(c+3-width);
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
  static int DebugCallback(CURL *handle, curl_infotype type,
    char *data, size_t size, void *userp)
    {
    FILE *trace_file = (FILE *) userp;
    const char *text;
    (void)handle; /* prevent compiler warning */ 

    switch(type) {
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

    dump(text, trace_file, (unsigned char *)data, size, true);
    return 0;
    }

};

/**
 * Export the workspace in a way such that all images are in the same directory as
 * the workspace and are in NIFTI format with generic filenames and no identifiers
 */
void ExportWorkspace(const Workspace &ws, const char *new_workspace)
{
  // Duplicate the workspace data
  Workspace wsexp = ws;

  // Get the directory where the new workspace will go
  string wsdir = SystemTools::GetParentDirectory(new_workspace);

  // Iterate over all the layers stored in the workspace
  int n_layers = wsexp.GetNumberOfLayers();

  // Load all of the layers in the current project
  for(int i = 0; i < n_layers; i++)
    {
    // Get the folder corresponding to the layer
    Registry &f_layer = wsexp.GetLayerFolder(i);

    // The the (possibly moved) absolute filename 
    string fn_layer = wsexp.GetLayerActualPath(f_layer);

    // The IO hints for the file
    Registry io_hints, *layer_io_hints;
    if((layer_io_hints = wsexp.GetLayerIOHints(f_layer)))
      io_hints.Update(*layer_io_hints);

    // Create a native image IO object for this image
    SmartPtr<GuidedNativeImageIO> io = GuidedNativeImageIO::New();

    // Load the header of the image and the image data
    io->ReadNativeImage(fn_layer.c_str(), io_hints);

    // Compute the hash of the image data to generate filename
    std::string image_md5 = io->GetNativeImageMD5Hash();

    // Create a filename that combines the layer index with the hash code
    char fn_layer_new[4096];
    sprintf(fn_layer_new, "%s/layer_%03d_%s.nii.gz", wsdir.c_str(), i, image_md5.c_str());

    // Save the layer there. Since we are saving as a NIFTI, we don't need to
    // provide any hints
    Registry dummy_hints;
    io->SaveNativeImage(fn_layer_new, dummy_hints);

    // Update the layer folder with the new path
    f_layer["AbsolutePath"] << fn_layer_new;

    // There are no hints necessary for NIFTI
    f_layer.Folder("IOHints").Clear();
    }

  // Write the updated project
  wsexp.SaveAsXMLFile(new_workspace);
}

/**
 * Cross-platform way of getting a temporary path
 */
string GetTempDirName()
{
#ifdef WIN32
  char tempDir[_MAX_PATH+1] = "";

  // We will try putting the executable in the system temp directory.
  // Note that the returned path already has a trailing slash.
  DWORD length = GetTempPath(_MAX_PATH+1, tempDir);
  if(length <= 0 || length > _MAX_PATH)
    throw IRISException("Unable to create temporary directory");

  return tempDir;

#else
  char tmp_template[4096];
  strcpy(tmp_template, "/tmp/alfabis_XXXXXX");
  string tmpdir = mkdtemp(tmp_template);
  return tmpdir;
#endif
}

/**
 * Export a workspace to a temporary directory and upload it
 */
void UploadWorkspace(const Workspace &ws,
                    const char *url,
                    int ticket_id,
                    const char *wsfile_suffix)
{
  // Create temporary directory for the export
  string tempdir = GetTempDirName();
  SystemTools::MakeDirectory(tempdir);

  // Export the workspace file to the temporary directory
  char ws_fname_buffer[4096];
  sprintf(ws_fname_buffer, "%s/ticket_%08d%s.itksnap", tempdir.c_str(), ticket_id, wsfile_suffix);
  ExportWorkspace(ws, ws_fname_buffer);

  cout << "Exported workspace to " << ws_fname_buffer << endl;

  // For each of the files in the directory upload it
  Directory dir;
  dir.Load(tempdir);
  for(int i = 0; i < dir.GetNumberOfFiles(); i++)
    {
    RESTClient rcu;
    const char *thisfile = dir.GetFile(i);
    string file_full_path = SystemTools::CollapseFullPath(thisfile, dir.GetPath());
    if(!SystemTools::FileIsDirectory(file_full_path))
      {
      // TODO: this is disgraceful!
      std::map<string, string> empty_map;
      if(!rcu.UploadFile(url, file_full_path.c_str(), empty_map, ticket_id))
        throw IRISException("Failed up upload file %s (%s)", file_full_path.c_str(), rcu.GetResponseText());

      cout << "Upload " << thisfile << " (" << rcu.GetUploadStatistics() << ")" << endl;
      }
    }

  // TODO: we should verify that all the files were successfully sent, via MD5
}

/**
 * Export a workspace to a temporary directory and use it to create a new ticket
 */
int CreateWorkspaceTicket(const Workspace &ws, const char *service_githash)
{
  // Create a new ticket
  RESTClient rc;
  if(!rc.Post("api/tickets","githash=%s", service_githash))
    throw IRISException("Failed to create new ticket (%s)", rc.GetResponseText());

  int ticket_id = atoi(rc.GetOutput());
      
  cout << "Created new ticket (" << ticket_id << ")" << endl;

  // Locally export and upload the workspace
  UploadWorkspace(ws, "api/tickets/%d/files/input", ticket_id, "");

  // Mark this ticket as ready
  if(!rc.Post("api/tickets/%d/status","status=ready", ticket_id))
    throw IRISException("Failed to mark ticket as ready (%s)", rc.GetResponseText());

  cout << "Changed ticket status to (" << rc.GetOutput() << ")" << endl;

  return ticket_id;
}

/**
 * Upload a workspace as the result for a ticket
 */
void UploadResultWorkspace(const Workspace &ws, int ticket_id)
{
  // Locally export and upload the workspace
  UploadWorkspace(ws, "api/pro/tickets/%d/files/results", ticket_id, "_results");
}

/**
 * Download ticket files to a directory. Flag provider_mode switches between
 * behavior for users and providers. String area is one of (input|results)
 */
string DownloadTicketFiles(int ticket_id, const char *outdir, bool provider_mode, const char *area)
{
  // Output string
  ostringstream oss;

  // Provider mode-specific settings
  const char *url_base = (provider_mode) ? "api/pro" : "api";

  // First off, get the list of all files for this ticket
  RESTClient rc;
  if(!rc.Get("%s/tickets/%d/files/%s", url_base, ticket_id, area))
    throw IRISException("Failed to get list of files for ticket %d (%s)", 
      ticket_id, rc.GetResponseText());

  // The output is in the form of a CSV, easiest to just parse it
  FormattedTable ft;
  ft.ParseCSV(rc.GetOutput());

  // Are there any files?
  if(ft.Rows() == 0 || ft.Columns() < 2)
    throw IRISException("Empty or invalid list of files for ticket %d", ticket_id);

  // Create the output directory
  if(!SystemTools::MakeDirectory(outdir))
    throw IRISException("Unable to create output directory %s", outdir);

  // Iterate over the dictionary
  for(int iFile = 0; iFile < ft.Rows(); iFile++)
    {
    // Where we will write this file to
    int file_index = atoi(ft(iFile,0).c_str());
    string file_name = ft(iFile, 1);
    string file_path = SystemTools::CollapseFullPath(file_name.c_str(), outdir);

    // Create a file handle
    FILE *fout = fopen(file_path.c_str(), "wb");
    rc.SetOutputFile(fout);

    if(!rc.Get("%s/tickets/%d/files/%s/%d", url_base, ticket_id, area, file_index))
      throw IRISException("Failed to download file %s for ticket %d (%s)", 
        file_name.c_str(), ticket_id, rc.GetResponseText());

    rc.SetOutputFile(NULL);
    fclose(fout);

    oss << file_path << endl;
    }

  return oss.str();
}

void PostAttachment(int ticket_id, string desc, string filename, string mimetype = "")
{
  RESTClient rcu;
  std::map<string, string> extra_param;
  extra_param["desc"] = desc;
  if(mimetype.size())
    extra_param["mime_type"] = mimetype;
  if(!rcu.UploadFile("api/pro/tickets/%d/attachments",
                     filename.c_str(), extra_param, ticket_id))
    throw IRISException("Error uploading attachment %s on ticket %d: %s",
                        ticket_id, filename.c_str(), rcu.GetResponseText());
}

void PostLogMessage(int ticket_id, string category, string message)
{
  RESTClient rc;
  if(!rc.Post("api/pro/tickets/%d/%s","message=%s",
              ticket_id, category.c_str(), message.c_str()))
    throw IRISException("Error posting message on ticket %d: %s",
                        ticket_id, rc.GetResponseText());
}

void print_string_with_prefix(ostream &sout, const string &text, const string &prefix)
{
  istringstream iss(text);
  string line, word;
  while(getline(iss, line))
    sout << prefix << line << endl;
}

/** 
 * Print ticket log with attachments and nice formatting
 */
int PrintTicketLog(int ticket_id, int id_start = 0)
{
  RESTClient rc;

  // Check for new log updates
  if(!rc.Get("api/tickets/%d/log?since=%d", ticket_id, id_start))
    throw IRISException("Error getting log for ticket %d: %s", ticket_id, rc.GetResponseText());

  // Format as a table
  FormattedTable ft;
  ft.ParseCSV(rc.GetOutput());

  // Create the filter for printing log rows
  if(ft.Rows())
    {
    vector<bool> col_filter(ft.Columns(), true);
    col_filter[0] = false; // log id
    col_filter[3] = false; // num_attach

    // Process each row of the table
    for(int i = 0; i < ft.Rows(); i++)
      {
      // TODO: we are hard-coding row meanings, which is not very smart in the client-server setting

      // Get the latest log_id for return value
      id_start = atoi(ft(i, 0).c_str());

      // Get the number of attachments
      int n_attach = atoi(ft(i, 3).c_str());

      // Print the row
      ft.PrintRow(cout, i, "", col_filter);

      // Process the attachments
      if(n_attach > 0)
        {
        RESTClient rca;
        if(!rca.Get("api/tickets/logs/%d/attachments", id_start))
          throw IRISException("Error getting attachment for ticket %d: %s", ticket_id, rca.GetResponseText());

        FormattedTable fta;
        fta.ParseCSV(rca.GetOutput());

        for(int k = 0; k < fta.Rows(); k++)
          {
          printf("  @ %s : %s\n", fta(k, 3).c_str(), fta(k, 1).c_str());
          }
        }
      }
    }

  return id_start;
} 


int main(int argc, char *argv[])
{
  // There must be some commands!
  if(argc < 2)
    return usage(-1);

  // Command line parsing helper
  CommandLineHelper cl(argc, argv);

  // Current workspace object
  Workspace ws;

  // Currently selected layer folder
  string layer_folder;

  // Index of the currently executed command
  int cmd_index = 1;

  // Whether printing of prefix has been disabled
  bool prefix_disabled = false;

  // Temporary prefix - only useful for a single command
  string temp_prefix;

  // The command index for which the temporary prefix should be used
  int temp_prefix_cmd_index = -1;

  // Context ticket id, i.e., the ticket id substituted for '--' in the input
  // TODO: implement this
  int context_ticket_id;

  // Parse the commands in order
  while(!cl.is_at_end())
    {
    // Come up with a default prefix for the current command
    string prefix = (cmd_index == temp_prefix_cmd_index) ? temp_prefix : 
      (prefix_disabled ? "" : Registry::Key("%d> ", cmd_index));

    // The argument
    string arg;

    try 
      {
      // Read the next command
      arg = cl.read_command();

      // Handle the various commands
      
      // Read a workspace
      if(arg == "-i")
        {
        ws.ReadFromXMLFile(cl.read_existing_filename().c_str());
        }

      else if(arg == "-o")
        {
        ws.SaveAsXMLFile(cl.read_output_filename().c_str());
        }

      // Archive the current workspace build
      else if(arg == "-a")
        {
        // Create an archive
        ExportWorkspace(ws, cl.read_output_filename().c_str());
        }

      // Prefix
      else if(arg == "-p" || arg == "-prefix")
        {
        temp_prefix = cl.read_string();
        temp_prefix_cmd_index = cmd_index + 1;
        }

      // Disable prefix printing
      else if(arg == "-P")
        {
        prefix_disabled = true;
        }

      // Dump the workspace contents
      else if(arg == "-dump")
        {
        ws.GetRegistry().Print(cout, "  ", prefix);
        }

      // List all layers
      else if(arg == "-layers-list" || arg == "-ll")
        {
        ws.PrintLayerList(cout, prefix);
        }

      // List the files associated with a specific tag
      else if(arg == "-layers-list-files" || arg == "-llf")
        {
        ws.ListLayerFilesForTag(cl.read_string(), cout, prefix);
        }

      // Select a layer - the selected layer is target for various property commands
      else if(arg == "-layers-pick" || arg == "-lp")
        {
        string layer_id = cl.read_string();
        layer_folder = ws.LayerSpecToKey(layer_id.c_str());
        cout << "INFO: picked layer " << layer_folder << endl;
        }

      else if(arg == "-layers-pick-by-tag" || arg == "-lpt" || arg == "-lpbt")
        {
        string tag = cl.read_string();
        layer_folder = ws.FindFolderForUniqueTag(tag);
        if(!ws.IsKeyValidLayer(layer_folder))
          throw IRISException("Folder %s with tag %s does not contain a valid layer", layer_folder.c_str(), tag.c_str());
        cout << "INFO: picked layer " << layer_folder << endl;
        }

      // Add a layer - the layer will be added in the anatomical role
      else if(arg == "-layers-add-anat" || arg == "-laa")
        {
        string filename = cl.read_existing_filename();
        string key = ws.AddLayer("AnatomicalRole", filename.c_str());
        layer_folder = key;
        cout << "INFO: picked layer " << layer_folder << endl;
        }

      // Add a layer - the layer will be added in the segmentation role
      else if(arg == "-layers-add-seg" || arg == "-las")
        {
        string filename = cl.read_existing_filename();
        string key = ws.AddLayer("SegmentationRole", filename.c_str());
        layer_folder = key;
        cout << "INFO: picked layer " << layer_folder << endl;
        }

      // Set the main layer
      else if(arg == "-layers-set-main" || arg == "-ls-main" || arg == "-lsm")
        {
        string filename = cl.read_existing_filename();
        string key = ws.SetLayer("MainRole", filename.c_str());
        layer_folder = key;
        cout << "INFO: picked layer " << layer_folder << endl;
        }

      // Set the main layer
      else if(arg == "-layers-set-seg" || arg == "-ls-seg" || arg == "-lss")
        {
        string filename = cl.read_existing_filename();
        string key = ws.SetLayer("SegmentationRole", filename.c_str());
        layer_folder = key;
        cout << "INFO: picked layer " << layer_folder << endl;
        }

      else if(arg == "-props-get-filename" || arg == "-pgf")
        {
        if(!ws.IsKeyValidLayer(layer_folder))
          throw IRISException("Selected object %s is not a valid layer", layer_folder.c_str());

        cout << prefix << ws.GetLayerActualPath(ws.GetFolder(layer_folder)) << endl;
        }

      // Set layer properties
      else if(arg == "-props-set-nickname" || arg == "-psn")
        {
        ws.SetLayerNickname(layer_folder, cl.read_string());
        }

      else if(arg == "-props-set-colormap")
        {
        ws.SetLayerColormapPreset(layer_folder, cl.read_string());
        }

      else if(arg == "-props-set-contrast")
        {
        string mode = cl.read_string();
        if(mode == "LINEAR" || mode == "L")
          {
          double t0 = cl.read_double();
          double t1 = cl.read_double();
          ws.SetLayerContrastToLinear(layer_folder, t0, t1);
          }
        }

      else if(arg == "-labels-set")
        {
        ws.SetLabels(cl.read_existing_filename());
        }

      else if(arg == "-labels-add")
        {
        string fn = cl.read_existing_filename();
        int offset = cl.command_arg_count() > 0 ? cl.read_integer() : 0;
        string rename_pattern = cl.command_arg_count() > 0 ? cl.read_string() : "%s";
        ws.AddLabels(fn, offset, rename_pattern);
        }

      else if (arg == "-labels-clear")
        {
        ws.ClearLabels();
        }

      else if(arg == "-tags-add" || arg == "-ta")
        {
        string tag = cl.read_string();
        ws.AddTag(ws.GetRegistry().Folder(layer_folder), tag);
        }
      else if(arg == "-tags-add-exclusive" || arg == "-tax")
        {
        string tag = cl.read_string();
        ws.ScrubTag(ws.GetRegistry(), tag);
        ws.AddTag(ws.GetRegistry().Folder(layer_folder), tag);
        }
      else if(arg == "-tags-delete" || arg == "-td")
        {
        string tag = cl.read_string();
        ws.RemoveTag(ws.GetRegistry().Folder(layer_folder), tag);
        }
      else if(arg == "-dss-auth")
        {
        // Read the url of the server
        string url = cl.read_string();

        // Tell the user where to go
        string token_string;
        printf("Paste this link into your browser to obtain a token:  %s/token\n", url.c_str());
        printf("  Enter the token: ");
        std::cin >> token_string;

        // Authenticate with the token
        RESTClient rc;
        rc.Authenticate(url.c_str(), token_string.c_str());
        }
      else if(arg == "-dss-services-list")
        {
        RESTClient rc;
        if(rc.Get("api/services"))
          print_string_with_prefix(cout, rc.GetFormattedCSVOutput(false), prefix);
        else
          throw IRISException("Error listing services: %s", rc.GetResponseText());
        }
      else if(arg == "-dss-tickets-create" || arg == "-dss-ticket-create")
        {
        string service_githash = cl.read_string();
        int ticket_id = CreateWorkspaceTicket(ws, service_githash.c_str());
        cout << prefix << ticket_id << endl;
        }
      else if(arg == "-dss-tickets-list" || arg == "-dtl")
        {
        RESTClient rc;
        if(rc.Get("api/tickets"))
          print_string_with_prefix(cout, rc.GetFormattedCSVOutput(false), prefix);
        else
          throw IRISException("Error listing tickets: %s", rc.GetResponseText());
        }
      else if(arg == "-dss-tickets-log" || arg == "-dt-log")
        {
        int ticket_id = cl.read_integer();
        PrintTicketLog(ticket_id);
        }
      else if(arg == "-dss-tickets-progress")
        {
        int ticket_id = cl.read_integer();
        RESTClient rc;
        if(rc.Get("api/tickets/%d/progress", ticket_id))
          cout << prefix << rc.GetOutput() << endl;
        else
          throw IRISException("Error getting progress for ticket %d: %s", ticket_id, rc.GetResponseText());
        }
      else if(arg == "-dss-tickets-wait")
        {
        // Takes a ticket ID and timeout in seconds
        int ticket_id = cl.read_integer();
        int timeout = cl.command_arg_count() > 0 ? cl.read_integer() : 10000;

        // Loop
        int tnow = 0, tprogress = 0;

        // Current progress 
        double p = 0;
        RESTClient rc;

        // The ID of the last log entry
        int last_log_id = 0;

        // The queue position
        int queue_pos = 0;

        // The status of the ticket
        string status;

        while(tnow < timeout)
          {
          // Go to the begin of line - to erase the current progress
          printf("\r");

          // Shoud we update the progress?
          if(tnow - tprogress > 5 || tprogress == 0)
            {
            // Check status
            if(!rc.Get("api/tickets/%d/status", ticket_id))
              throw IRISException("Error getting progress for ticket %d: %s", ticket_id, rc.GetResponseText());
            status = rc.GetOutput();

            // If the status is 'claimed', the ticket is in progress and we can show progress
            // Likewise for success/fail, we want to show the user the progress
            if(status == "claimed" || status == "success" || status == "failed")
              {
              // Check progress
              if(!rc.Get("api/tickets/%d/progress", ticket_id))
                throw IRISException("Error getting progress for ticket %d: %s", ticket_id, rc.GetResponseText());
              string p_string = rc.GetOutput();
              p = atof(p_string.c_str());
              }

            // The status is claimed or later - so we can read the log
            if(status != "init" && status != "ready")
              {
              // Print the new ticket log if any
              last_log_id = PrintTicketLog(ticket_id, last_log_id);

              // We are not in the queue
              queue_pos = 0;
              }

            else if(status == "ready")
              {
              // Get the queue position of the ticket
              if(!rc.Get("api/tickets/%d/queuepos", ticket_id))
                throw IRISException("Error getting queue position for ticket %d: %s", ticket_id, rc.GetResponseText());

              // Get the queue position
              queue_pos = atoi(rc.GetOutput());
              }
            }

          // If we are in the queue, print queue position
          if(queue_pos > 0)
            {
            printf("Ticket in queue position %d ... ", queue_pos);
            }
          else
            {
            // Display progress nicely
            for(int i = 0; i < 78; i++)
              if(i <=  p * 78)
                printf("#");
              else
                printf(" ");
            printf(" %3d%% ", (int) (100 * p));
            }

          // If terminal status, exit
          if(status == "failed" || status == "success" || status == "timeout")
            {
            printf("\n");
            break;
            }

          const char blop[] = "|/-\\";
            printf("%c", blop[tnow % 4]);

          fflush(stdout);

          // Wait
          sleep(1);
          tnow ++;
          }

        // Print additional information
        if(tnow < timeout)
          printf("\nTicket completed with status: %s\n", status.c_str());
        else
          printf("\nTimed out\n");
        }
      else if(arg == "-dssp-services-list")
        {
        RESTClient rc;
        if(rc.Get("api/pro/services"))
          print_string_with_prefix(cout, rc.GetFormattedCSVOutput(false), prefix);
        else
          throw IRISException("Error listing services: %s", rc.GetResponseText());
        }
      else if(arg == "-dssp-services-claim")
        {
        string service_githash = cl.read_string();
        string provider_name = cl.read_string();
        string provider_code = cl.read_string();
        long timeout = cl.command_arg_count() > 0 ? cl.read_integer() : 0L;
        long tnow = 0, twait = 30;
        
        while(true)
          {
          // Try claiming the ticket
          RESTClient rc;
          if(!rc.Post("api/pro/services/%s/claims","provider=%s,code=%s",
                      service_githash.c_str(), provider_name.c_str(), provider_code.c_str()))
            throw IRISException("Error claiming ticket for service %s: %s", 
              service_githash.c_str(), rc.GetResponseText());

          // Ticket id will be 0 if nothing is available, or the ticket id
          int ticket_id = atoi(rc.GetOutput());

          // If actual ticket, print it and we are done
          if(ticket_id > 0) 
            {
            cout << prefix << ticket_id << endl;
            context_ticket_id = ticket_id;
            break;
            }
          else if(tnow + twait > timeout)
            {
            throw IRISException("No tickets available to claim for service %s", service_githash.c_str());
            }
          else
            {
            tnow += 30;
            sleep(30);
            }
          }
        }
      else if(arg == "-dss-tickets-download")
        {
        int ticket_id = cl.read_integer();
        string output_path = cl.read_string();
        string file_list = DownloadTicketFiles(ticket_id, output_path.c_str(), false, "results");
        print_string_with_prefix(cout, file_list, prefix);
        }
      else if(arg == "-dssp-tickets-download")
        {
        int ticket_id = cl.read_integer();
        string output_path = cl.read_string();
        string file_list = DownloadTicketFiles(ticket_id, output_path.c_str(), true, "input");
        print_string_with_prefix(cout, file_list, prefix);
        }
      else if(arg == "-dssp-tickets-fail")
        {
        int ticket_id = cl.read_integer();
        string message = cl.read_string();

        // Set the status of the ticket
        PostLogMessage(ticket_id, "error", message);

        // Change the status of the ticket to failed
        RESTClient rc;
        if (rc.Post("api/pro/tickets/%d/status","status=failed", ticket_id))
          {
          cout << prefix << rc.GetOutput() << endl;
          }
        else
          throw IRISException("Error marking ticket %d as failed: %s", 
            ticket_id, rc.GetResponseText());
        }
      else if(arg == "-dssp-tickets-success")
        {
        int ticket_id = cl.read_integer();

        RESTClient rc;
        if (rc.Post("api/pro/tickets/%d/status","status=success", ticket_id))
          {
          cout << prefix << rc.GetOutput() << endl;
          }
        else
          throw IRISException("Error marking ticket %d as completed: %s", 
            ticket_id, rc.GetResponseText());
        }
      else if(arg == "-dssp-tickets-set-progress")
        {
        int ticket_id = cl.read_integer();
        RESTClient rc;
        double chunk_start = cl.read_double();
        double chunk_end = cl.read_double();
        double chunk_prog = cl.read_double();
        if(rc.Post("api/pro/tickets/%d/progress","chunk_start=%f&chunk_end=%f&progress=%f", 
            ticket_id, chunk_start, chunk_end, chunk_prog))
          cout << rc.GetOutput() << endl;
        else
          throw IRISException("Error setting progress for ticket %d: %s", 
            ticket_id, rc.GetResponseText());
        }
      else if(arg == "-dssp-tickets-attach")
        {
        int ticket_id = cl.read_integer();
        string desc = cl.read_string();
        string filename = cl.read_existing_filename();
        string mimetype = cl.command_arg_count() > 0 ? cl.read_string() : "";
        PostAttachment(ticket_id, desc, filename, mimetype);
        }
      else if(arg == "-dssp-tickets-log")
        {
        // Post the log message
        int ticket = cl.read_integer();
        std::string type = cl.read_string();
        std::string message = cl.read_string();
        PostLogMessage(ticket,type,message);
        }
      else if(arg == "-dssp-tickets-upload")
        {
        // Upload the workspace as the result
        UploadResultWorkspace(ws, cl.read_integer());
        }
      else
        throw IRISException("Unknown command %s", arg.c_str());
      }
    catch(IRISException &exc)
      {
      cerr << "ITK-SNAP exception for command " << arg << " : " << exc.what() << endl;
      return -1;
      }
    catch(std::exception &sexc)
      {
      cerr << "System exception for command " << arg << " : " << sexc.what() << endl;
      return -1;
      }

    // Increment the command index
    cmd_index++;
    }

  return 0;
}
