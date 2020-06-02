#ifndef WORKSPACEAPI_H
#define WORKSPACEAPI_H

#include "Registry.h"
#include <set>

namespace itk { class Command; }

struct MultiChannelDisplayMode;

/**
 * This class encapsulates an ITK-SNAP workspace. It is just a wrapper around
 * a registry object, but with extra functions that support workspaces
 */
class WorkspaceAPI
{
public:

  typedef std::set<std::string> StringSet;
  typedef std::list<std::string> StringList;

  // Progress callback signature
  typedef itk::Command CommandType;

  /**
   * Read the workspace from a file, determine if it has been moved or copied
   * since it was saved originally.
   */
  void ReadFromXMLFile(const char *proj_file);

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
  void SaveAsXMLFile(const char *proj_file);

  /**
   * Get number of image layers in the workspace
   */
  int GetNumberOfLayers() const;

  /**
   * Get the folder for the n-th layer
   */
  Registry &GetLayerFolder(int layer_index);

  /**
   * Get the layer folder by key. This function throws an exception if the key does
   * not correspond to a valid layer.
   */
  Registry &GetLayerFolder(const std::string &layer_key);

  /**
   * Check if the provided key specifies a valid layer - a valid layer is specified
   * as "Layers.Layer[xxx]" and contains an absolute filename and a role as the minimum
   * required entries
   */
  bool IsKeyValidLayer(const std::string &key);

  /**
   * Find a physical file corresponding to a file referenced by the project, accouting
   * for the possibility that the project may have been moved or copied
   */
  std::string GetLayerActualPath(Registry &folder);

  /**
   * Convert all layer paths in the workspace to actual paths - should be done
   * when saving a project
   */
  void SetAllLayerPathsToActualPaths();

  /**
   * Get the IO hints for the layer, or NULL pointer if there are no IO hints stored
   */
  Registry * GetLayerIOHints(Registry &folder);

  void PrintLayerList(std::ostream &os, const std::string &line_prefix = "");

  /** List all layer files associated with a tag */
  void ListLayerFilesForTag(const std::string &tag, std::ostream &sout, const std::string &prefix);

  /** Get a list of tags from a particular folder */
  StringSet GetTags(Registry &folder);

  /** Put tags into a folder. Assumes that there are no commas in the tags */
  void PutTags(Registry &folder, const StringSet &tags);

  /** Add a tag to a particular folder */
  void AddTag(Registry &folder, const std::string &newtag);

  /** Remove a tag from a folder */
  void RemoveTag(Registry &folder, const std::string &tag);

  /** Remove given tag from all subfolders in the given folder */
  void ScrubTag(Registry &folder, const std::string &tag);

  /**
   * Recursive tag search
   */
  void FindTag(Registry &folder, const std::string &tag, StringList &found_keys, const std::string &prefix);

  /**
   * Find a folder for a given tag. This will crash if the tag is missing or
   * if more than one object has the given tag
   */
  std::string FindFolderForUniqueTag(const std::string &tag);

  /**
   * Find layer by role. If pos_in_role is negative, this looks from
   * the end for that role
   */
  std::string FindLayerByRole(const std::string &role, int pos_in_role);

  /**
   * Find layers that match a tag
   */
  std::list<std::string> FindLayersByTag(const std::string &tag);

  /**
   * Translate a shorthand layer specifier to a folder ID. Will throw an exception if
   * the layer specifier cannot be found or is out of range
   */
  std::string LayerSpecToKey(const std::string &layer_spec);

  /**
   * Get the folder id for the main image or throw exception if it does not exist
   */
  std::string GetMainLayerKey();

  /**
   * Set the main layer dimensions in the registry. This should be called whenever the
   * main layer is assigned or changed
   */
  void UpdateMainLayerFieldsFromImage(Registry &main_layer_folder);

  /** Add a layer to the workspace in a given role */
  std::string AddLayer(std::string role, const std::string &filename);

  /** Assign a layer to a specific role (main/seg) */
  std::string SetLayer(std::string role, const std::string &filename);

  /** Write a control point sequence to the folder for a contrast mapping */
  void WriteLayerContrastToRegistry(Registry &folder, int n, double *tarray, double *yarray);

  /** Set layer nickname */
  void SetLayerNickname(const std::string &layer_key, const std::string &value);

  /** Set layer nickname */
  void SetLayerColormapPreset(const std::string &layer_key, const std::string &value);

  /** Rename the layer on disk and in the project */
  void RenameLayer(const std::string &layer_key, const std::string &new_filename, bool force = false);

  /** Set the contrast mapping for the selected layer */
  void SetLayerContrastToLinear(const std::string &layer_key, double t0, double t1);

  /** Set the multi-component display mode in a layer */
  void SetLayerMultiComponentDisplay(const std::string &layer_key, const MultiChannelDisplayMode &mode);

  /** Set labels from a label file */
  void SetLabels(const std::string &label_file);

  /**
   * Add labels with an offset and a prefix/suffix.
   * Format of rename_pattern is "left %s" for example
   */
  void AddLabels(const std::string &label_file, int offset, const std::string &rename_pattern);

  /** Reset the labels - leaves only the clear label */
  void ClearLabels();

  /** Get the project registry by reference */
  const Registry &GetRegistry() const;

  /** Get the project registry by reference */
  Registry &GetRegistry();

  /** Get a folder within the registry - this is just a helper */
  Registry &GetFolder(const std::string &key);

  /** Get a folder within the registry - this is just a helper */
  bool HasFolder(const std::string &key);

  /** Get the absolute path to the directory where the project was loaded from */
  std::string GetWorkspaceActualDirectory()  const;

  /** Cross-platform way of getting a temporary path */
  static std::string GetTempDirName();

  /** Export the workspace */
  void ExportWorkspace(const char *new_workspace, CommandType *cmd_progress = NULL, bool scramble_filenames = true) const;

  /** Upload the workspace */
  void UploadWorkspace(const char *url, int ticket_id, const char *wsfile_suffix,
                       CommandType *cmd_progress = NULL) const;

  /** Create a ticket from a workspace */
  int CreateWorkspaceTicket(const std::string &service_desc, CommandType *cmd_progress = NULL) const;

  /**
   * Download ticket files to a directory. Flag provider_mode switches between
   * behavior for users and providers. String area is one of (input|results)
   */
  static std::string DownloadTicketFiles(
      int ticket_id, const char *outdir, bool provider_mode, const char *area,
      const char *workspace_filename = NULL, CommandType *cmd_progress = NULL);

  /** Get number of annotations in the workspace */
  int GetNumberOfAnnotations();

  /** Get n-th annotation folder */
  Registry &GetAnnotationFolder(int annot_index);

  /** Print a list of all annotations */
  void PrintAnnotationList(std::ostream &os, const std::string &line_prefix);

protected:

  // The Registry object containing workspace data
  Registry m_Registry;

  // Has the workspace moved from its original location
  bool m_Moved;

  // The full path and containing directory of the workspace file
  std::string m_WorkspaceFilePath, m_WorkspaceFileDir;

  // The directory where workspace was last saved
  std::string m_WorkspaceSavedDir;

};


#endif // WORKSPACEAPI_H
