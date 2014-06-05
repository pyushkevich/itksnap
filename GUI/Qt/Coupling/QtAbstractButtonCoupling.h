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

#ifndef QTABSTRACTBUTTONCOUPLING_H
#define QTABSTRACTBUTTONCOUPLING_H

#include <QtWidgetCoupling.h>
#include "SNAPQtCommon.h"
#include <QAbstractButton>
#include <QColorDialog>
#include <QColorButtonWidget.h>

/**
  Default traits for QColorButtonWidget, to be hooked up to a model of integer
  3-vector that specifies color in the 0-255 range
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

protected:
};

/**
  Alternative traits for QColorButtonWidget, to be hooked up to a model of double
  3-vector that specifies color in the 0-1 range
  */
template <>
class DefaultWidgetValueTraits< iris_vector_fixed<double, 3>, QColorButtonWidget>
    : public WidgetValueTraitsBase<iris_vector_fixed<double, 3>, QColorButtonWidget *>
{
public:

  typedef iris_vector_fixed<double, 3> ColorVec;

  // Get the Qt signal that the widget fires when its value has changed. The
  // value here is the selected item in the combo box.
  const char *GetSignal()
  {
    return SIGNAL(valueChanged());
  }

  ColorVec GetValue(QColorButtonWidget *w)
  {
    QColor qclr = w->value();
    return ColorVec(qclr.red() / 255.0, qclr.green() / 255.0, qclr.blue() / 255.0);
  }

  void SetValue(QColorButtonWidget *w, const ColorVec &value)
  {
    // We have to actually find the item
    Vector3i cint = to_int(value * 255.0);
    w->setValue(QColor(cint[0], cint[1], cint[2]));
  }

  void SetValueToNull(QColorButtonWidget *w)
  {
    w->setValue(QColor());
  }

protected:
};


/**
 * This traits class allows a color button widget to be hooked up to a model
 * with a domain. However, the domain will be ignored.
 */
template <class TAtomic>
class DefaultWidgetDomainTraits<NumericValueRange<TAtomic>, QColorButtonWidget>
    : public WidgetDomainTraitsBase<NumericValueRange<TAtomic>, QColorButtonWidget *>
{
public:
  typedef NumericValueRange<TAtomic> DomainType;
  virtual void SetDomain(QColorButtonWidget *, const DomainType &) { }
  virtual DomainType GetDomain(QColorButtonWidget *) { return DomainType(); }
};


/**
 * A simple coupling between a button and a boolean value.
 */
template <>
class DefaultWidgetValueTraits<bool, QAbstractButton>
    : public WidgetValueTraitsBase<bool, QAbstractButton *>
{
public:
  const char *GetSignal()
  {
    return SIGNAL(toggled(bool));
  }

  bool GetValue(QAbstractButton *w)
  {
    return w->isChecked();
  }

  void SetValue(QAbstractButton *w, const bool &value)
  {
    w->setChecked(value);
  }

  void SetValueToNull(QAbstractButton *w)
  {
    w->setChecked(false);
  }
};

#endif // QTABSTRACTBUTTONCOUPLING_H
