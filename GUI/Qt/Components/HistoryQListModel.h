#ifndef HISTORYQLISTMODEL_H
#define HISTORYQLISTMODEL_H

#include "SNAPCommon.h"
#include <QAbstractListModel>
#include <vector>
#include <QDateTime>
#include <QIcon>
#include <QImage>
#include <QStandardItemModel>
#include <QStandardItem>

class EventBucket;
class GlobalUIModel;

class HistoryQListItem : public QObject, public QStandardItem
{
  Q_OBJECT
public:

  // Data role that is true when the item's file was not found
  static const int MissingFileRole = Qt::UserRole + 1;

  virtual void setItem(GlobalUIModel *model, const QString &history_entry);

protected slots:

  void onTimer();

  void onFileStatusChanged(QString path);

protected:

  // Set the thumbnail as the item's icon, with a badge for remote items and
  // for files that were not found
  void setThumbnail(const QPixmap &thumbnail);

  // Show whether the item's file exists
  void updateMissing();

  QString m_IconFilename;

  // The history entry and the last thumbnail set
  QString m_Path;
  QPixmap m_Thumbnail;

  // Whether the item is a remote URL
  bool m_IsRemote = false;

  // Whether the item's file was not found
  bool m_IsMissing = false;

};


/**
  QT model used to display an item from the image history as an entry in
  the table of recently loaded images
  */
class HistoryQListModel : public QStandardItemModel
{
  Q_OBJECT

public:
  explicit HistoryQListModel(QObject *parent = 0);

  void Initialize(GlobalUIModel *, const std::string &category);

public slots:

  void onModelUpdate(const EventBucket &);

protected:

  void rebuildModel();

  static void updateIcon(QStandardItem *item);

  // Need a pointer to the model
  GlobalUIModel *m_Model;

  // The name of the history
  std::string m_HistoryName;

  // Dummy icon
  QIcon m_DummyIcon;

  // List of standard items for concurrent code
  QList<QStandardItem *> m_ItemList;
};


#endif // HISTORYQLISTMODEL_H
