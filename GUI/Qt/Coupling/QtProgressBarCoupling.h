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
#ifndef QTPROGRESSBARCOUPLING_H
#define QTPROGRESSBARCOUPLING_H

#include <QtWidgetCoupling.h>
#include <QProgressBar>

template <class TAtomic>
class DefaultWidgetValueTraits<TAtomic, QProgressBar>
    : public WidgetValueTraitsBase<TAtomic, QProgressBar *>
{
public:

  TAtomic GetValue(QProgressBar *w)
  {
    return w->value() * 0.01;
  }

  void SetValue(QProgressBar *w, const TAtomic &value)
  {
    w->setValue(static_cast<int>(value * 100.0));
  }

  virtual void SetValueToNull(QProgressBar *w)
  {
    w->setValue(w->minimum());
  }
};



template <class TAtomic>
class DefaultWidgetDomainTraits<NumericValueRange<TAtomic>, QProgressBar>
    : public WidgetDomainTraitsBase<NumericValueRange<TAtomic>, QProgressBar *>
{
public:
  void SetDomain(QProgressBar *w, const NumericValueRange<TAtomic> &range)
  {
    w->setMinimum((int)(range.Minimum * 100.0));
    w->setMaximum((int)(range.Maximum * 100.0));
  }

  NumericValueRange<TAtomic> GetDomain(QProgressBar *w)
  {
    return NumericValueRange<TAtomic>(
          static_cast<TAtomic>(w->minimum() * 0.01),
          static_cast<TAtomic>(w->maximum() * 0.01),
          static_cast<TAtomic>(0.01));
  }
};

#endif // QTPROGRESSBARCOUPLING_H
