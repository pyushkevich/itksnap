#ifndef PRESETMANAGER_H
#define PRESETMANAGER_H

#include "itkObject.h"
#include "SNAPCommon.h"

class SystemInterface;

/**
  A class that handles system and user presets for arbitrary data structures
  in ITK-SNAP. An example structure is the color map. The data structure must
  be described by a traits object. The traits object must define the following:

  // The type (derived from itk::Object) that is being managed
  typedef ManagedType;

  // The iterator type for the system presets
  typedef SystemPresetIterator;

  // The list of system presets
  static SystemPresetIterator SystemPresetBegin();
  static SystemPresetIterator SystemPresetEnd();

  // The unique name for this class of presets, used for storing in registry
  static std::string GetPresetCategoryName();

  // Apply system preset
  static void SetToSystemPreset(ManagedType *instance, const SystemPresetIterator &preset);
  static std::string GetSystemPresetName(const SystemPresetIterator &preset);

  // Registry io for the managed type
  static void ReadFromRegistry(ManagedType *instance, Registry &folder);
  static void WriteToRegistry(ManagedType *instance, Registry &folder);

  TODO: we need a good way for synchronizing presets across multiple sessions.
  Currently, the list of presets is read at startup, and as the user saves and
  deletes presets, these operations are carried out on disk, without checking
  what another SNAP session might have done.

  The object fires a itk::ModifiedEvent event when presets have been modified
 */
template<class TManagedObjectTraits>
class PresetManager : public itk::Object
{
public:

  irisITKObjectMacro(PresetManager, itk::Object)

  typedef typename TManagedObjectTraits::ManagedType ManagedType;
  typedef SmartPtr<ManagedType> ManagedTypePtr;
  typedef typename TManagedObjectTraits::SystemPresetIterator SystemPresetIterator;

  enum PresetType { PRESET_SYSTEM, PRESET_USER, PRESET_NONE };
  typedef std::pair<PresetType, std::string> PresetMatch;

  /** Load the presets from disk and initialize them */
  void Initialize(SystemInterface *si);

  /** Get the list of user and system presets */
  const std::vector<std::string> &GetSystemPresets()
    { return m_PresetSystem; }

  const std::vector<std::string> &GetUserPresets()
    { return m_PresetUser; }

  /**
   * Query if the passed in instance of the object matches one of the presets,
   * and if so, which type of preset (system or user)
   */
  PresetMatch QueryPreset(ManagedType *instance);

  /** Set the instance passed in to a preset */
  void SetToPreset(ManagedType *instance, const std::string &preset);

  /** Save an instance as a new preset or override an existing preset */
  void SaveAsPreset(ManagedType *instance, const std::string &preset);

  /** Delete a user preset */
  void DeletePreset(const std::string &preset);

  /** Access a preset */
  ManagedType *GetPreset(const std::string &preset);

  /** Whether a string is a valid preset */
  bool IsValidPreset(const std::string &preset);


protected:

  PresetManager();
  virtual ~PresetManager() {}

  // Pointer to the system interface object used to manage user presets
  SystemInterface *m_System;

  // The name of the category
  std::string m_Category;

  // Map of presets to instances
  typedef std::map<std::string, ManagedTypePtr> PresetMap;
  PresetMap m_PresetMap;

  // List of system and user presets
  std::vector<std::string> m_PresetSystem, m_PresetUser;

};




#endif // PRESETMANAGER_H
