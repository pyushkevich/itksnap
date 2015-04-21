#ifndef HISTORYQLISTMODEL_H
#define HISTORYQLISTMODEL_H

#include "SNAPCommon.h"
#include <QAbstractListModel>
#include <vector>
#include <QDateTime>
#include <QIcon>
#include <QImage>
#include <QRunnable>
#include <QMutex>

class EventBucket;
class GlobalUIModel;



class HistoryThumbnailLoader : public QObject
{
  Q_OBJECT
public:

  void setHistoryItem(const QString &item) { m_HistoryItem = item; }
  void setFileName(const QString &filename) { m_FileName = filename; }
  virtual void run();

signals:

  void dataLoaded(QString history_item, QImage image);

private:

  static QMutex mutex;
  QString m_HistoryItem, m_FileName;
};

class HistoryThumbnailLoaderRunnable : public QRunnable
{
public:
  HistoryThumbnailLoaderRunnable(HistoryThumbnailLoader *target)
    : m_Target(target) {}

  ~HistoryThumbnailLoaderRunnable() { delete m_Target; }

  virtual void run() { m_Target->run(); }
protected:
  HistoryThumbnailLoader *m_Target;
};

#include <QStandardItemModel>
#include <QStandardItem>

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

/**
  QT model used to display an item from the image history as an entry in
  the table of recently loaded images
  */
/*
class HistoryQListModel : public QAbstractListModel
{
  Q_OBJECT

public:
  explicit HistoryQListModel(QObject *parent = 0);

  virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;
  virtual QVariant data(const QModelIndex &index, int role) const;

  void Initialize(GlobalUIModel *, const std::string &category);

signals:
  
public slots:
  void onModelUpdate(const EventBucket &bucket);

  void onIconLoaded(QString hist_item, QImage icon);

protected:

  // Need a pointer to the model
  GlobalUIModel *m_Model;

  // The name of the history
  std::string m_HistoryName;

  // The cached value of the history - we keep it for speed
  struct HistoryEntry {
    QDateTime last_read_time;
    QIcon thumbnail;
    bool is_ready;

    HistoryEntry() : is_ready(false) {}
  };

  QIcon m_DummyIcon;

  std::vector<std::string> m_CachedHistory;
  typedef std::map<std::string, HistoryEntry> HistoryMap;
  HistoryMap m_CachedHistoryData;

  void updateCache();
  void updateHistoryEntry(std::string entry, HistoryEntry &entry_data);
};

*/

#endif // HISTORYQLISTMODEL_H
