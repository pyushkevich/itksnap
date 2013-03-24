#include "HistoryManager.h"
#include "SystemInterface.h"
#include "Registry.h"
#include "itksys/SystemTools.hxx"

const unsigned int HistoryManager::HISTORY_SIZE_LOCAL = 5;
const unsigned int HistoryManager::HISTORY_SIZE_GLOBAL = 20;

HistoryManager::HistoryManager()
{
}

void HistoryManager
::SaveHistory(Registry &folder, HistoryManager::HistoryMap &hmap)
{
  // Write all the histories to the registry
  for(HistoryMap::iterator it = hmap.begin(); it != hmap.end(); it++)
    {
    folder.Folder(it->first).PutArray(it->second);
    }
}

void HistoryManager
::LoadHistory(Registry &folder, HistoryManager::HistoryMap &hmap)
{
  hmap.clear();

  // Read all the relevant histories from the file. We do this dynamically
  // although it would also have made sense to hard code here the list of
  // all histories. I guess it does not really matter.
  Registry::StringListType historyNames;
  folder.GetFolderKeys(historyNames);
  for(Registry::StringListType::iterator it = historyNames.begin();
      it != historyNames.end(); it++)
    {
    hmap[*it] = folder.Folder(*it).GetArray(std::string(""));
    }
}

void HistoryManager
::UpdateHistoryList(
    HistoryListType &array, const std::string &file, unsigned int maxsize)
{
  // First, search the history for the instance of the file and delete
  // existing occurences
  HistoryListType::iterator it;
  while((it = find(array.begin(),array.end(),file)) != array.end())
    array.erase(it);

  // Append the file to the end of the array
  array.push_back(file);

  // Trim the array to appropriate size
  if(array.size() > maxsize)
    array.erase(array.begin(),array.begin() + array.size() - maxsize);
}


void HistoryManager::UpdateHistory(
    const std::string &category,
    const std::string &filename,
    bool make_local)
{
  // Create a string for the new file
  std::string fullpath = itksys::SystemTools::CollapseFullPath(filename.c_str());

  // Get the current history registry
  UpdateHistoryList(m_GlobalHistory[category], fullpath, HISTORY_SIZE_GLOBAL);
  if(make_local)
    UpdateHistoryList(m_LocalHistory[category], fullpath, HISTORY_SIZE_LOCAL);

  // TODO: history is a property that should be preserved. We need to fire some
  // kind of an event to let someone know that the history has changed and that
  // it needs to be stored in a file and transferred to other ITK-SNAP processes
}

const HistoryManager::HistoryListType
&HistoryManager::GetLocalHistory(const std::string &category)
{
  return m_LocalHistory[category];
}

const HistoryManager::HistoryListType
&HistoryManager::GetGlobalHistory(const std::string &category)
{
  return m_GlobalHistory[category];
}
