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

  virtual void setItem(GlobalUIModel *model, const QString &history_entry);

protected slots:

  void onTimer();

protected:

  QString m_IconFilename;

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

  void onModelUpdate(const EventBucket &bucket);

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
