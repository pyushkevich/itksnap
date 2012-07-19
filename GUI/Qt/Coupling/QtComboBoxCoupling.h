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

/**
  Row traits for mapping the description of a color label into a row in a combo box
  */
class ColorLabelToComboBoxWidgetTraits
{
public:

  static void removeAll(QComboBox *w) { w->clear(); }

  static void appendRow(QComboBox *w, LabelType label, ColorLabel &cl)
  {
    QString text(cl.GetLabel());
    QIcon ic = CreateColorBoxIcon(16, 16,
          Vector3ui(cl.GetRGB(0), cl.GetRGB(1), cl.GetRGB(2)));
    w->addItem(ic, text, QVariant(label));
  }
};

#endif // QTCOMBOBOXCOUPLING_H
