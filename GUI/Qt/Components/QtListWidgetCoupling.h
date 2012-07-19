#ifndef QTLISTWIDGETCOUPLING_H
#define QTLISTWIDGETCOUPLING_H

#include <PropertyModel.h>
#include <QComboBox>
#include <QListWidget>
#include <QtWidgetCoupling.h>
#include "SNAPQtCommon.h"

#include <ColorLabel.h>

/**
  Default traits for mapping a numeric value (or any sort of key, actually)
  to a row in a combo box
  */
template <class TAtomic>
class DefaultWidgetValueTraits<TAtomic, QComboBox>
    : public WidgetValueTraitsBase<TAtomic, QComboBox *>
{
public:
  // Get the Qt signal that the widget fires when its value has changed. The
  // value here is the selected item in the combo box.
  const char *GetSignal()
  {
    return SIGNAL(currentIndexChanged(int));
  }

  TAtomic GetValue(QComboBox *w)
  {
    int index = w->currentIndex();
    QVariant id = w->itemData(index);
    return id.value<TAtomic>();
  }

  void SetValue(QComboBox *w, const TAtomic &value)
  {
    // We have to actually find the item
    w->setCurrentIndex(w->findData(QVariant(value)));
  }

  void SetValueToNull(QComboBox *w)
  {
    w->setCurrentIndex(-1);
  }
};


/**
  Default traits for mapping a numeric value (or any sort of key, actually)
  to a row in a list box
  */
template <class TAtomic>
class DefaultWidgetValueTraits<TAtomic, QListWidget>
    : public WidgetValueTraitsBase<TAtomic, QListWidget *>
{
public:
  // Get the Qt signal that the widget fires when its value has changed. The
  // value here is the selected item in the combo box.
  const char *GetSignal()
  {
    return SIGNAL(currentItemChanged(QListWidgetItem *, QListWidgetItem *));
  }

  TAtomic GetValue(QListWidget *w)
  {
    // Get the UserData associated with the current item
    return w->currentItem()->data(Qt::UserRole).value<TAtomic>();
  }

  void SetValue(QListWidget *w, const TAtomic &value)
  {
    // Unset the current index
    int row = -1;

    // We have to actually find the item
    for(int i = 0; i < w->count(); i++)
      {
      QModelIndex idx = w->model()->index(i, 0);
      if(w->model()->data(idx, Qt::UserRole).value<TAtomic>() == value)
        {
        row = i;
        break;
        }
      }

    // Have we found it?
    w->setCurrentRow(row);
  }

  void SetValueToNull(QListWidget *w)
  {
    w->setCurrentRow(-1);
  }
};


/**
  Traits for populating a combo box with a set of options from which the user
  chooses a single option. These traits are parameterized by a TRowTraits
  object, which provides the traits for mapping a description of one of the
  options to the information (icon/text) actually shown in the widget.

  The TItemDomain parameter needs to be a subclass of AbstractItemSetDomain
  or provide the same signature.
  */
template <class TItemDomain, class TWidget, class TRowTraits>
class ItemSetWidgetDomainTraits :
    public WidgetDomainTraitsBase<TItemDomain, TWidget *>
{
public:

  // The information about the item type are taken from the domain
  typedef typename TItemDomain::ValueType AtomicType;
  typedef typename TItemDomain::DescriptorType DescriptorType;
  typedef TItemDomain DomainType;

  void SetDomain(TWidget *w, const DomainType &domain)
  {
    // Clear the combo box
    w->clear();

    // This is where we actually populate the widget
    for(typename DomainType::const_iterator it = domain.begin();
        it != domain.end(); it++)
      {
      // Get the key/value pair
      AtomicType value = domain.GetValue(it);
      DescriptorType row = domain.GetDescription(it);

      // Use the row traits to map information to the widget
      TRowTraits::appendRow(w, value, row);
      }
  }

  TItemDomain GetDomain(TWidget *w)
  {
    // We don't actually pull the widget because the domain is fully specified
    // by the model.
    return DomainType();
  }
};

/**
  Row traits for mapping a color label into a combo box entry
  */
class ColorLabelToComboBoxWidgetTraits
{
public:
  static void appendRow(QComboBox *w, LabelType label, ColorLabel &cl)
  {
    QString text(cl.GetLabel());
    QIcon ic = CreateColorBoxIcon(16, 16,
          Vector3ui(cl.GetRGB(0), cl.GetRGB(1), cl.GetRGB(2)));
    w->addItem(ic, text, QVariant(label));
  }
};


/**
  Row traits for mapping a color label into a combo box entry
  */
class ColorLabelToListWidgetTraits
{
public:
  static void appendRow(QListWidget *w, LabelType label, ColorLabel &cl)
  {
    QString text(cl.GetLabel());
    QIcon ic = CreateColorBoxIcon(16, 16,
          Vector3ui(cl.GetRGB(0), cl.GetRGB(1), cl.GetRGB(2)));
    QListWidgetItem *item = new QListWidgetItem(ic, text, w);
    item->setData(Qt::UserRole, label);
  }
};


#endif // QTLISTWIDGETCOUPLING_H
