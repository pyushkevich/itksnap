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

#ifndef EDITABLENUMERICVALUEMODEL_H
#define EDITABLENUMERICVALUEMODEL_H

#include <SNAPCommon.h>
#include <SNAPEvents.h>
#include "AbstractModel.h"

template<class TVal> struct NumericValueRange
{
  TVal Minimum, Maximum, StepSize;

  NumericValueRange(TVal min, TVal max, TVal step) :
    Minimum(min), Maximum(max), StepSize(step) {}

  NumericValueRange()
    : NumericValueRange(0, 100, 1) {}
};

template<class TVal>
class AbstractEditableNumericValueModel : public AbstractModel
{
public:

  virtual TVal GetValue() const = 0;
  virtual void SetValue(TVal value) = 0;
  virtual NumericValueRange<TVal> GetRange() const = 0;

  class ValueChangedEvent : public IRISEvent {};
  class RangeChangedEvent : public IRISEvent {};
};

template<class TVal>
class EditableNumericValueModel
    : public AbstractEditableNumericValueModel<TVal>
{
public:

  irisITKObjectMacro(EditableNumericValueModel<TVal>,
                     AbstractEditableNumericValueModel<TVal>)

  irisGetMacro(Value, TVal)
  irisSetWithEventMacro(Value, TVal, Superclass::ValueChangedEvent)

  irisGetMacro(Range, NumericValueRange<TVal>)
  irisSetWithEventMacro(Range, NumericValueRange<TVal>, Superclass::RangeChangedEvent)

protected:

  EditableNumericValueModel();
  virtual ~EditableNumericValueModel();

  TVal m_Value;
  NumericValueRange<TVal> m_Range;
};

#endif // EDITABLENUMERICVALUEMODEL_H
