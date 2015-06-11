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

#ifndef QTSLIDERCOUPLING_H
#define QTSLIDERCOUPLING_H

#include <QtWidgetCoupling.h>
#include <QSlider>
#include <QDoubleSlider.h>

template <class TAtomic>
class DefaultWidgetValueTraits<TAtomic, QSlider>
    : public WidgetValueTraitsBase<TAtomic, QSlider *>
{
public:
  const char *GetSignal()
  {
    return SIGNAL(valueChanged(int));
  }

  TAtomic GetValue(QSlider *w)
  {
    return static_cast<TAtomic>(w->value());
  }

  void SetValue(QSlider *w, const TAtomic &value)
  {
    w->setValue(static_cast<int>(value));
  }

  virtual void SetValueToNull(QSlider *w)
  {
    w->setValue(w->minimum());
  }
};

template <class TAtomic>
class DefaultWidgetDomainTraits<NumericValueRange<TAtomic>, QSlider>
    : public WidgetDomainTraitsBase<NumericValueRange<TAtomic>, QSlider *>
{
public:
  void SetDomain(QSlider *w, const NumericValueRange<TAtomic> &range)
  {
    w->setMinimum(range.Minimum);
    w->setMaximum(range.Maximum);
    w->setSingleStep(range.StepSize);
  }

  NumericValueRange<TAtomic> GetDomain(QSlider *w)
  {
    return NumericValueRange<TAtomic>(
          static_cast<TAtomic>(w->minimum()),
          static_cast<TAtomic>(w->maximum()),
          static_cast<TAtomic>(w->singleStep()));
  }
};


template <class TAtomic>
class DefaultWidgetValueTraits<TAtomic, QDoubleSlider>
    : public WidgetValueTraitsBase<TAtomic, QDoubleSlider *>
{
public:
  const char *GetSignal()
  {
    return SIGNAL(valueChanged(int));
  }

  TAtomic GetValue(QDoubleSlider *w)
  {
    return static_cast<TAtomic>(w->doubleValue());
  }

  void SetValue(QDoubleSlider *w, const TAtomic &value)
  {
    w->setDoubleValue(static_cast<double>(value));
  }
};

template <class TAtomic>
class DefaultWidgetDomainTraits<NumericValueRange<TAtomic>, QDoubleSlider>
    : public WidgetDomainTraitsBase<NumericValueRange<TAtomic>, QDoubleSlider *>
{
public:
  void SetDomain(QDoubleSlider *w, const NumericValueRange<TAtomic> &range)
  {
    w->setDoubleMinimum(range.Minimum);
    w->setDoubleMaximum(range.Maximum);
    w->setDoubleSingleStep(range.StepSize);
  }

  NumericValueRange<TAtomic> GetDomain(QDoubleSlider *w)
  {
    return NumericValueRange<TAtomic>(
          static_cast<TAtomic>(w->doubleMinimum()),
          static_cast<TAtomic>(w->doubleMaximum()),
          static_cast<TAtomic>(w->doubleSingleStep()));
  }
};


#endif // QTSLIDERCOUPLING_H
