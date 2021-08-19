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
#include "itkMatrixOffsetTransformBase.h"
#include "IRISException.h"
#include "ColorLabelTable.h"

#include "IRISApplication.h"
#include "AffineTransformHelper.h"
#include "itkTransform.h"

#include "json/json.h"


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
  cout << "  -props-set-mcd <mcd_spec>         : Set the multi-component display mode (see below)" << endl;
  cout << "  -props-set-sticky <on|off>        : Set the stickiness of the layer" << endl;
  cout << "  -props-set-alpha <value>          : Set the alpha (transparency) of the layer" << endl;
  cout << "  -props-set-transform <file>       : Set the transform relative to main image" << endl;
  cout << "  -props-registry-get <key>         : Gets a registry key relative to picked layer" << endl;
  cout << "  -props-registry-set <key> <value> : Sets a registry key relative to picked layer" << endl;
  cout << "  -props-registry-dump <key> <file> : Dump registry folder relative to picked layer" << endl;
  cout << "  -props-registry-load <key> <file> : Load registry folder relative to picked layer" << endl;
  cout << "Tag assignment commands (apply to picked layer/object): " << endl;
  cout << "  -tags-add <tag>                   : Add a tag to the picked object" << endl;
  cout << "  -tags-add-excl <tag>              : Add a tag that is exclusive to the object" << endl;
  cout << "  -tags-del <tag>                   : Remove tag from the object" << endl;
  cout << "Segmentation label commands" << endl;
  cout << "  -labels-set <file>                : Read the labels from the labelfile specified" << endl;
  cout << "  -labels-clear                     : Remove all labels except the default clear label" << endl;
  cout << "  -labels-add <file> [offst] [ptrn] : Add labels from file, optionally shifting by offset and" << endl;
  cout << "                                      renaming with C printf pattern (e.g. 'left %s')" << endl;
  cout << "Annotation object commands" << endl;
  cout << "  -annot-list                       : List all annotations in the workspace" << endl;
  cout << "Distributed segmentation server (DSS) user commands: " << endl;
  cout << "  -dss-auth <url> [user] [passwd]   : Sign in to the server. This will create a token" << endl;
  cout << "                                      that may be used in future -dss calls" << endl;
  cout << "  -dss-services-list                : List all available segmentation services" << endl;
  cout << "  -dss-services-detail <service_id> : Get detailed description of a service" << endl;
  cout << "  -dss-tickets-create <service_id>  : Create a new ticket using current workspace. 'service_id' may be" << endl;
  cout << "                                      either the name or git hash returned by -dss-services-list." << endl;
  cout << "  -dss-tickets-list                 : List all of your tickets" << endl;
  cout << "  -dss-tickets-log <id>             : Get the error/warning/info log for ticket 'id'" << endl;
  cout << "  -dss-tickets-progress <id>        : Get the total progress for ticket 'id'" << endl;
  cout << "  -dss-tickets-wait <id> [timeout]  : Wait for the ticket 'id' to complete" << endl;
  cout << "  -dss-tickets-download <id> <dir>  : Download the result for ticket 'id' to directory 'dir'" << endl;
  cout << "  -dss-tickets-delete <id>          : Delete a ticket" << endl;
  cout << "DSS service provider commands: " << endl;
  cout << "  -dssp-services-list               : List all the services you are listed as provider for" << endl;
  cout << "  -dssp-services-claim <service_hash_list> <provider> <instance_id> [timeout]" << endl;
  cout << "                                    : Claim the next available ticket for given service or list of services." << endl;
  cout << "                                      'service_hash_list' is a comma-separated list of service git hashes." << endl;
  cout << "                                      'provider' is the provider identifier code" << endl;
  cout << "                                      'instance_id' is a unique identifier within the provider" << endl;
  cout << "                                      if 'timeout' specified, command will halt until a ticket " << endl;
  cout << "                                      can be claimed or 'timeout' seconds pass." << endl;
  cout << "  -dssp-tickets-download <id> <dir> : Download the files for claimed ticket id to dir" << endl;
  cout << "  -dssp-tickets-fail <id> <msg>     : Mark the ticket as failed with provided message" << endl;
  cout << "  -dssp-tickets-success <id>        : Mark the ticket as successfully completed " << endl;
  cout << "  -dssp-tickets-status <id>         : Check the status of a ticket" << endl;
  cout << "  -dssp-tickets-set-progress ...    : Set progress for ticket 'id'. Progress is specified as" << endl;
  cout << "      <id> <start> <end> <value>      chunk start, chunk end, and progress within chunk (all in [0 1])" << endl;
  cout << "  -dssp-tickets-log ...             : Add a log message for ticket 'id'. Type is info|warning|error" << endl; 
  cout << "      <id> <type> <msg>               " << endl;
  cout << "  -dssp-tickets-attach ...          : Attach a file to the ticket <id>. The file will be linked to the next" << endl;
  cout << "      <id> <desc> <file> [mimetype]   log command issued for this ticket" << endl;
  cout << "  -dssp-tickets-upload <id>         : Send the current workspace as the result for ticket 'id'" << endl;
  cout << "DSS administrative commands: " << endl;
  cout << "  -dssa-providers-list              : List all the registered providers" << endl;
  cout << "  -dssa-providers-add <name>        : Add new provider" << endl;
  cout << "  -dssa-providers-delete <name>     : Remove provider" << endl;
  cout << "  -dssa-providers-users-list        : List all users authorized under a provider" << endl;
  cout << "  -dssa-providers-users-add <provider_name> <user_email>" << endl;
  cout << "  -dssa-providers-users-delete <provider_name> <user_numeric_id>" << endl;
  cout << "  -dssa-providers-services-list <provider_name>" << endl;
  cout << "                                    : List all services under a provider" << endl;
  cout << "  -dssa-providers-services-add <provider_name> <git_repo> <git_ref>" << endl;
  cout << "                                    : Add a service under a provider by specifying a githash repository" << endl;
  cout << "                                      and reference (branch/tag/commit)" << endl;
  cout << "  -dssa-providers-services-delete <provider_name> <service_githash>" << endl;
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
  cout << "Multi-Component Display (MCD) Specification:" << endl;
  cout << "  comp <N>                          : Display N-th component" << endl;
  cout << "  <mag|avg|max|rgb|grid>            : Special modes" << endl;
  cout << "Environment Variables" << endl;
  cout << "  ITKSNAP_WT_DSS_SERVER             : URL of the server to use. When you authenticate with -dss-auth" << endl;
  cout << "                                      the server is stored in a config file. When this variable is set" << endl;
  cout << "                                      the config file is ignored and this server is used instead." << endl;
  return rc;
}



/**
 * Upload a workspace as the result for a ticket
 */
void UploadResultWorkspace(const WorkspaceAPI &ws, int ticket_id)
{
  // Locally export and upload the workspace
  ws.UploadWorkspace("api/pro/tickets/%d/files/results", ticket_id, "_results");
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

void simple_rest_get(const char *url, const char *exception_message, const char *prefix, ...)
{
  // Handle the ...
  std::va_list args;
  va_start(args, prefix);

  // Execute the REST command
  RESTClient rc;

  // Try calling command
  try {
    if(!rc.GetVA(url, args))
      throw IRISException("%s: %s", exception_message, rc.GetResponseText());
    va_end(args);
  }
  catch(IRISException &exc) {
    va_end(args);
    throw;
  }

  // Print CSV
  print_string_with_prefix(cout, rc.GetFormattedCSVOutput(false), prefix);
}

void simple_rest_post(const char *url, const char *params, const char *exception_message, const char *prefix, ...)
{
  // Handle the ...
  std::va_list args;
  va_start(args, prefix);

  // Execute the REST command
  RESTClient rc;

  // Try calling command
  std::cout << "prefix: " << prefix << std::endl;
  try {
    if(!rc.PostVA(url, params, args))
      throw IRISException("%s: %s", exception_message, rc.GetResponseText());
    va_end(args);
  }
  catch(IRISException &exc) {
    va_end(args);
    throw;
  }

  cout << prefix << rc.GetOutput() << endl;
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
        ws.ExportWorkspace(cl.read_output_filename().c_str());
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
        std::list<std::string> layers = ws.FindLayersByTag(tag);

        if(layers.size() != 1)
          throw IRISException("No unique layer found, tag %s is associated with %d layers", tag.c_str(), layers.size());

        layer_folder = layers.front();

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

      else if(arg == "-props-registry-dump" || arg == "-prd")
        {
        if(!ws.IsKeyValidLayer(layer_folder))
          throw IRISException("Selected object %s is not a valid layer", layer_folder.c_str());

        string key = cl.read_string();
        string out_file = cl.read_output_filename();
        Registry &folder_to_dump = ws.GetRegistry().Folder(layer_folder).Folder(key);
        folder_to_dump.WriteToFile(out_file.c_str());
        }

      else if(arg == "-props-registry-load" || arg == "-prl")
        {
        if(!ws.IsKeyValidLayer(layer_folder))
          throw IRISException("Selected object %s is not a valid layer", layer_folder.c_str());

        string key = cl.read_string();
        string in_file = cl.read_existing_filename();
        Registry &folder_to_dump = ws.GetRegistry().Folder(layer_folder).Folder(key);
        folder_to_dump.ReadFromFile(in_file.c_str());
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

      else if(arg == "-props-get-transform" || arg == "-pgt")
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

        // Print the matrix
        for(unsigned int i = 0; i < 4; i++)
          {
          cout << prefix << Q(i,0) << " " << Q(i,1) << " " << Q(i,2) << " " << Q(i,3) << endl;
          }
        }

      else if (arg == "-props-set-transform" || arg == "-pst")
        {
        // Must be a valid folder
        if(!ws.IsKeyValidLayer(layer_folder))
          throw IRISException("Selected object %s is not a valid layer", layer_folder.c_str());

        // Read the transform file
        std::string fn_tran = cl.read_existing_filename();

        // Read the transform. Format is not specified so we assume it is in C3D format
        SmartPtr<AffineTransformHelper::ITKTransformMOTB> tran =
            AffineTransformHelper::ReadAsRASMatrix(fn_tran.c_str());

        // Get the folder
        Registry &folder = ws.GetRegistry().Folder(layer_folder);

        // Write to registry
        AffineTransformHelper::WriteToRegistry(&folder, tran.GetPointer());
        }

      else if(arg == "-props-set-mcd")
        {
        // Read the mode information
        string mode = cl.read_string();
        std::transform(mode.begin(), mode.end(), mode.begin(), ::tolower);

        // Initialize the multi-channel display mode
        MultiChannelDisplayMode mcd;
        if(mode == "comp")
          {
          mcd.SelectedScalarRep = SCALAR_REP_COMPONENT;
          mcd.SelectedComponent = (int) cl.read_integer();
          }
        else if(mode == "avg")
          mcd.SelectedScalarRep = SCALAR_REP_AVERAGE;
        else if(mode == "mag")
          mcd.SelectedScalarRep = SCALAR_REP_MAGNITUDE;
        else if(mode == "max")
          mcd.SelectedScalarRep = SCALAR_REP_MAX;
        else if(mode == "rgb")
          mcd = MultiChannelDisplayMode::DefaultForRGB();
        else if(mode == "grid")
          {
          mcd = MultiChannelDisplayMode::DefaultForRGB();
          mcd.RenderAsGrid = true;
          }

        // Store the mode in the folder
        ws.SetLayerMultiComponentDisplay(layer_folder, mcd);
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
      else if(arg == "-annot-list")
        {
        ws.PrintAnnotationList(cout, prefix);
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
        if(!rc.Authenticate(url.c_str(), token_string.c_str()))
          throw IRISException("Authentication error: %s", rc.GetResponseText());
        else
          printf("Success: %s", rc.GetOutput());
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
        int ticket_id = ws.CreateWorkspaceTicket(service_githash.c_str());
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
      else if(arg == "-dss-tickets-delete" || arg == "-dt-del")
        {
        int ticket_id = cl.read_integer();
        RESTClient rc;
        if(rc.Get("api/tickets/%d/delete", ticket_id))
          cout << prefix << rc.GetOutput() << endl;
        else
          throw IRISException("Error deleting ticket %d: %s", ticket_id, rc.GetResponseText());

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

        // Main loop
        clock_t t_start = clock();
        clock_t t_end = t_start + timeout * CLOCKS_PER_SEC;

        // Use a single REST client
        RESTClient rc;

        // Run main loop until timeout
        long last_log = 0;
        double progress = 0.0;
        std::string status;

        // Keep track of the number of consecutive failures
        int n_conseq_fail = 0;

        // Keep a loop counter
        int loop_counter = 0;

        bool timed_out = true;
        while(clock() < t_end && n_conseq_fail < 5)
          {
          // Go to the begin of line - to erase the current progress
          printf("\r");

          // Count a consecutive failure
          n_conseq_fail++;

          if(rc.Get("api/tickets/%ld/detail?since=%ld", ticket_id, last_log))
            {
            Json::Reader json_reader;
            Json::Value root;
            if(json_reader.parse(rc.GetOutput(), root, false))
              {
              const Json::Value result = root["result"];

              // Read progress
              progress = result.get("progress", progress).asDouble();
              status = result.get("status","").asString();

              // Print the log messages
              const Json::Value log_entry = result["log"];
              for(int i = 0; i < log_entry.size(); i++)
                {
                last_log = log_entry[i].get("id", (int) last_log).asLargestInt();
                printf("%20s %10s %s\n",
                       log_entry[i].get("atime","").asString().c_str(),
                       log_entry[i].get("category","").asString().c_str(),
                       log_entry[i].get("message","").asString().c_str());


                const Json::Value att_entry = log_entry[i]["attachments"];
                for(int i = 0; i < att_entry.size(); i++)
                  {
                  printf("  @ %s : %s\n",
                         att_entry[i].get("url","").asString().c_str(),
                         att_entry[i].get("description","").asString().c_str());
                  }
                }

              // This loop run was a success
              n_conseq_fail = 0;
              }
            }

          // Display the progress nicely
          for(int i = 0; i < 78; i++)
            if(i <=  progress * 78 )
              printf("#");
            else
              printf(" ");
          printf(" %3d%% ", (int) (100 * progress));

          // If status is something terminal, exit
          if(status == "failed" || status == "success" || status == "timeout" || status == "deleted")
            {
            timed_out = false;
            printf("\n");
            break;
            }

          // Show a blop
          const char blop[] = "|/-\\";
            printf("%c", blop[(loop_counter++) % 4]);
          fflush(stdout);

          // Sleep (time depends on failures)
          sleep(n_conseq_fail == 0 ? 5 : 10);
          }

        // Print additional information
        if(timed_out)
          {
          printf("\nTimed out\n");
          return -1;
          }
        else
          {
          printf("\nTicket completed with status: %s\n", status.c_str());
          }
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
          if(!rc.Post("api/pro/services/claims","services=%s&provider=%s&code=%s",
                      service_githash.c_str(), provider_name.c_str(), provider_code.c_str()))
            throw IRISException("Error claiming ticket for service %s: %s", 
              service_githash.c_str(), rc.GetResponseText());

          // The output will be a table with ticket id and githash
          FormattedTable ft;
          ft.ParseCSV(rc.GetOutput());

          int ticket_id;
          if(ft.Rows() == 1 && (ticket_id = atoi(ft(0, 0).c_str())) > 0)
            {
            ft.Print(cout, prefix);
            context_ticket_id = ticket_id;
            break;
            }
          else if(tnow + twait > timeout)
            {
            cerr << "Timed out waiting for available tickets" << endl;
            exit(1);
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
        string file_list = WorkspaceAPI::DownloadTicketFiles(ticket_id, output_path.c_str(), false, "results");
        print_string_with_prefix(cout, file_list, prefix);
        }
      else if(arg == "-dssp-tickets-download")
        {
        int ticket_id = cl.read_integer();
        string output_path = cl.read_string();
        string file_list = WorkspaceAPI::DownloadTicketFiles(ticket_id, output_path.c_str(), true, "input");
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
      else if(arg == "-dssp-tickets-status")
        {
        int ticket_id = cl.read_integer();
        RESTClient rc;
        if(!rc.Get("api/pro/tickets/%d/status", ticket_id))
          throw IRISException("Error checking status of ticket %d: %s",
            ticket_id, rc.GetResponseText());
        cout << prefix << rc.GetOutput() << endl;
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
      else if(arg == "-dssa-providers-list")
        {
        simple_rest_get("api/admin/providers", "Error listing providers", prefix.c_str());
        }
      else if(arg == "-dssa-providers-add")
        {
        std::string pname = cl.read_string();
        simple_rest_post("api/admin/providers", "name=%s", "Error adding provider", prefix.c_str(), pname.c_str());
        }
      else if(arg == "-dssa-providers-delete")
        {
        std::string pname = cl.read_string();
        simple_rest_post("api/admin/providers/%s/delete", NULL, "Error deleting provider", prefix.c_str(), pname.c_str());
        }
      else if(arg == "-dssa-providers-users-list")
        {
        std::string pname = cl.read_string();
        simple_rest_get("api/admin/providers/%s/users", "Error listing provider's users", prefix.c_str(), pname.c_str());
        }
      else if(arg == "-dssa-providers-users-add")
        {
        std::string pname = cl.read_string();
        std::string email = cl.read_string();
        simple_rest_post("api/admin/providers/%s/users", "email=%s", "Error adding user to provider", prefix.c_str(), 
                         pname.c_str(), email.c_str());
        }
      else if(arg == "-dssa-providers-users-delete")
        {
        std::string pname = cl.read_string();
        int user_id = cl.read_integer();
        simple_rest_post("api/admin/providers/%s/users/%d/delete", NULL, "Error deleting user from provider", prefix.c_str(), 
                         pname.c_str(), user_id);
        }
      else if(arg == "-dssa-providers-services-list")
        {
        std::string pname = cl.read_string();
        simple_rest_get("api/admin/providers/%s/services", "Error listing provider's services", prefix.c_str(), pname.c_str());
        }
      else if(arg == "-dssa-providers-services-add")
        {
        std::string pname = cl.read_string();
        std::string repo = cl.read_string();
        std::string ref = cl.read_string();
        simple_rest_post("api/admin/providers/%s/services", "repo=%s&ref=%s", "Error adding service to provider", prefix.c_str(), 
                         pname.c_str(), repo.c_str(), ref.c_str());
        }
      else if(arg == "-dssa-providers-services-delete")
        {
        std::string pname = cl.read_string();
        std::string githash = cl.read_string();
        simple_rest_post("api/admin/providers/%s/services/%s/delete", NULL, "Error deleting user from provider", prefix.c_str(), 
                         pname.c_str(), githash.c_str());
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
