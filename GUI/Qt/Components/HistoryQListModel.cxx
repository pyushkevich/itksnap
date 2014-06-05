#include "HistoryQListModel.h"
#include "HistoryManager.h"
#include "SystemInterface.h"
#include "IRISApplication.h"
#include "GlobalUIModel.h"
#include <itksys/SystemTools.hxx>
#include <QIcon>
#include "SNAPQtCommon.h"
#include "LatentITKEventNotifier.h"

HistoryQListModel::HistoryQListModel(QObject *parent) :
  QAbstractListModel(parent)
{
  m_Model = NULL;
}

int HistoryQListModel::rowCount(const QModelIndex &parent) const
{
  // Display at most 12 entries in the history
  return std::min((size_t) 12, m_CachedHistory.size());
}

QVariant HistoryQListModel::data(const QModelIndex &index, int role) const
{
  // Get the entry
  std::string item = m_CachedHistory[m_CachedHistory.size() - (1 + index.row())];

  // Display the appropriate item
  if(role == Qt::DisplayRole)
    {
    // Get the shorter filename
    std::string shorty = itksys::SystemTools::GetFilenameName(item.c_str());
    return from_utf8(shorty);
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
  else if(role == Qt::ToolTipRole || role == Qt::UserRole)
    {
    return from_utf8(item.c_str());
    }
  return QVariant();

}

void HistoryQListModel::Initialize(
    GlobalUIModel *model, const std::string &category)
{
  m_Model = model;
  m_HistoryName = category;

  // Get the property models for the local and global histories
  HistoryManager::AbstractHistoryModel *hmodel =
      m_Model->GetDriver()->GetHistoryManager()->GetGlobalHistoryModel(category);

  // Listen for updates from the history model
  LatentITKEventNotifier::connect(hmodel, ValueChangedEvent(),
                                  this, SLOT(onModelUpdate(EventBucket)));

  // Cache the history
  m_CachedHistory = hmodel->GetValue();
}

void HistoryQListModel::onModelUpdate(const EventBucket &bucket)
{
  this->beginResetModel();

  // When history changes, we update
  m_CachedHistory =
      m_Model->GetDriver()->GetHistoryManager()->GetGlobalHistory(m_HistoryName);

  this->endResetModel();
}
