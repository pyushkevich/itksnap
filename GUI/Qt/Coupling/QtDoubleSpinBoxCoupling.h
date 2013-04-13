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

#ifndef QTDOUBLESPINBOXCOUPLING_H
#define QTDOUBLESPINBOXCOUPLING_H

#include <QtWidgetCoupling.h>
#include <QDoubleSpinBox>

/**
  Default traits for the Qt Double Spin Box
  */
template <class TAtomic>
class DefaultWidgetValueTraits<TAtomic, QDoubleSpinBox>
    : public WidgetValueTraitsBase<TAtomic, QDoubleSpinBox *>
{
public:
  const char *GetSignal()
  {
    return SIGNAL(valueChanged(double));
  }

  TAtomic GetValue(QDoubleSpinBox *w)
  {
    return static_cast<TAtomic>(w->value());
  }

  void SetValue(QDoubleSpinBox *w, const TAtomic &value)
  {
    w->setSpecialValueText("");
    w->setValue(static_cast<double>(value));
  }


  void SetValueToNull(QDoubleSpinBox *w)
  {
    w->setValue(w->minimum());
    w->setSpecialValueText(" ");
  }
};

template <class TAtomic>
class DefaultWidgetDomainTraits<NumericValueRange<TAtomic>, QDoubleSpinBox>
    : public WidgetDomainTraitsBase<NumericValueRange<TAtomic>, QDoubleSpinBox *>
{
public:
  void SetDomain(QDoubleSpinBox *w, const NumericValueRange<TAtomic> &range)
  {
    w->setMinimum(range.Minimum);
    w->setMaximum(range.Maximum);
    w->setSingleStep(range.StepSize);

    // Make sure the precision is smaller than the step size. This is a
    // temporary fix. A better solution is to have the model provide the
    // precision for the widget.
    if(range.StepSize > 0)
      {
      double logstep = std::log10((double)range.StepSize);
      int prec = std::max((int) (1 - floor(logstep)), 0);
      w->setDecimals(prec);
      }
    else
      w->setDecimals(0);
  }

  NumericValueRange<TAtomic> GetDomain(QDoubleSpinBox *w)
  {
    return NumericValueRange<TAtomic>(
          static_cast<TAtomic>(w->minimum()),
          static_cast<TAtomic>(w->maximum()),
          static_cast<TAtomic>(w->singleStep()));
  }
};

#endif // QTDOUBLESPINBOXCOUPLING_H
