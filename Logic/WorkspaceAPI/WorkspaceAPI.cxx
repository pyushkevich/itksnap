#include "WorkspaceAPI.h"
#include "IRISException.h"
#include "itksys/MD5.h"
#include "itksys/Directory.hxx"
#include "itksys/SystemTools.hxx"
#include "itksys/RegularExpression.hxx"
#include "FormattedTable.h"
#include "GuidedNativeImageIO.h"
#include "ColorLabelTable.h"
#include "MultiChannelDisplayMode.h"
#include "RESTClient.h"
#include "itkCommand.h"

using namespace std;
using itksys::SystemTools;
using itksys::RegularExpression;
using itksys::Directory;


void WorkspaceAPI::ReadFromXMLFile(const char *proj_file)
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

void WorkspaceAPI::SaveAsXMLFile(const char *proj_file)
{
  // Get the full name of the project file
  string proj_file_full = SystemTools::CollapseFullPath(proj_file);

  // Get the directory in which the project will be saved
  string project_dir = SystemTools::GetParentDirectory(proj_file_full.c_str());

  // Put version information - later versions may not be compatible
  m_Registry["Version"] << SNAPCurrentVersionReleaseDate;

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

int WorkspaceAPI::GetNumberOfLayers() const
{
  // Unfortunately we have to count the folders each time we want to return the number
  // of layers. This is the unfortunate consequence of the lame way in which Registry
  // supports arrays of folders.
  int n_layers = 0;
  while(m_Registry.HasFolder(Registry::Key("Layers.Layer[%03d]", n_layers)))
    n_layers++;

  return n_layers;
}

Registry &WorkspaceAPI::GetLayerFolder(int layer_index)
{
  string key = Registry::Key("Layers.Layer[%03d]", layer_index);
  return m_Registry.Folder(key);
}

Registry &WorkspaceAPI::GetLayerFolder(const string &layer_key)
{
  if(!layer_key.length() || !m_Registry.HasFolder(layer_key))
    throw IRISException("Layer folder %s does not exist", layer_key.c_str());
  return m_Registry.Folder(layer_key);
}

bool WorkspaceAPI::IsKeyValidLayer(const string &key)
{
  RegularExpression re("Layers.Layer\\[[0-9]+\\]");
  if(!re.find(key))
    return false;

  if(!m_Registry.HasFolder(key))
    return false;

  Registry &folder = m_Registry.Folder(key);

  return folder.HasEntry("AbsolutePath") && folder.HasEntry("Role");
}

// TODO: merge with IRISApplication
string WorkspaceAPI::GetLayerActualPath(Registry &folder)
{
  // Get the filenames for the layer
  string layer_file_full = folder["AbsolutePath"][""];

  // If the project has moved, try finding a relative location
  if(m_Moved)
    {
    // Get the relative path of the layer wrt project
    string relative_path;

    // Test the simple thing: is the project location included in the file path
    if(layer_file_full.compare(0, m_WorkspaceSavedDir.length(), m_WorkspaceSavedDir) == 0)
      {
      // Get the balance of the path
      relative_path = layer_file_full.substr(m_WorkspaceSavedDir.length());

      // Strip the leading slashes
      SystemTools::ConvertToUnixSlashes(relative_path);
      relative_path = relative_path.substr(relative_path.find_first_not_of('/'));
      }
    else
      {
      // Fallback: use relative path mechanism
      relative_path = SystemTools::RelativePath(
                                   m_WorkspaceSavedDir.c_str(), layer_file_full.c_str());
      }

    string moved_file_full = SystemTools::CollapseFullPath(
                               relative_path.c_str(), m_WorkspaceFileDir.c_str());

    if(SystemTools::FileExists(moved_file_full.c_str(), true))
      layer_file_full = moved_file_full;
    }

  // Return the file - no guarantee that it exists...
  return layer_file_full;
}

void WorkspaceAPI::SetAllLayerPathsToActualPaths()
{
  for(int i = 0; i < this->GetNumberOfLayers(); i++)
    {
    Registry &folder = this->GetLayerFolder(i);
    folder["AbsolutePath"] << this->GetLayerActualPath(folder);
    }
}

Registry *WorkspaceAPI::GetLayerIOHints(Registry &folder)
{
  // Load the IO hints for the image from the project - but only if this
  // folder is actually present (otherwise some projects from before 2016
  // will not load hints)
  Registry *io_hints = NULL;
  if(folder.HasFolder("IOHints"))
    io_hints = &folder.Folder("IOHints");
  return io_hints;
}

void WorkspaceAPI::PrintLayerList(std::ostream &os, const string &line_prefix)
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

int WorkspaceAPI::GetNumberOfAnnotations()
{
  return m_Registry["Annotations.Annotations.ArraySize"][0];
}

Registry & WorkspaceAPI::GetAnnotationFolder(int annot_index)
{
  string key = Registry::Key("Annotations.Annotations.Element[%d]", annot_index);
  return m_Registry.Folder(key);
}

#include "ImageAnnotationData.h"

void WorkspaceAPI::PrintAnnotationList(std::ostream &os, const string &line_prefix)
{
  // Annotations are lightweight, so we can using existing API to load them
  SmartPtr<ImageAnnotationData> iad = ImageAnnotationData::New();
  Registry f_annot = m_Registry.Folder("Annotations");
  iad->LoadAnnotations(f_annot);

  // Use a formatted table
  FormattedTable table(4);

  // Print the header information
  table << "Annotation" << "Kind" << "Center" << "Tags";

  // Iterate
  int i = 0;
  for(ImageAnnotationData::AnnotationConstIterator it = iad->GetAnnotations().begin();
      it != iad->GetAnnotations().end(); it++, i++)
    {
    annot::AbstractAnnotation *ann = *it;
    annot::AnnotationType type = ann->GetType();
    table
        << i
        << (type==annot::LANDMARK ? "Landmark" : "LineSegment")
        << ann->GetCenter()
        << ann->GetTags().ToString();
    }

  table.Print(os, line_prefix);
}

std::list<std::string> WorkspaceAPI::FindLayersByTag(const string &tag)
{
  // Iterate over all the layers stored in the workspace
  int n_layers = this->GetNumberOfLayers();

  // Matching layers
  std::list<std::string> matches;

  // Load all of the layers in the current project
  for(int i = 0; i < n_layers; i++)
    {
    // Get the key of the layer
    string key = Registry::Key("Layers.Layer[%03d]", i);
    Registry &f = m_Registry.Folder(key);

    // Get the tags in this layer
    StringSet tags = WorkspaceAPI::GetTags(f);
    if(tags.find(tag) != tags.end())
      {
      matches.push_back(key);
      }
    }

  return matches;
}

void WorkspaceAPI::ListLayerFilesForTag(const string &tag, ostream &sout, const string &prefix)
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

WorkspaceAPI::StringSet WorkspaceAPI::GetTags(Registry &folder)
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

void WorkspaceAPI::PutTags(Registry &folder, const StringSet &tags)
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

void WorkspaceAPI::AddTag(Registry &folder, const string &newtag)
{
  set<string> tags = this->GetTags(folder);
  tags.insert(newtag);
  this->PutTags(folder, tags);
}

void WorkspaceAPI::RemoveTag(Registry &folder, const string &tag)
{
  set<string> tags = this->GetTags(folder);
  tags.erase(tag);
  this->PutTags(folder, tags);
}

void WorkspaceAPI::ScrubTag(Registry &folder, const string &tag)
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

void WorkspaceAPI::FindTag(Registry &folder, const string &tag, StringList &found_keys, const string &prefix)
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

string WorkspaceAPI::FindFolderForUniqueTag(const string &tag)
{
  list<string> found_keys;
  this->FindTag(m_Registry, tag, found_keys, "");
  if(found_keys.size() == 0)
    throw IRISException("No folders with tag %s found in the workspace", tag.c_str());
  else if(found_keys.size() > 1)
    throw IRISException("Multiple folders with tag %s found in the workspace", tag.c_str());

  return found_keys.back();
}

string WorkspaceAPI::FindLayerByRole(const string &role, int pos_in_role)
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

string WorkspaceAPI::LayerSpecToKey(const string &layer_spec)
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

string WorkspaceAPI::GetMainLayerKey()
{
  return this->LayerSpecToKey("M");
}

void WorkspaceAPI::UpdateMainLayerFieldsFromImage(Registry &main_layer_folder)
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

string WorkspaceAPI::AddLayer(string role, const string &filename)
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
  folder["AbsolutePath"] << SystemTools::CollapseFullPath(filename);
  folder["Role"] << role;

  // If the role is 'main' then we need to write the dimensions of the image into projectmetadata
  if(role == "MainRole")
    this->UpdateMainLayerFieldsFromImage(folder);

  return key;
}

string WorkspaceAPI::SetLayer(string role, const string &filename)
{
  string key = this->FindLayerByRole(role, 0);

  // If this role does not already exist, we use the 'Add' functionality
  if(key.length() == 0)
    key = this->AddLayer(role, filename);

  // Otherwise, we trash the old folder associated with this role
  Registry &folder = m_Registry.Folder(key);
  folder.Clear();

  // Add the filename and role
  folder["AbsolutePath"] << SystemTools::CollapseFullPath(filename);
  folder["Role"] << role;

  // If the role is 'main' then we need to write the dimensions of the image into projectmetadata
  if(role == "MainRole")
    this->UpdateMainLayerFieldsFromImage(folder);

  // Return the key
  return key;
}

void WorkspaceAPI::WriteLayerContrastToRegistry(Registry &folder, int n, double *tarray, double *yarray)
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

void WorkspaceAPI::SetLayerNickname(const string &layer_key, const string &value)
{
  GetLayerFolder(layer_key)["LayerMetaData.CustomNickName"] << value;
}

void WorkspaceAPI::SetLayerColormapPreset(const string &layer_key, const string &value)
{
  GetLayerFolder(layer_key)["LayerMetaData.DisplayMapping.ColorMap.Preset"] << value;
}

void WorkspaceAPI::SetLayerMultiComponentDisplay(const string &layer_key, const MultiChannelDisplayMode &mode)
{
  Registry &folder = GetLayerFolder(layer_key);
  mode.Save(folder.Folder("LayerMetaData.DisplayMapping"));
}

void WorkspaceAPI::RenameLayer(const string &layer_key, const string &new_filename, bool force)
{
  // Get the folder for this layer
  Registry &layer_folder = GetLayerFolder(layer_key);

  // Get the filename for this layer
  string fn_layer = this->GetLayerActualPath(layer_folder);

  // Get the desired filename for this layer
  string fn_new_full = SystemTools::CollapseFullPath(new_filename.c_str());

  // If the files are the same, generate an warning
  if(fn_layer == fn_new_full)
    {
    cerr << "Warning: attempt to rename a layer to itself" << endl;
    return;
    }

  // If the new file exists and force is off, do nothing
  if(SystemTools::FileExists(fn_new_full) && !force)
    {
    throw IRISException("Cannot rename file %s as %s because an existing file will be overwritten",
                        fn_layer.c_str(), fn_new_full.c_str());
    }

  // Read and write the file
  Registry io_hints, *layer_io_hints;
  if((layer_io_hints = this->GetLayerIOHints(layer_folder)))
    io_hints.Update(*layer_io_hints);

  // Create a native image IO object for this image
  SmartPtr<GuidedNativeImageIO> io = GuidedNativeImageIO::New();

  // Load the header of the image and the image data
  io->ReadNativeImage(fn_layer.c_str(), io_hints);

  // Save the layer as nee image. Since we are saving as a NIFTI, we don't need to
  // provide any hints
  Registry dummy_hints;
  io->SaveNativeImage(fn_new_full.c_str(), dummy_hints);

  // Update the layer folder with the new path
  layer_folder["AbsolutePath"] << fn_new_full;

  // There are no hints necessary for NIFTI
  layer_folder.Folder("IOHints").Clear();
}

void WorkspaceAPI::SetLayerContrastToLinear(const string &layer_key, double t0, double t1)
{
  // Get the folder containing the curve
  Registry &layer_folder = GetLayerFolder(layer_key);
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

void WorkspaceAPI::SetLabels(const string &label_file)
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

void WorkspaceAPI::AddLabels(const string &label_file, int offset, const string &rename_pattern)
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

void WorkspaceAPI::ClearLabels()
{
  // Get the main layer
  Registry &main = m_Registry.Folder(this->GetMainLayerKey());

  // Read the existing labels
  SmartPtr<ColorLabelTable> clt = ColorLabelTable::New();
  clt->RemoveAllLabels();

  // Clear the label table
  clt->SaveToRegistry(main.Folder("ProjectMetaData.IRIS.LabelTable"));
}

const Registry &WorkspaceAPI::GetRegistry() const
{
  return m_Registry;
}

Registry &WorkspaceAPI::GetRegistry()
{
  return m_Registry;
}

Registry &WorkspaceAPI::GetFolder(const string &key)
{
  return m_Registry.Folder(key);
}

bool WorkspaceAPI::HasFolder(const string &key)
{
  return m_Registry.HasFolder(key);
}

string WorkspaceAPI::GetWorkspaceActualDirectory() const
{
  return m_WorkspaceFileDir;
}

string WorkspaceAPI::GetTempDirName()
{
#ifdef WIN32
  char tempDir[_MAX_PATH + 1] = "";
  char tempFile[_MAX_PATH + 1] = "";

  // First call return a directory only
  DWORD length = GetTempPath(_MAX_PATH+1, tempDir);
  if(length <= 0 || length > _MAX_PATH)
    throw IRISException("Unable to create temporary directory");

  // This will create a unique file in the temp directory
  if (0 == GetTempFileName(tempDir, TEXT("alfabis"), 0, tempFile))
    throw IRISException("Unable to create temporary directory");

  // We use the filename to create a directory
  string dir = tempFile;
  dir += "_d";

  return dir;

#else
  char tmp_template[4096];
  strcpy(tmp_template, "/tmp/alfabis_XXXXXX");
  string tmpdir = mkdtemp(tmp_template);
  return tmpdir;
#endif
}

#include "AllPurposeProgressAccumulator.h"

void WorkspaceAPI::ExportWorkspace(const char *new_workspace,
                                   CommandType *cmd_progress,
                                   bool scramble_filenames) const
{
  // Create a progress tracker
  SmartPtr<TrivalProgressSource> progress = TrivalProgressSource::New();
  if(cmd_progress)
    {
    progress->AddObserver(itk::StartEvent(), cmd_progress);
    progress->AddObserver(itk::ProgressEvent(), cmd_progress);
    progress->AddObserver(itk::EndEvent(), cmd_progress);
    }

  // Duplicate the workspace data
  WorkspaceAPI wsexp = (*this);

  // Convert to absolute path (because project will use absolute path)
  string ws_file_full = SystemTools::CollapseFullPath(new_workspace);

  // Get the directory in which the project will be saved
  string wsdir = SystemTools::GetParentDirectory(ws_file_full.c_str());

  // Iterate over all the layers stored in the workspace
  int n_layers = wsexp.GetNumberOfLayers();

  // Report progress
  progress->StartProgress(n_layers);

  // Load all of the layers in the current project
  for(int i = 0; i < n_layers; i++)
    {
    // Get the folder corresponding to the layer
    Registry &f_layer = wsexp.GetLayerFolder(i);

    // The the (possibly moved) absolute filename
    string fn_layer = wsexp.GetLayerActualPath(f_layer);

    // Get the current layer base filename
    string fn_layer_basename = SystemTools::GetFilenameWithoutExtension(fn_layer);

    // The IO hints for the file
    Registry io_hints, *layer_io_hints;
    if((layer_io_hints = wsexp.GetLayerIOHints(f_layer)))
      io_hints.Update(*layer_io_hints);

    // Create a native image IO object for this image
    SmartPtr<GuidedNativeImageIO> io = GuidedNativeImageIO::New();

    // Load the header of the image and the image data
    io->ReadNativeImage(fn_layer.c_str(), io_hints);

    // Report progress
    progress->AddProgress(0.5);

    // Compute the hash of the image data to generate filename
    if(scramble_filenames)
      {
      // Use the hash as the basename
      fn_layer_basename = io->GetNativeImageMD5Hash();
      }

    // Create a filename that combines the layer index with the hash code
    char fn_layer_new[4096];
    sprintf(fn_layer_new, "%s/layer_%03d_%s.nii.gz", wsdir.c_str(), i, fn_layer_basename.c_str());

    // Save the layer there. Since we are saving as a NIFTI, we don't need to
    // provide any hints
    Registry dummy_hints;
    io->SaveNativeImage(fn_layer_new, dummy_hints);

    // Report progress
    progress->AddProgress(0.5);

    // Update the layer folder with the new path
    f_layer["AbsolutePath"] << fn_layer_new;

    // There are no hints necessary for NIFTI
    f_layer.Folder("IOHints").Clear();
    }

  // Write the updated project
  wsexp.SaveAsXMLFile(new_workspace);

  // Report progress
  progress->EndProgress();
}

void WorkspaceAPI::UploadWorkspace(const char *url, int ticket_id,
                                   const char *wsfile_suffix,
                                   CommandType *cmd_progress) const
{
  // There is a lot of progress to keep track of so we create an accumulator
  SmartPtr<AllPurposeProgressAccumulator> accum = AllPurposeProgressAccumulator::New();
  if(cmd_progress)
    accum->AddObserver(itk::ProgressEvent(), cmd_progress);

  // Create a command that can be passed on to the export code
  SmartPtr<CommandType> cmd_export = accum->RegisterITKSourceViaCommand(0.5);

  // Create a second accumulator for the upload. This is because we do not yet know how
  // many files we will be uploading (not until the export is done)
  SmartPtr<AllPurposeProgressAccumulator> accum_upload = AllPurposeProgressAccumulator::New();
  accum->RegisterSource(accum_upload, 0.5);

  // Create temporary directory for the export
  string tempdir = GetTempDirName();
  SystemTools::MakeDirectory(tempdir);

  // Export the workspace file to the temporary directory
  char ws_fname_buffer[4096];
  sprintf(ws_fname_buffer, "%s/ticket_%08d%s.itksnap", tempdir.c_str(), ticket_id, wsfile_suffix);
  ExportWorkspace(ws_fname_buffer, cmd_export);

  // Count the number of files in the directory
  std::vector<std::string> fn_to_upload;
  Directory dir;
  dir.Load(tempdir);
  for(int i = 0; i < dir.GetNumberOfFiles(); i++)
    {
    const char *thisfile = dir.GetFile(i);
    string file_full_path = SystemTools::CollapseFullPath(thisfile, dir.GetPath());
    if(!SystemTools::FileIsDirectory(file_full_path))
      fn_to_upload.push_back(file_full_path);
    }

  cout << "Exported workspace to " << ws_fname_buffer << endl;

  // Create a source for transfer progress
  void *transfer_progress_src = accum_upload->RegisterGenericSource(fn_to_upload.size(), 1.0);

  // For each of the files in the directory upload it
  for(int i = 0; i < fn_to_upload.size(); i++)
    {
    const char *fn = fn_to_upload[i].c_str();

    RESTClient rcu;

    // Set progress callback
    rcu.SetProgressCallback(transfer_progress_src,
                            AllPurposeProgressAccumulator::GenericProgressCallback);

    // TODO: this is disgraceful!
    std::map<string, string> empty_map;
    if(!rcu.UploadFile(url, fn, empty_map, ticket_id))
      throw IRISException("Failed up upload file %s (%s)", fn, rcu.GetResponseText());

    // Reset progress counter for next run
    accum_upload->StartNextRun(transfer_progress_src);

    cout << "Upload " << fn << " (" << rcu.GetUploadStatistics() << ")" << endl;
    }

  // Finish with the progress
  accum_upload->UnregisterAllSources();
  accum->UnregisterAllSources();

  // TODO: we should verify that all the files were successfully sent, via MD5
}

int WorkspaceAPI::CreateWorkspaceTicket(const string &service_desc,
                                        CommandType *cmd_progress) const
{
  // What is specified - name or githash?
  std::string which_desc;
  if(service_desc.length() == 40 &&
     service_desc.find_first_not_of("0123456789abcdef") == string::npos)
    {
    which_desc = "githash";
    }
  else
    {
    which_desc = "name";
    }

  // Create a new ticket
  RESTClient rc;
  if(!rc.Post("api/tickets","%s=%s", which_desc.c_str(), service_desc.c_str()))
    throw IRISException("Failed to create new ticket (%s)", rc.GetResponseText());

  int ticket_id = atoi(rc.GetOutput());

  cout << "Created new ticket (" << ticket_id << ")" << endl;

  // Locally export and upload the workspace
  UploadWorkspace("api/tickets/%d/files/input", ticket_id, "", cmd_progress);

  // Mark this ticket as ready
  if(!rc.Post("api/tickets/%d/status","status=ready", ticket_id))
    throw IRISException("Failed to mark ticket as ready (%s)", rc.GetResponseText());

  cout << "Changed ticket status to (" << rc.GetOutput() << ")" << endl;

  return ticket_id;
}

string WorkspaceAPI::DownloadTicketFiles(
    int ticket_id, const char *outdir, bool provider_mode,
    const char *area, const char *workspace_filename,
    CommandType *cmd_progress)
{
  // Output string
  ostringstream oss;

  // Provider mode-specific settings
  const char *url_base = (provider_mode) ? "api/pro" : "api";

  // Progress stuff
  SmartPtr<AllPurposeProgressAccumulator> accum = AllPurposeProgressAccumulator::New();
  if(cmd_progress)
    accum->AddObserver(itk::ProgressEvent(), cmd_progress);

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

  // Progress source
  void *transfer_progress_src = accum->RegisterGenericSource(ft.Rows(), 1.0);
  rc.SetProgressCallback(transfer_progress_src,
                         AllPurposeProgressAccumulator::GenericProgressCallback);

  // Iterate over the dictionary
  for(int iFile = 0; iFile < ft.Rows(); iFile++)
    {
    // Where we will write this file to
    int file_index = atoi(ft(iFile,0).c_str());
    string file_name = ft(iFile, 1);

    // If the filename is an '.itksnap' file, and the user requested a different filename
    // for the workspace, make the substitution
    if(workspace_filename
       && itksys::SystemTools::GetFilenameLastExtension(file_name) == ".itksnap")
      {
      file_name = workspace_filename;
      }

    // Make it into a full path
    string file_path = SystemTools::CollapseFullPath(file_name.c_str(), outdir);

    // Create a file handle
    FILE *fout = fopen(file_path.c_str(), "wb");
    rc.SetOutputFile(fout);

    if(!rc.Get("%s/tickets/%d/files/%s/%d", url_base, ticket_id, area, file_index))
      throw IRISException("Failed to download file %s for ticket %d (%s)",
        file_name.c_str(), ticket_id, rc.GetResponseText());

    rc.SetOutputFile(NULL);
    fclose(fout);

    // Start next run of uploading
    accum->StartNextRun(transfer_progress_src);

    oss << file_path << endl;
    }

  accum->UnregisterAllSources();

  return oss.str();
}
