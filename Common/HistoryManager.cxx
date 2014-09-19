#include "HistoryManager.h"
#include "SystemInterface.h"
#include "Registry.h"
#include "itksys/SystemTools.hxx"
#include <algorithm>

const unsigned int HistoryManager::HISTORY_SIZE_LOCAL = 5;
const unsigned int HistoryManager::HISTORY_SIZE_GLOBAL = 20;

HistoryManager::HistoryManager()
{
}

HistoryManager::ConcreteHistoryModel
*HistoryManager::GetHistory(const std::string &category, HistoryMap &hmap)
{
  // Does the history exist?
  HistoryMap::iterator it = hmap.find(category);
  if(it == hmap.end())
    {
    ConcreteHistoryModelPtr model = ConcreteHistoryModel::New();
    hmap.insert(std::make_pair(category, model));
    return model;
    }
  else return it->second;
}

void HistoryManager
::SaveHistory(Registry &folder, HistoryManager::HistoryMap &hmap)
{
  // Write all the histories to the registry
  for(HistoryMap::iterator it = hmap.begin(); it != hmap.end(); it++)
    {
    ConcreteHistoryModel *model = it->second;
    folder.Folder(it->first).PutArray(model->GetValue());
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
    // Histories are created on demand - so we must call the get model method,
    // which will create and store the history model
    ConcreteHistoryModel *model = GetHistory(*it, hmap);
    model->SetValue(folder.Folder(*it).GetArray(std::string("")));
    }
}

void HistoryManager
::UpdateHistoryList(ConcreteHistoryModel *model, const std::string &file, unsigned int maxsize)
{
  // Get the list (passing by value here, but these lists are not huge)
  HistoryListType array = model->GetValue();

  // Remove all occurences of the file from the array
  array.erase(std::remove(array.begin(), array.end(), file), array.end());

  // Append the file to the end of the array
  array.push_back(file);

  // Trim the array to appropriate size
  if(array.size() > maxsize)
    array.erase(array.begin(), array.begin() + array.size() - maxsize);

  // Put the new array back in the model
  model->SetValue(array);
}

void HistoryManager::UpdateHistory(
    const std::string &category,
    const std::string &filename,
    bool make_local)
{
  // Create a string for the new file
  std::string fullpath = itksys::SystemTools::CollapseFullPath(filename.c_str());

  // Get the current history registry
  UpdateHistoryList(GetHistory(category, m_GlobalHistory), fullpath, HISTORY_SIZE_GLOBAL);
  if(make_local)
    UpdateHistoryList(GetHistory(category, m_LocalHistory), fullpath, HISTORY_SIZE_LOCAL);

  // TODO: right now, no events are fired to notify changes to the HistoryManager as
  // a whole. Also, there is no mechanism for sharing histories across sessions.
}

void HistoryManager::DeleteHistoryItem(
    const std::string &category, const std::string &file)
{
  // Delete all occurences of file from the history
  ConcreteHistoryModel *model = GetHistory(category, m_GlobalHistory);
  HistoryListType hist = model->GetValue();
  hist.erase(std::remove(hist.begin(), hist.end(), file), hist.end());
  model->SetValue(hist);
}

void HistoryManager::ClearLocalHistory()
{
  m_LocalHistory.clear();
}

HistoryManager::AbstractHistoryModel *
HistoryManager::GetLocalHistoryModel(const std::string &category)
{
  return GetHistory(category, m_LocalHistory);
}

HistoryManager::AbstractHistoryModel *
HistoryManager::GetGlobalHistoryModel(const std::string &category)
{
  return GetHistory(category, m_GlobalHistory);
}

HistoryManager::HistoryListType
HistoryManager::GetLocalHistory(const std::string &category)
{
  return GetHistory(category, m_LocalHistory)->GetValue();
}

HistoryManager::HistoryListType
HistoryManager::GetGlobalHistory(const std::string &category)
{
  return GetHistory(category, m_GlobalHistory)->GetValue();
}
