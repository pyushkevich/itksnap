#ifndef QTTABLEWIDGETCOUPLING_H
#define QTTABLEWIDGETCOUPLING_H

#ifndef QTLISTWIDGETCOUPLING_H
#define QTLISTWIDGETCOUPLING_H

#include <QtWidgetCoupling.h>
#include "SNAPQtCommon.h"
#include <ColorLabel.h>
#include <QTableWidget>


/**
  Default traits for mapping a numeric value (or any sort of key, actually)
  to a row in a table. This is similar to a list box, but with multiple
  columns. TODO: unify this with ListWidget traits by using a Qt
  AbstractItemModel
  */
template <class TAtomic>
class DefaultWidgetValueTraits<TAtomic, QTableWidget>
    : public WidgetValueTraitsBase<TAtomic, QTableWidget *>
{
public:
  // Get the Qt signal that the widget fires when its value has changed. The
  // value here is the selected item in the combo box.
  const char *GetSignal()
  {
    return SIGNAL(currentItemChanged(QTableWidgetItem *, QTableWidgetItem *));
  }

  TAtomic GetValue(QTableWidget *w)
  {
    // Get the UserData associated with the current item
    TAtomic val = w->currentItem()->data(Qt::UserRole).value();
    return val;
  }

  void SetValue(QTableWidget *w, const TAtomic &value)
  {
    // We have to actually find the row
    for(int i = 0; i < w->rowCount(); i++)
      {
      QTableWidgetItem *item = w->item(i, 0);
      TAtomic stored_value = item->data(Qt::UserRole).value();
      if(stored_value == value)
        {
        w->setCurrentItem(item);
        return;
        }
      }

    // Have we found it?
    w->setCurrentItem(NULL);
  }

  void SetValueToNull(QTableWidget *w)
  {
    // Have we found it?
    w->setCurrentItem(NULL);
  }
};


template <class TAtomic, class TDesc, class TItemDesriptionTraits>
class TextAndIconTableWidgetRowTraits
{
public:
  static void removeAll(QTableWidget *w)
  {
    while(w->rowCount())
      w->removeRow(0);
  }

  static int getNumberOfRows(QTableWidget *w)
  {
    return w->rowCount();
  }

  static TAtomic getValueInRow(QTableWidget *w, int i)
  {
    TAtomic value = w->item(i,0)->data(Qt::UserRole).value();
    return value;
  }

  static void appendRow(QTableWidget *w, TAtomic value, const TDesc &desc)
  {
    // Insert a row
    int row = w->rowCount();
    w->insertRow(row);

    // Fill all the columns
    for(int col = 0; col < w->columnCount(); col++)
      {
      // The description
      QString text = TItemDesriptionTraits::GetText(value, desc, col);

      // The icon
      QIcon icon = TItemDesriptionTraits::GetIcon(value, desc, col);

      // The icon signature - a value that can be used to check if the icon has changed
      QVariant iconSig = TItemDesriptionTraits::GetIconSignature(value, desc, col);

      // Set the item properties
      QTableWidgetItem *item = new QTableWidgetItem();
      item->setText(text);
      item->setIcon(icon);
      item->setData(Qt::UserRole + 1, iconSig);
      w->setItem(row, col, item);
      }

    // Set the item for the row
    w->item(row, 0)->setData(Qt::UserRole, QVariant::fromValue(value));
  }

  static void updateRowDescription(QTableWidget *w, int row, const TDesc &desc)
  {
    // The current value
    TAtomic value = w->item(row, 0)->data(Qt::UserRole).value();

    // Get the properies and compare them to the color label
    for(int col = 0; col < w->columnCount(); col++)
      {
      QTableWidgetItem *item = w->item(row, col);

      QVariant currentIconSig = item->data(Qt::UserRole + 1);
      QVariant newIconSig = TItemDesriptionTraits::GetIconSignature(value, desc, col);

      if(currentIconSig != newIconSig)
        {
        QIcon ic = TItemDesriptionTraits::GetIcon(value, desc, col);
        item->setIcon(ic);
        item->setData(Qt::UserRole + 1, newIconSig);
        }

      QString currentText = item->text();
      QString newText = TItemDesriptionTraits::GetText(value, desc, col);

      if(currentText != newText)
        {
        item->setText(newText);
        }
      }
  }
};






/**
  Row traits for mapping a color label into a list widget entry

class ColorLabelToListWidgetTraits
    : public ListWidgetRowTraitsBase<LabelType>
{
public:

  static void appendRow(QTableWidget *w, LabelType label, const ColorLabel &cl)
  {
    // The description
    QString text(cl.GetLabel());

    // The color
    QColor fill(cl.GetRGB(0), cl.GetRGB(1), cl.GetRGB(2));

    // Icon based on the color
    QIcon ic = CreateColorBoxIcon(16, 16, fill);

    // Create item and set its properties
    QTableWidgetItem *item = new QTableWidgetItem(ic, text, w);
    item->setData(Qt::UserRole, label);
    item->setData(Qt::UserRole + 1, fill);
    item->setData(Qt::EditRole, text);
    // item->setFlags(item->flags() | Qt::ItemIsEditable);
  }

  static void updateRowDescription(QTableWidget *w, int index, const ColorLabel &cl)
  {
    // Get the current item
    QTableWidgetItem *item = w->item(index);

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
class DefaultWidgetDomainTraits<TDomain, QTableWidget>
    : public ItemSetWidgetDomainTraits<TDomain, QTableWidget, ColorLabelToListWidgetTraits>
{
};

*/

#endif // QTLISTWIDGETCOUPLING_H


#endif // QTTABLEWIDGETCOUPLING_H
