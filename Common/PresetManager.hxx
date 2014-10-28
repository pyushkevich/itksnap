#include "PresetManager.h"
#include "SystemInterface.h"
#include "IRISException.h"
#include <algorithm>

template <class TManagedObjectTraits>
PresetManager<TManagedObjectTraits>
::PresetManager()
{
  m_System = NULL;
}

template <class TManagedObjectTraits>
void
PresetManager<TManagedObjectTraits>
::Initialize(SystemInterface *si)
{
  // Store the system interface pointer
  m_System = si;
  m_Category = TManagedObjectTraits::GetPresetCategoryName().c_str();

  // Create all the system presets
  m_PresetMap.clear();
  m_PresetSystem.clear();
  for(SystemPresetIterator i = TManagedObjectTraits::SystemPresetBegin();
      i != TManagedObjectTraits::SystemPresetEnd();
      i++)
    {
    std::string name = TManagedObjectTraits::GetSystemPresetName(i);
    ManagedTypePtr mtp = ManagedType::New();
    TManagedObjectTraits::SetToSystemPreset(mtp, i);
    m_PresetMap[name] = mtp;
    m_PresetSystem.push_back(name);
    }

  // Load all the user preset names
  m_PresetUser = m_System->GetSavedObjectNames(m_Category.c_str());

  // Load each of the presets from the registry
  for(int j = 0; j < m_PresetUser.size(); j++)
    {
    Registry reg = m_System->ReadSavedObject(m_Category.c_str(), m_PresetUser[j].c_str());
    ManagedTypePtr mtp = ManagedType::New();
    TManagedObjectTraits::ReadFromRegistry(mtp, reg);
    m_PresetMap[m_PresetUser[j]] = mtp;
    }

  this->Modified();
}

template <class TManagedObjectTraits>
typename PresetManager<TManagedObjectTraits>::PresetMatch
PresetManager<TManagedObjectTraits>
::QueryPreset(ManagedType *instance)
{
  // TODO: we currently employ brute force linear search. This is probably
  // just fine, but could be done more cleanly using some sort of a hash key
  std::vector<std::string>::const_iterator it;

  for(it = m_PresetSystem.begin(); it != m_PresetSystem.end(); it++)
    {
    if(*m_PresetMap[*it] == *instance)
      return std::make_pair(PRESET_SYSTEM, *it);
    }

  for(it = m_PresetUser.begin(); it != m_PresetUser.end(); it++)
    {
    if(*m_PresetMap[*it] == *instance)
      return std::make_pair(PRESET_USER, *it);
    }

  return std::make_pair(PRESET_NONE, std::string());
}

template <class TManagedObjectTraits>
void
PresetManager<TManagedObjectTraits>
::SetToPreset(ManagedType *instance, const std::string &preset)
{
  typename PresetMap::iterator it = m_PresetMap.find(preset);
  if(it == m_PresetMap.end())
    throw IRISException("Preset %s not found in category %s", preset.c_str(),
                        m_Category.c_str());
  instance->CopyInformation(it->second);
}

template <class TManagedObjectTraits>
void
PresetManager<TManagedObjectTraits>
::SaveAsPreset(ManagedType *instance, const std::string &preset)
{
  // Check that the name is not used for a system preset
  if(std::find(m_PresetSystem.begin(), m_PresetSystem.end(), preset) != m_PresetSystem.end())
    throw IRISException(
        "%s is not a valid user preset name. It conflicts with a system preset",
        preset.c_str());

  // Assign as a user preset
  if(std::find(m_PresetUser.begin(), m_PresetUser.end(), preset)  == m_PresetUser.end())
    m_PresetUser.push_back(preset);

  // Store the preset in memory
  ManagedTypePtr mtp = ManagedType::New();
  mtp->CopyInformation(instance);
  m_PresetMap[preset] = mtp;

  // Write the preset to disk
  Registry reg;
  TManagedObjectTraits::WriteToRegistry(instance, reg);
  m_System->UpdateSavedObject(m_Category.c_str(), preset.c_str(), reg);

  this->Modified();
}

template <class TManagedObjectTraits>
void
PresetManager<TManagedObjectTraits>
::DeletePreset(const std::string &preset)
{
  // Assign as a user preset
  std::vector<std::string>::iterator it =
      std::find(m_PresetUser.begin(), m_PresetUser.end(), preset);

  if(it != m_PresetUser.end())
    {
    // Delete the preset from the file system
    m_System->DeleteSavedObject(m_Category.c_str(), preset.c_str());

    // Also delete it from the list of user presets
    m_PresetUser.erase(it);
    }

  this->Modified();
}


template <class TManagedObjectTraits>
typename PresetManager<TManagedObjectTraits>::ManagedType *
PresetManager<TManagedObjectTraits>
::GetPreset(const std::string &preset)
{
  return m_PresetMap[preset];
}

template <class TManagedObjectTraits>
bool
PresetManager<TManagedObjectTraits>
::IsValidPreset(const std::string &preset)
{
  return m_PresetMap.find(preset) != m_PresetMap.end();
}
