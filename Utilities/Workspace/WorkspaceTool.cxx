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
#include <fstream>
#include <string>
#include "CommandLineHelper.h"
#include "Registry.h"

using namespace std;

int usage(int rc) 
{
  cout << "itksnap_ws : ITK-SNAP Workspace Tool" << endl;
  cout << "usage: " << endl;
  cout << "  itksnap_ws [commands]" << endl;
  cout << "commands: " << endl;
  cout << "  -i <workspace>                    : Read workspace file" << endl;
  cout << "  -a <outfile.zip>                  : Package workspace into an archive" << endl;
  cout << "  -I                                : Summarize workspace file" << endl;
  cout << "  -t <tag> <objref>                 : Assign a tag to the referenced object" << endl;
  return rc;
}

#ifdef _GZOOTY_
class Workspace : public Registry
{
public:

  /**
   * Read the workspace from a file
   */
  void ReadFromXMLFile(const char *proj_file)
    {
    // Read the contents of the project from the file
    Registry::ReadFromXMLFile(proj_file);

    // Get the full name of the project file
    m_WorkspaceFilePath = itksys::SystemTools::CollapseFullPath(proj_file);

    // Get the directory in which the project will be saved
    m_WorkspaceFileDir = itksys::SystemTools::GetParentDirectory(m_WorkspaceFilePath);

    // Read the location where the file was saved initially
    m_WorkspaceSavedDir = (*this)["SaveLocation"][""];

    // If the locations are different, we will attempt to find relative paths first
    m_Moved = (m_WorkspaceFileDir != m_WorkspaceSavedDir);
    }

  /**
   * Find a physical file corresponding to a file referenced by the project, accouting
   * for the possibility that the project may have been moved or copied
   */
  string FindFile(Registry &folder)
    {
    // Get the filenames for the layer
    string layer_file_full = folder["AbsolutePath"][""];

    // If the project has moved, try finding a relative location
    if(m_Moved)
      {
      string relative_path = itksys::SystemTools::RelativePath(
            m_WorkspaceSavedDir.c_str(), layer_file_full.c_str());

      string moved_file_full = itksys::SystemTools::CollapseFullPath(
            relative_path.c_str(), m_WorkspaceFileDir.c_str());

      if(itksys::SystemTools::FileExists(moved_file_full.c_str(), true))
        layer_file_full = moved_file_full;
      }

    // Return the file - no guarantee that it exists...
    return layer_file_full;
    }

  /**
   * Export the workspace in a way such that all images are in the same directory as
   * the workspace and are in NIFTI format with generic filenames and no identifiers
   */
  void ExportWorkspace(const char *new_project)
    {
    // Make a copy of the current project
    Registry rnew = (*this);

    // Load all of the layers in the current project
    // Read all the layers
    std::string key;
    bool main_loaded = false;
    for(int i = 0; rnew.HasFolder(key = Registry::Key("Layers.Layer[%03d]", i)); i++)
      {
      // Get the key for the next image
      Registry &folder = rnew.Folder(key);

      // Read the role
      LayerRole role = folder["Role"].GetEnum(
        SNAPRegistryIO::GetEnumMapLayerRole(), NO_ROLE);

      // Get the actual filename
      string layer_file_full = this->FindFile(folder);

      // Load the IO hints for the image from the project - but only if this
      // folder is actually present (otherwise some projects from before 2016
      // will not load hints)
      Registry *io_hints = NULL;
      if(folder.HasFolder("IOHints"))
        io_hints = &folder.Folder("IOHints");

      // Load the image and its metadata
      LoadImage(layer_file_full.c_str(), role, warn, &folder, io_hints);

    }

  /**
   * Query the location of an image layer in the workspace file
   */
  

protected:

  // Has the workspace moved from its original location
  bool m_Moved;

  // The full path and containing directory of the workspace file
  string m_WorkspaceFilePath, m_WorkspaceFileDir;

  // The directory where workspace was last saved
  string m_WorkspaceSavedDir;



};

#endif

#include "IRISApplication.h"
#include "ImageIODelegates.h"

int main(int argc, char *argv[])
{
  // There must be some commands!
  if(argc < 2)
    return usage(-1);

  CommandLineHelper cl(argc, argv);

  // IRIS application - the main API for ITK-SNAP (minus the GUI)
  IRISApplication *app;

  // Parse the commands in order
  while(!cl.is_at_end())
    {
    try 
      {
      // Read the next command
      std::string arg = cl.read_command();

      // Handle the various commands
      if(arg == "-i")
        {
        IRISWarningList warnings;
        app->OpenProject(cl.read_existing_filename(), warnings);
        if(warnings.size())
          {
          cerr << "Reading workspace file generated WARNINGS:" << endl;
          for(int i = 0; i < warnings.size(); i++)
            cerr << warnings[i].what() << endl;
          }
        }
      }
    catch(IRISException &exc)
      {
      cerr << "ITK-SNAP exception: " << exc.what() << endl;
      return -1;
      }
    catch(std::exception &sexc)
      {
      cerr << "System exception: " << sexc.what() << endl;
      }
    }


}
