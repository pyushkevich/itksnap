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

#ifndef QTDOUBLESLIDERWITHEDITORCOUPLING_H
#define QTDOUBLESLIDERWITHEDITORCOUPLING_H

#include <QtWidgetCoupling.h>
#include <QDoubleSliderWithEditor.h>

/**
  Default traits for the Qt Double Spin Box
  */
template <class TAtomic>
class DefaultWidgetValueTraits<TAtomic, QDoubleSliderWithEditor>
    : public WidgetValueTraitsBase<TAtomic, QDoubleSliderWithEditor *>
{
public:
  const char *GetSignal()
  {
    return SIGNAL(valueChanged(double));
  }

  TAtomic GetValue(QDoubleSliderWithEditor *w)
  {
    return static_cast<TAtomic>(w->value());
  }

  void SetValue(QDoubleSliderWithEditor *w, const TAtomic &value)
  {
    w->setValue(static_cast<double>(value));
  }


  void SetValueToNull(QDoubleSliderWithEditor *w)
  {
    w->setValueToNull();
  }
};

template <class TAtomic>
class DefaultWidgetDomainTraits<NumericValueRange<TAtomic>, QDoubleSliderWithEditor>
    : public WidgetDomainTraitsBase<NumericValueRange<TAtomic>, QDoubleSliderWithEditor *>
{
public:
  void SetDomain(QDoubleSliderWithEditor *w, const NumericValueRange<TAtomic> &range)
  {
    w->setMinimum(range.Minimum);
    w->setMaximum(range.Maximum);
    w->setSingleStep(range.StepSize);
  }

  NumericValueRange<TAtomic> GetDomain(QDoubleSliderWithEditor *w)
  {
    return NumericValueRange<TAtomic>(
          static_cast<TAtomic>(w->minimum()),
          static_cast<TAtomic>(w->maximum()),
          static_cast<TAtomic>(w->singleStep()));
  }
};

#endif // QTDOUBLESLIDERWITHEDITORCOUPLING_H
