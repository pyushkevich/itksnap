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


  template <class TAtomic>
  class ListWidgetRowTraitsBase
  {
  public:
    static void removeAll(QListWidget *w)
    {
      w->clear();
    }

    static int getNumberOfRows(QListWidget *w)
    {
      return w->count();
    }

    static TAtomic getValueInRow(QListWidget *w, int i)
    {
      return w->item(i)->data(Qt::UserRole).value<TAtomic>();
    }
  };

  /**
    Row traits for mapping a color label into a list widget entry
    */
  class ColorLabelToListWidgetTraits
      : public ListWidgetRowTraitsBase<LabelType>
  {
  public:

    static void appendRow(QListWidget *w, LabelType label, const ColorLabel &cl)
    {
      // The description
      QString text(cl.GetLabel());

      // The color
      QColor fill(cl.GetRGB(0), cl.GetRGB(1), cl.GetRGB(2));

      // Icon based on the color
      QIcon ic = CreateColorBoxIcon(16, 16, fill);

      // Create item and set its properties
      QListWidgetItem *item = new QListWidgetItem(ic, text, w);
      item->setData(Qt::UserRole, label);
      item->setData(Qt::UserRole + 1, fill);
      item->setData(Qt::EditRole, text);
      // item->setFlags(item->flags() | Qt::ItemIsEditable);
    }

    static void updateRowDescription(QListWidget *w, int index, const ColorLabel &cl)
    {
      // Get the current item
      QListWidgetItem *item = w->item(index);

      // Get the properies and compare them to the color label
      QColor currentFill = item->data(Qt::UserRole+1).value<QColor>();
      QColor newFill(cl.GetRGB(0), cl.GetRGB(1), cl.GetRGB(2));

      if(currentFill != newFill)
        {
        QIcon ic = CreateColorBoxIcon(16, 16, newFill);
        item->setIcon(ic);
        item->setData(Qt::UserRole + 1, newFill);
        }

      QString currentText = item->text();
      QString newText(cl.GetLabel());

      if(currentText != newText)
        {
        item->setText(newText);
        item->setData(Qt::EditRole, newText);
        }
    }
  };

  // Define the defaults
  template <class TDomain>
  class DefaultWidgetDomainTraits<TDomain, QListWidget>
      : public ItemSetWidgetDomainTraits<TDomain, QListWidget, ColorLabelToListWidgetTraits>
  {
  };


  #endif // QTLISTWIDGETCOUPLING_H
