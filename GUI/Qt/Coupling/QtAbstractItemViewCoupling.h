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


template <class TItemDomain, class TRowTraits>
class QStandardItemModelWidgetDomainTraits :
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

    model->clear();
    model->setColumnCount(TRowTraits::columnCount());

    // Populate
    for(typename DomainType::const_iterator it = domain.begin();
        it != domain.end(); ++it)
      {
      // Get the key/value pair
      AtomicType value = domain.GetValue(it);
      const DescriptorType &row = domain.GetDescription(it);

      // Use the row traits to map information to the widget
      QList<QStandardItem *> rlist;
      for(int j = 0; j < model->columnCount(); j++)
        {
        QStandardItem *item = new QStandardItem();
        TRowTraits::updateItem(item, j, value, row);
        rlist.append(item);
        }
      model->appendRow(rlist);
      }
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
      QStandardItem *item = model->item(i);
      AtomicType id = TRowTraits::getItemValue(item);
      typename DomainType::const_iterator it = domain.find(id);
      if(it != domain.end())
        {
        const DescriptorType &row = domain.GetDescription(it);
        for(int j = 0; j <  model->columnCount(); j++)
          TRowTraits::updateItem(model->item(i,j), j, id, row);
        }
      }
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

  static void updateItem(QStandardItem *item, int column, LabelType label, const ColorLabel &cl)
  {
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

  static LabelType getItemValue(QStandardItem *item)
  {
    return item->data(Qt::UserRole).value<LabelType>();
  }
};

class TwoColumnColorLabelToQSIMCouplingRowTraits
{
public:

  static int columnCount() { return 2; }

  static void updateItem(QStandardItem *item, int column, LabelType label, const ColorLabel &cl)
  {
    // Handle the timestamp - if the timestamp has not changed, don't need to update
    unsigned long ts = item->data(Qt::UserRole+1).toLongLong();
    if(ts == cl.GetTimeStamp().GetMTime())
      return;

    // The description
    if(column == 0)
      {
      QString text = QString("%1").arg(label);

      // The color
      QColor fill(cl.GetRGB(0), cl.GetRGB(1), cl.GetRGB(2));

      // Icon based on the color
      QIcon ic = CreateColorBoxIcon(16, 16, fill);

      // Create item and set its properties
      item->setIcon(ic);
      item->setText(text);
      item->setData(text, Qt::EditRole);
      item->setData(label, Qt::UserRole);
      }
    else if(column == 1)
      {
      QString text = QString::fromUtf8(cl.GetLabel());
      item->setText(text);
      item->setData(text, Qt::EditRole);
      }

    // Update the timestamp
    item->setData((qulonglong) cl.GetTimeStamp().GetMTime(), Qt::UserRole+1);
  }

  static LabelType getItemValue(QStandardItem *item)
  {
    return item->data(Qt::UserRole).value<LabelType>();
  }
};


template<class TAtomic, class TItemDescriptor>
class DefaultQSIMCouplingRowTraits
{
};

template<>
class DefaultQSIMCouplingRowTraits<LabelType, ColorLabel>
    : public TwoColumnColorLabelToQSIMCouplingRowTraits
{
};

template<class TDomain>
class DefaultWidgetDomainTraits<TDomain, QAbstractItemView>
    : public QStandardItemModelWidgetDomainTraits<
        TDomain, DefaultQSIMCouplingRowTraits<typename TDomain::ValueType,
                                              typename TDomain::DescriptorType> >
{
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
