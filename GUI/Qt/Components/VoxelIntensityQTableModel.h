#ifndef VOXELINTENSITYQTABLEMODEL_H
#define VOXELINTENSITYQTABLEMODEL_H

#include <QAbstractTableModel>
#include <RandomAccessCollectionModel.h>

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

template <class TItem>
class DefaultQtItemRowTraits
{
  virtual int GetColumnCount() = 0;
  virtual QVariant GetItemData(TItem &, int, Qt::ItemDataRole) = 0;
};


/**
  This class provides the interface between SNAP's random access collection
  model (i.e., a list of items with cached random access), and the Qt list
  and table widgets. This class relies on TItemRowTraits template parameter,
  which describes how to map objects of type TItem to the cells in the table.
  */
template <class TItem, class TItemRowTraits = DefaultQtItemRowTraits<TItem> >
class QtWrappedRandomAccessCollectionModel : public QAbstractItemModel
{
public:
  // This is the internal model that we provide a wrapping around
  typedef AbstractRandomAccessCollectionModel<TItem> WrappedModel;

  // Get and set the wrapped model
  irisGetSetMacro(WrappedModel, WrappedModel *)

  // The traits for this object do not have to be static. We allow the
  // user to pass it a traits object
  TItemRowTraits &GetTraits() { return m_Traits; }

  // Subclass standard functions
  QModelIndex index(int row, int column,
                    const QModelIndex &parent = QModelIndex()) const
  {
    return this->createIndex(row, column);
  }

  QModelIndex parent(const QModelIndex &child) const
  {
    return QModelIndex();
  }

  int rowCount(const QModelIndex &parent = QModelIndex()) const
  {
    return (int) m_WrappedModel->GetSize();
  }

  int columnCount(const QModelIndex &parent = QModelIndex()) const
  {
    return m_Traits.GetColumnCount();
  }

  QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const
  {
    // Get the right item
    TItem item = (*m_WrappedModel)[index.row()];
    return m_Traits.GetItemData(item, index.column(), role);
  }

protected:

  WrappedModel *m_WrappedModel;
  TItemRowTraits m_Traits;
};



#endif // VOXELINTENSITYQTABLEMODEL_H
