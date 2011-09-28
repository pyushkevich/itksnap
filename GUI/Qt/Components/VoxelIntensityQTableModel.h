#ifndef VOXELINTENSITYQTABLEMODEL_H
#define VOXELINTENSITYQTABLEMODEL_H

#include <QAbstractTableModel>

class GlobalUIModel;
class EventBucket;

class VoxelIntensityQTableModel : public QAbstractTableModel
{
  Q_OBJECT
public:
  explicit VoxelIntensityQTableModel(QObject *parent = 0);

  void SetParentModel(GlobalUIModel *model);

  int rowCount(const QModelIndex &parent = QModelIndex()) const ;
  int columnCount(const QModelIndex &parent = QModelIndex()) const;
  QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;

  QVariant headerData(int section, Qt::Orientation orientation, int role) const;

signals:

public slots:

  void onModelUpdate(const EventBucket &);

private:

  GlobalUIModel *m_Model;
};

#endif // VOXELINTENSITYQTABLEMODEL_H
