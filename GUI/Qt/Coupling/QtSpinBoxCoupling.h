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

#ifndef QTSPINBOXCOUPLING_H
#define QTSPINBOXCOUPLING_H

#include <QtWidgetCoupling.h>
#include <QSpinBox>

/**
  Default traits for the Qt Spin Box, coupled with a numeric value range
  */
template <class TAtomic>
class DefaultWidgetValueTraits<TAtomic, QSpinBox>
    : public WidgetValueTraitsBase<TAtomic, QSpinBox *>
{
public:
  const char *GetSignal()
  {
    return SIGNAL(valueChanged(int));
  }

  TAtomic GetValue(QSpinBox *w)
  {
    return static_cast<TAtomic>(w->value());
  }

  void SetValue(QSpinBox *w, const TAtomic &value)
  {
    w->setSpecialValueText("");
    w->setValue(static_cast<int>(value));
  }


  void SetValueToNull(QSpinBox *w)
  {
    w->setValue(w->minimum());
    w->setSpecialValueText(" ");
  }

};

template <class TAtomic>
class DefaultWidgetDomainTraits<NumericValueRange<TAtomic>, QSpinBox>
    : public WidgetDomainTraitsBase<NumericValueRange<TAtomic>, QSpinBox *>
{
public:

  void SetDomain(QSpinBox *w, const NumericValueRange<TAtomic> &range)
  {
    w->setMinimum(range.Minimum);
    w->setMaximum(range.Maximum);
    w->setSingleStep(range.StepSize);
  }

  NumericValueRange<TAtomic> GetDomain(QSpinBox *w)
  {
    return NumericValueRange<TAtomic>(
          static_cast<TAtomic>(w->minimum()),
          static_cast<TAtomic>(w->maximum()),
          static_cast<TAtomic>(w->singleStep()));
  }
};

#endif // QTSPINBOXCOUPLING_H
