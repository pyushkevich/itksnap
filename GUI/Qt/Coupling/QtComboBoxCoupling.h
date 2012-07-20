/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: Filename.cxx,v $
  Language:  C++
  Date:      $Date: 2010/10/18 11:25:44 $
  Version:   $Revision: 1.12 $
  Copyright (c) 2011 Paul A. Yushkevich

  This file is part of ITK-SNAP

  ITK-SNAP is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

=========================================================================*/

#ifndef QTCOMBOBOXCOUPLING_H
#define QTCOMBOBOXCOUPLING_H

#include <QtWidgetCoupling.h>
#include "SNAPQtCommon.h"
#include <ColorLabel.h>
#include <QComboBox>

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

template <class TAtomic>
class ComboBoxRowTraitsBase
{
public:
  static void removeAll(QComboBox *w)
  {
    w->clear();
  }

  static int getNumberOfRows(QComboBox *w)
  {
    return w->count();
  }

  static TAtomic getValueInRow(QComboBox *w, int i)
  {
    return w->itemData(i).value<TAtomic>();
  }
};

/**
  Row traits for mapping the description of a color label into a row in a combo box
  */
class ColorLabelToComboBoxWidgetTraits : public ComboBoxRowTraitsBase<LabelType>
{
public:

  static void appendRow(QComboBox *w, LabelType label, const ColorLabel &cl)
  {
    // The description
    QString text(cl.GetLabel());

    // The color
    QColor fill(cl.GetRGB(0), cl.GetRGB(1), cl.GetRGB(2));

    // Icon based on the color
    QIcon ic = CreateColorBoxIcon(16, 16, fill);

    // Store all of these properties
    w->addItem(ic, text, QVariant(label));
    w->setItemData(w->count()-1, fill, Qt::UserRole + 1);
  }

  static void updateRowDescription(QComboBox *w, int index, const ColorLabel &cl)
  {
    // Get the properies and compare them to the color label
    QColor currentFill = w->itemData(index, Qt::UserRole + 1).value<QColor>();
    QColor newFill(cl.GetRGB(0), cl.GetRGB(1), cl.GetRGB(2));

    if(currentFill != newFill)
      {
      QIcon ic = CreateColorBoxIcon(16, 16, newFill);
      w->setItemIcon(index, ic);
      w->setItemData(index, newFill, Qt::UserRole + 1);
      }

    QString currentText = w->itemText(index);
    QString newText(cl.GetLabel());

    if(currentText != newText)
      {
      w->setItemText(index, newText);
      }
  }
};

#endif // QTCOMBOBOXCOUPLING_H
