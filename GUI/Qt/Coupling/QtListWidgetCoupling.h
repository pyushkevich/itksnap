#ifndef QTLISTWIDGETCOUPLING_H
#define QTLISTWIDGETCOUPLING_H

#include <QtWidgetCoupling.h>
#include "SNAPQtCommon.h"
#include <ColorLabel.h>
#include <QListWidget>


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
  Row traits for mapping a color label into a list widget entry
  */
class ColorLabelToListWidgetTraits
{
public:

  static void removeAll(QListWidget *w) { w->clear(); }

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
