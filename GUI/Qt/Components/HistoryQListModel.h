#ifndef HISTORYQLISTMODEL_H
#define HISTORYQLISTMODEL_H

#include "SNAPCommon.h"
#include <QAbstractListModel>

class EventBucket;
class GlobalUIModel;

/**
  QT model used to display an item from the image history as an entry in
  the table of recently loaded images
  */
class HistoryQListModel : public QAbstractListModel
{
  Q_OBJECT

public:
  explicit HistoryQListModel(QObject *parent = 0);

  virtual int rowCount(const QModelIndex &parent) const;
  virtual QVariant data(const QModelIndex &index, int role) const;

  irisGetSetMacro(Model, GlobalUIModel *)
  irisGetSetMacro(HistoryName, std::string)

signals:
  
public slots:
  void onModelUpdate(const EventBucket &bucket);

protected:

  // Need a pointer to the model
  GlobalUIModel *m_Model;

  // The name of the history
  std::string m_HistoryName;

};

#endif // HISTORYQLISTMODEL_H
