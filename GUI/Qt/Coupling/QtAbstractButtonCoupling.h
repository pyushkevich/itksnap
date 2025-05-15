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
#include <QPushButton>
#include <QMenu>
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
 * A simple coupling between a button and a true/false value strored in an int.
 */
template <typename TAtomic>
class DefaultWidgetValueTraits<
    TAtomic,
    QAbstractButton,
    typename std::enable_if<std::is_integral<TAtomic>::value>::type>
    : public WidgetValueTraitsBase<TAtomic, QAbstractButton *>
{
public:
  const char *GetSignal()
  {
    return SIGNAL(toggled(bool));
  }

  TAtomic GetValue(QAbstractButton *w)
  {
    return w->isChecked();
  }

  void SetValue(QAbstractButton *w, const TAtomic &value)
  {
    w->setChecked(value);
  }

  void SetValueToNull(QAbstractButton *w)
  {
    w->setChecked(false);
  }
};

/**
  These are the row traits for adding and updating rows in menus attached to buttons.
  This class is further parameterized by the class TItemDesriptionTraits, which is
  used to obtain the text and icon information from the value/description pairs
  provided by the model.
  */
template <class TAtomic>
class SimplePushButtonWithMenuRowTraits
{
public:
  static void removeAll(QPushButton *w)
  {
    w->menu()->clear();
  }

  static int getNumberOfRows(QPushButton *w)
  {
    return w->menu()->actions().count();
  }

  static TAtomic getValueInRow(QPushButton *w, int i)
  {
    return w->menu()->actions()[i]->data().value<TAtomic>();
  }

  static void appendRow(QPushButton *w, TAtomic label, const std::string &desc)
  {
    // The description
    QString text = from_utf8(desc);

    // Icon based on the color
    QAction *action = new QAction(w->menu());
    action->setData(QVariant::fromValue(label));
    action->setText(text);
    w->menu()->addAction(action);
  }

  static void updateRowDescription(QPushButton *w, int index, const std::string &desc)
  {
    // The current value
    QAction *action = w->menu()->actions()[index];
    TAtomic label = action->data().value<TAtomic>();

    // Get the properies and compare them to the color label
    QString currentText = action->text();
    QString newText = from_utf8(desc);
    if(currentText != newText)
      action->setText(newText);
  }
};


// Define the defaults
template <class TDomain>
class SimplePushButtonWithMenuDomainTraits
  : public ItemSetWidgetDomainTraits<
      TDomain, QPushButton,
      SimplePushButtonWithMenuRowTraits<typename TDomain::ValueType> >
{
};


/**
 * A coupling between a button with an attached menu and a set of items for
 * this menu.
 */
template <typename TAtomic>
class PushButtonWithMenuValueTraits
  : public WidgetValueTraitsBase<TAtomic, QPushButton *>
{
public:
  const char *GetSignal()
  {
    // return SIGNAL(toggled(bool));
  }

  TAtomic GetValue(QPushButton *w)
  {
    // return w->isChecked();
  }

  void SetValue(QPushButton *w, const TAtomic &value)
  {
    // w->setChecked(value);
  }

  void SetValueToNull(QPushButton *w)
  {
    // w->setChecked(false);
  }
};


#endif // QTABSTRACTBUTTONCOUPLING_H
