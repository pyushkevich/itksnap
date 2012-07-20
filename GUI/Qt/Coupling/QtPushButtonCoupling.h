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

#ifndef QTPUSHBUTTONCOUPLING_H
#define QTPUSHBUTTONCOUPLING_H

#include <QtWidgetCoupling.h>
#include "SNAPQtCommon.h"
#include <QAbstractButton>
#include <QColorDialog>
#include <QColorButtonWidget.h>

/**
  Default traits for QColorButtonWidget. TColorRep should be an integer type.
  */
template <class TColorRep>
class DefaultWidgetValueTraits< iris_vector_fixed<TColorRep, 3>, QColorButtonWidget>
    : public WidgetValueTraitsBase<iris_vector_fixed<TColorRep, 3>, QColorButtonWidget *>
{
public:

  typedef iris_vector_fixed<TColorRep, 3> ColorVec;

  // Get the Qt signal that the widget fires when its value has changed. The
  // value here is the selected item in the combo box.
  const char *GetSignal()
  {
    return SIGNAL(valueChanged());
  }

  ColorVec GetValue(QColorButtonWidget *w)
  {
    QColor qclr = w->value();
    return ColorVec(qclr.red(), qclr.green(), qclr.blue());
  }

  void SetValue(QColorButtonWidget *w, const ColorVec &value)
  {
    // We have to actually find the item
    w->setValue(QColor(value[0],value[1],value[2]));
  }

  void SetValueToNull(QColorButtonWidget *w)
  {
    w->setValue(QColor());
  }
};


#endif // QTPUSHBUTTONCOUPLING_H
