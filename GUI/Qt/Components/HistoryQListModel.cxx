#include "HistoryQListModel.h"
#include "HistoryManager.h"
#include "SystemInterface.h"
#include "IRISApplication.h"
#include "GlobalUIModel.h"
#include <itksys/SystemTools.hxx>
#include <QIcon>


HistoryQListModel::HistoryQListModel(QObject *parent) :
  QAbstractListModel(parent)
{
  m_Model = NULL;
}

int HistoryQListModel::rowCount(const QModelIndex &parent) const
{
  HistoryManager *hm = m_Model->GetDriver()->GetSystemInterface()->GetHistoryManager();
  const HistoryManager::HistoryListType &history = hm->GetGlobalHistory(m_HistoryName.c_str());

  // Display at most 12 entries in the history
  return std::min((size_t) 12, history.size());
}

QVariant HistoryQListModel::data(const QModelIndex &index, int role) const
{
  // Get the history
  HistoryManager *hm = m_Model->GetDriver()->GetSystemInterface()->GetHistoryManager();
  const HistoryManager::HistoryListType &history = hm->GetGlobalHistory(m_HistoryName.c_str());

  // Get the entry
  std::string item = history[history.size() - (1 + index.row())];

  // Display the appropriate item
  if(role == Qt::DisplayRole)
    {
    // Get the shorter filename
    std::string shorty = itksys::SystemTools::GetFilenameName(item.c_str());
    return QString(shorty.c_str());
    }
  else if(role == Qt::DecorationRole)
    {
    // Need to get an icon!
    std::string iconfile = m_Model->GetDriver()->GetSystemInterface()
        ->GetThumbnailAssociatedWithFile(item.c_str());

    // Need to load the icon
    QIcon icon;
    icon.addFile(iconfile.c_str());
    return icon;
    }
  else if(role == Qt::ToolTipRole)
    {
    return QString(item.c_str());
    }
  return QVariant();

}

void HistoryQListModel::onModelUpdate(const EventBucket &bucket)
{
  if(bucket.HasEvent(MainImageDimensionsChangeEvent()))
    {
    this->reset();
    }
}
