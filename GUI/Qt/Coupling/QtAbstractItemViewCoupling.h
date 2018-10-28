#ifndef QTABSTRACTITEMVIEWCOUPLING_H
#define QTABSTRACTITEMVIEWCOUPLING_H

#include <QtWidgetCoupling.h>
#include "SNAPQtCommon.h"
#include <QAbstractItemModel>
#include <QItemSelectionModel>
#include <QAbstractItemView>
#include <QModelIndex>
#include <QStandardItemModel>
#include <QAbstractProxyModel>
#include <ColorLabel.h>

#include <QTableView>
#include <QHeaderView>
#include <QTimer>

/**
 * Some Ideas:
 *
 * 1. Coupling between a selection model and an integer value. Treat the
 *    domain as trivial.
 *
 * 2. Coupling between a model that represents a list of items with descriptors
 *    and a QStandardItemModel. Mapping is through a coupling. This coupling
 *    maps between rows in the descriptor and items in the item model.
 */



/**
 * The default value traits for QAbstractItemView use the currentRow in the item view
 * to represent a value. Each row is assigned a value of class TAtomic, and the value
 * of the current row is considered to be the value of the widget.
 *
 * in other words, the model holds an item of type TAtomic and the widget selects a
 * corresponding row in the widget
 */
template <class TAtomic>
class DefaultWidgetValueTraits<TAtomic, QAbstractItemView>
    : public WidgetValueTraitsBase<TAtomic, QAbstractItemView *>
{
public:
  const char *GetSignal()
  {
    return SIGNAL(currentChanged(const QModelIndex &, const QModelIndex &));
  }

  virtual QObject *GetSignalEmitter(QObject *w)
  {
    QAbstractItemView *view = dynamic_cast<QAbstractItemView *>(w);
    return view ? view->selectionModel() : NULL;
  }

  TAtomic GetValue(QAbstractItemView *w)
  {
    // Get the UserData associated with the current item    
    QModelIndex icur = w->currentIndex();
    QModelIndex irow = w->model()->index(icur.row(), 0, w->currentIndex().parent());
    return irow.data(Qt::UserRole).value<TAtomic>();
  }

  bool FindRowRecursive(QAbstractItemView *w, QModelIndex parent, const TAtomic &value)
  {
    for(int i = 0; i < w->model()->rowCount(parent); i++)
      {
      QModelIndex index = w->model()->index(i, 0, parent);
      TAtomic val = w->model()->data(index, Qt::UserRole).value<TAtomic>();
      if(val == value)
        {
        w->setCurrentIndex(index);
        return true;
        }
      else if(FindRowRecursive(w, index, value))
        {
        return true;
        }
      }
    return false;
  }

  void SetValue(QAbstractItemView *w, const TAtomic &value)
  {
    // Find the item in the model
    FindRowRecursive(w, QModelIndex(), value);
  }

  void SetValueToNull(QAbstractItemView *w)
  {
    QModelIndex index = w->model()->index(-1, 0);
    w->setCurrentIndex(index);
  }
};


/**
 * An alternative value traits for QAbstractItemView uses the selection of all rows in
 * the view to represent a value. Thus value here is map from an atomic type to boolean,
 * where the boolean stores the selection state.
 *
 * Under this coupling, the model stores a list of on/off items, and the user can use
 * multiple selection to modify this list.
 */
template<class TItemIndex>
class DefaultWidgetValueTraits< std::map<TItemIndex, bool>, QAbstractItemView>
    : public WidgetValueTraitsBase<std::map<TItemIndex, bool>, QAbstractItemView *>
{
public:
  typedef std::map<TItemIndex, bool> AtomicType;

  const char *GetSignal()
  {
    return SIGNAL(selectionChanged(const QItemSelection &, const QItemSelection &));
  }

  virtual QObject *GetSignalEmitter(QObject *w)
  {
    QAbstractItemView *view = dynamic_cast<QAbstractItemView *>(w);
    return view ? view->selectionModel() : NULL;
  }

  void ScanValuesRecursive(QAbstractItemView *w, QModelIndex parent, AtomicType &result)
  {
    for(int i = 0; i < w->model()->rowCount(parent); i++)
      {
      QModelIndex index = w->model()->index(i, 0, parent);
      TItemIndex val = w->model()->data(index, Qt::UserRole).value<TItemIndex>();
      result[val] = w->selectionModel()->isSelected(index);
      ScanValuesRecursive(w, index, result);
      }
  }

  void SetValuesRecursive(QAbstractItemView *w, QModelIndex parent, const AtomicType &mapping)
  {
    for(int i = 0; i < w->model()->rowCount(parent); i++)
      {
      QModelIndex index = w->model()->index(i, 0, parent);
      TItemIndex val = w->model()->data(index, Qt::UserRole).value<TItemIndex>();
      typename AtomicType::const_iterator it = mapping.find(val);
      if(it != mapping.end())
        w->selectionModel()->select(index, it->second ? QItemSelectionModel::Select
                                                      : QItemSelectionModel::Deselect);

      SetValuesRecursive(w, index, mapping);
      }
  }

  AtomicType GetValue(QAbstractItemView *w)
  {
    AtomicType value_map;
    ScanValuesRecursive(w, QModelIndex(), value_map);
    return value_map;
  }

  void SetValue(QAbstractItemView *w, const AtomicType &value)
  {
    // Find the item in the model
    SetValuesRecursive(w, QModelIndex(), value);
  }

  void SetValueToNull(QAbstractItemView *w)
  {
    if(w->selectionModel())
      w->selectionModel()->clear();
  }
};


template <class TItemDomain, class TRowTraits>
class DefaultMultiRowWidgetDomainTraits<TItemDomain, QAbstractItemView, TRowTraits> :
    public WidgetDomainTraitsBase<TItemDomain, QAbstractItemView *>
{
public:
  // The information about the item type are taken from the domain
  typedef typename TItemDomain::ValueType AtomicType;
  typedef typename TItemDomain::DescriptorType DescriptorType;
  typedef TItemDomain DomainType;

  // Navigate through proxy models until we find a standard item model in the
  // view. Allows couplings to be installed on views with proxies
  QStandardItemModel *GetTopLevelModel(QAbstractItemView *w)
  {
    QAbstractItemModel *model = w->model();
    while(model)
      {
      QStandardItemModel *msi = dynamic_cast<QStandardItemModel *>(model);
      if(msi)
        return msi;

      QAbstractProxyModel *mpx = dynamic_cast<QAbstractProxyModel *>(model);
      if(mpx)
        model = mpx->sourceModel();
      }
    return NULL;
  }

  void SetDomain(QAbstractItemView *w, const DomainType &domain)
  {
    // Remove everything from the model
    QStandardItemModel *model = GetTopLevelModel(w);
    if(!model)
      return;

    model->removeRows(0, model->rowCount());

    // Populate
    for(typename DomainType::const_iterator it = domain.begin();
        it != domain.end(); ++it)
      {
      // Get the key/value pair
      AtomicType value = domain.GetValue(it);
      const DescriptorType &row_desc = domain.GetDescription(it);

      // Use the row traits to map information to the widget
      QList<QStandardItem *> row_items;
      for(int j = 0; j < model->columnCount(); j++)
        row_items.append(new QStandardItem());
      TRowTraits::updateRow(row_items, value, row_desc);
      model->appendRow(row_items);
      }

#if QT_VERSION >= 0x050000
    // I am not sure why this is necessary - the model should automatically be sending signals when its
    // contents are being changed. Something must be amiss.
    emit model->dataChanged(model->index(0,0), model->index(model->rowCount()-1, model->columnCount()-1));
#endif
  }

  void UpdateDomainDescription(QAbstractItemView *w, const DomainType &domain)
  {
    // Remove everything from the model
    QStandardItemModel *model = GetTopLevelModel(w);
    if(!model)
      return;

    // This is not the most efficient way of doing things, because we
    // are still linearly parsing through the widget and updating rows.
    // But at least the actual modifications to the widget are limited
    // to the rows that have been modified.
    //
    // What would be more efficient is to have a list of ids which have
    // been modified and update only those. Or even better, implement all
    // of this using an AbstractItemModel
    int nrows = model->rowCount();
    for(int i = 0; i < nrows; i++)
      {
      // Collect the items for this row
      QList<QStandardItem *> row_items;
      for(int j = 0; j <  model->columnCount(); j++)
        row_items.append(model->item(i,j));

      // Get the value associated with this row
      AtomicType id = TRowTraits::getRowValue(row_items);

      // Find the row in the model and update the items
      typename DomainType::const_iterator it = domain.find(id);
      if(it != domain.end())
        TRowTraits::updateRow(row_items, id, domain.GetDescription(it));
      }

#if QT_VERSION >= 0x050000
    emit model->dataChanged(model->index(0,0), model->index(model->rowCount()-1, model->columnCount()-1));
#endif
  }

  TItemDomain GetDomain(QAbstractItemView *w)
  {
    // We don't actually pull the widget because the domain is fully specified
    // by the model.
    return DomainType();
  }
};

class SingleColumnColorLabelToQSIMCouplingRowTraits
{
public:

  static int columnCount() { return 1; }

  static void updateRow(QList<QStandardItem *> items, LabelType label, const ColorLabel &cl)
  {
    QStandardItem *item = items[0];

    // Handle the timestamp - if the timestamp has not changed, don't need to update
    unsigned long ts = item->data(Qt::UserRole+1).toLongLong();
    if(ts == cl.GetTimeStamp().GetMTime())
      return;

    // The description
    QString text = QString::fromUtf8(cl.GetLabel());

    // The color
    QColor fill(cl.GetRGB(0), cl.GetRGB(1), cl.GetRGB(2));

    // Icon based on the color
    QIcon ic = CreateColorBoxIcon(16, 16, fill);

    // Create item and set its properties
    item->setIcon(ic);
    item->setText(text);
    item->setData(label, Qt::UserRole);
    item->setData(text, Qt::EditRole);
    item->setData((qulonglong) cl.GetTimeStamp().GetMTime(), Qt::UserRole+1);
  }

  static LabelType getRowValue(QList<QStandardItem *> items)
  {
    return items[0]->data(Qt::UserRole).value<LabelType>();
  }
};

class TwoColumnColorLabelToQSIMCouplingRowTraits
{
public:

  static int columnCount() { return 2; }

  static void updateRow(QList<QStandardItem *> items, LabelType label, const ColorLabel &cl)
  {
    // Handle the timestamp - if the timestamp has not changed, don't need to update
    unsigned long ts = items[0]->data(Qt::UserRole+1).toLongLong();
    if(ts == cl.GetTimeStamp().GetMTime())
      return;

    // The description
    QString id_text = QString("%1").arg(label);

    // The color
    QColor fill(cl.GetRGB(0), cl.GetRGB(1), cl.GetRGB(2));

    // Icon based on the color
    QIcon ic = CreateColorBoxIcon(16, 16, fill);

    // Create item and set its properties
    items[0]->setIcon(ic);
    items[0]->setText(id_text);
    items[0]->setData(id_text, Qt::EditRole);
    items[0]->setData(label, Qt::UserRole);

    // Update the timestamp
    items[0]->setData((qulonglong) cl.GetTimeStamp().GetMTime(), Qt::UserRole+1);

    // Description
    QString desc_text = QString::fromUtf8(cl.GetLabel());
    items[1]->setText(desc_text);
    items[1]->setData(desc_text, Qt::EditRole);

  }

  static LabelType getRowValue(QList<QStandardItem *> items)
  {
    return items[0]->data(Qt::UserRole).value<LabelType>();
  }
};


template<>
class DefaultWidgetDomainTraits<TrivialDomain, QAbstractItemView>
{
public:
  void SetDomain(QAbstractItemView *w, const TrivialDomain &domain) {}
  TrivialDomain GetDomain(QAbstractItemView *w) { return TrivialDomain(); }
  void UpdateDomainDescription(QAbstractItemView *w, const TrivialDomain &domain) {}
};


#endif // QTABSTRACTITEMVIEWCOUPLING_H
