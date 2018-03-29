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

#include "CSVParser.h"
#include "WorkspaceAPI.h"
#include "FormattedTable.h"
#include "RESTClient.h"

#include "CommandLineHelper.h"
#include "GuidedNativeImageIO.h"
#include "itksys/MD5.h"
#include "itksys/Directory.hxx"
#include "itksys/RegularExpression.hxx"
#include "IRISException.h"
#include "ColorLabelTable.h"

#include "IRISApplication.h"
#include "AffineTransformHelper.h"
#include "itkTransform.h"

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
  cout << "  -registry-get <key>               : Get the value of a specified key" << endl;
  cout << "  -registry-set <key> <value>       : Set the value of a specified key" << endl;
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
  cout << "  -props-get-transform              : Print the affine transform relative to main image" << endl;
  cout << "  -props-rename-file                : Rename the picked layer on disk and in workspace" << endl;
  cout << "  -props-set-nickname <name>        : Set the nickname of the selected layer" << endl;
  cout << "  -props-set-colormap <preset>      : Set colormap to a given system preset" << endl;
  cout << "  -props-set-contrast <map_spec>    : Set the contrast mapping specification" << endl;
  cout << "  -props-set-sticky <on|off>        : Set the stickiness of the layer" << endl;
  cout << "  -props-set-alpha <value>          : Set the alpha (transparency) of the layer" << endl;
  cout << "  -props-registry-get <key>         : Gets a registry key relative to picked layer" << endl;
  cout << "  -props-registry-set <key> <value> : Sets a registry key relative to picked layer" << endl;
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
  cout << "  -dss-services-detail <service_id> : Get detailed description of a service" << endl;
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




/*
{
  ft.Print(oss);
  return oss;
}*/







/**
 * Export the workspace in a way such that all images are in the same directory as
 * the workspace and are in NIFTI format with generic filenames and no identifiers
 */
void ExportWorkspace(const WorkspaceAPI &ws, const char *new_workspace)
{
  // Duplicate the workspace data
  WorkspaceAPI wsexp = ws;

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
void UploadWorkspace(const WorkspaceAPI &ws,
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
int CreateWorkspaceTicket(const WorkspaceAPI &ws, const char *service_githash)
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
void UploadResultWorkspace(const WorkspaceAPI &ws, int ticket_id)
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
  WorkspaceAPI ws;

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

      else if(arg == "-registry-get")
        {
        string key = cl.read_string();
        cout << prefix << ws.GetRegistry()[key][""] << endl;
        }

      else if(arg == "-registry-set")
        {
        string key = cl.read_string();
        string value = cl.read_string();
        ws.GetRegistry()[key] << value;
        cout << "INFO: set registry entry '" << key << "' to '" << ws.GetRegistry()[key][""] << "'" << endl;
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

      else if(arg == "-props-registry-get" || arg == "-prg")
        {
        if(!ws.IsKeyValidLayer(layer_folder))
          throw IRISException("Selected object %s is not a valid layer", layer_folder.c_str());

        string key = cl.read_string();
        cout << prefix << ws.GetRegistry().Folder(layer_folder)[key][""] << endl;
        }

      else if(arg == "-props-registry-set" || arg == "-prs")
        {
        if(!ws.IsKeyValidLayer(layer_folder))
          throw IRISException("Selected object %s is not a valid layer", layer_folder.c_str());

        string key = cl.read_string();
        string value = cl.read_string();
        ws.GetRegistry().Folder(layer_folder)[key] << value;
        cout << "INFO: set registry entry '" << key << "' to '" << ws.GetRegistry().Folder(layer_folder)[key][""] << "'" << endl;
        }

      else if(arg == "-props-rename-file" || arg == "-prf")
        {
        string new_filename = cl.read_output_filename();

        if(!ws.IsKeyValidLayer(layer_folder))
          throw IRISException("Selected object %s is not a valid layer", layer_folder.c_str());

        ws.RenameLayer(layer_folder, new_filename, true);
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

      else if(arg == "-props-get-transform")
        {
        if(!ws.IsKeyValidLayer(layer_folder))
          throw IRISException("Selected object %s is not a valid layer", layer_folder.c_str());

        // Get the folder
        Registry &folder = ws.GetRegistry().Folder(layer_folder);

        // Read the transform from the registry
        SmartPtr<AffineTransformHelper::ITKTransformBase> tran =
            AffineTransformHelper::ReadFromRegistry(&folder);

        // Read the transform
        AffineTransformHelper::Mat44 Q = AffineTransformHelper::GetRASMatrix(tran);

        // Generate a matrix from the transform
        cout << prefix << Q << endl;
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
      else if(arg == "-dss-services-detail")
        {
        string service_githash = cl.read_string();
        RESTClient rc;
        if(rc.Get("api/services/%s/detail", service_githash.c_str()))
          print_string_with_prefix(cout, rc.GetOutput(), prefix);
        else
          throw IRISException("Error getting service detail: %s", rc.GetResponseText());

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
