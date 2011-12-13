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

  NumericValueRange(TVal min, TVal max) :
    Minimum(min), Maximum(max), StepSize(0.0) {}

  NumericValueRange()
    : NumericValueRange(0, 100, 1) {}
};

template<class TVal>
class AbstractEditableNumericValueModel : public AbstractModel
{
public:

  virtual TVal GetValue() = 0;
  virtual void SetValue(TVal value) = 0;
  virtual NumericValueRange<TVal> GetRange() = 0;
  virtual bool IsValueNull() = 0;

};

template<class TVal>
class EditableNumericValueModel
    : public AbstractEditableNumericValueModel<TVal>
{
public:

  irisITKObjectMacro(EditableNumericValueModel<TVal>,
                     AbstractEditableNumericValueModel<TVal>)

  TVal GetValue() { return m_Value; }
  irisSetWithEventMacro(Value, TVal, Superclass::ValueChangedEvent)

  NumericValueRange<TVal> GetRange() { return m_Range; }
  irisSetWithEventMacro(Range, NumericValueRange<TVal>, Superclass::RangeChangedEvent)

  bool IsValueNull() { return false; }

protected:

  EditableNumericValueModel();
  virtual ~EditableNumericValueModel();

  TVal m_Value;
  NumericValueRange<TVal> m_Range;
};

/**
  This is a model that simply serves as a wrapper around an object and
  a set of four functions in the object. This makes it easier to construct
  these editable models without having to write a new class. The parent
  model must be an instance of AbstractModel and have a Getter for the
  value. Optionally, it should also have a setter for the value coupled
  with a getter for the numeric range. An optional IsValueNull function
  can also be supplied
  */
template<class TVal, class TModel>
class FunctionWrapperNumericValueModel
    : public AbstractEditableNumericValueModel<TVal>
{
public:

  // Standard ITK stuff (can't use irisITKObjectMacro because of two template
  // parameters, comma breaks the macro).
  typedef FunctionWrapperNumericValueModel<TVal, TModel> Self; \
  typedef AbstractEditableNumericValueModel<TVal> Superclass; \
  typedef SmartPtr<Self> Pointer; \
  typedef SmartPtr<const Self> ConstPointer; \
  itkTypeMacro(FunctionWrapperNumericValueModel,
               AbstractEditableNumericValueModel) \
  itkNewMacro(Self)

  // Function pointers available
  typedef bool (TModel::*IsValueValidFunctionPointer)();
  typedef TVal (TModel::*GetValueFunctionPointer)();
  typedef void (TModel::*SetValueFunctionPointer)(TVal t);
  typedef NumericValueRange<TVal> (TModel::*GetRangeFunctionPointer)();

  // The setup function
  void Initialize(TModel *model,
                  GetValueFunctionPointer getValue,
                  SetValueFunctionPointer setValue = NULL,
                  GetRangeFunctionPointer getRange = NULL,
                  IsValueValidFunctionPointer isValid = NULL)
  {
    m_Model = model;
    m_GetValueFunctionPointer = getValue;
    m_SetValueFunctionPointer = setValue;
    m_GetRangeFunctionPointer = getRange;
    m_IsValueValidFunction = isValid;
  }

  /**
    Set up the events fired by the parent model that this model should
    listen to. The model will rebroadcast these events
    */
  void SetEvents(const itk::EventObject &valueEvent,
                 const itk::EventObject &rangeEvent)
  {
    Rebroadcast(m_Model, valueEvent, ValueChangedEvent());
    Rebroadcast(m_Model, rangeEvent, RangeChangedEvent());
  }

  TVal GetValue()
  {
    static_cast<AbstractModel *>(m_Model)->Update();
    return ((*m_Model).*(m_GetValueFunctionPointer))();
  }

  void SetValue(TVal value)
  {
    if(m_SetValueFunctionPointer)
      {
      static_cast<AbstractModel *>(m_Model)->Update();
      ((*m_Model).*(m_SetValueFunctionPointer))(value);
      }
  }

  NumericValueRange<TVal> GetRange()
  {
    if(m_GetRangeFunctionPointer)
      {
      static_cast<AbstractModel *>(m_Model)->Update();
      return ((*m_Model).*(m_GetRangeFunctionPointer))();
      }
    else
      {
      return NumericValueRange<TVal>(GetValue(), GetValue());
      }
  }

  bool IsValueNull()
  {
    static_cast<AbstractModel *>(m_Model)->Update();
    return !((*m_Model).*(m_IsValueValidFunction))();
  }

protected:

  TModel *m_Model;
  IsValueValidFunctionPointer m_IsValueValidFunction;
  GetValueFunctionPointer m_GetValueFunctionPointer;
  SetValueFunctionPointer m_SetValueFunctionPointer;
  GetRangeFunctionPointer m_GetRangeFunctionPointer;

  FunctionWrapperNumericValueModel() {}
  ~FunctionWrapperNumericValueModel() {}

};

/**
  This model simply accesses a component in a model templated over a fixed
  length vector type. It's a convenience when we are dealing with vectors of
  numeric values. The component model has a smart pointer to the parent model,
  so it is safe to not hold on the the parent model after creating the
  component model. Here is a use case:

  typedef SomeEditableNumericValueModel<Vector3d> VectorModel;
  SmartPtr<VectorModel> p = VectorModel::New();
  ...
  typedef AbstractEditableNumericValueModel<double> ComponentModel;
  SmartPtr<ComponentModel> c0 = ComponentModel::New(p, 0);

  */
template <class TVal, unsigned int VDim>
class ComponentEditableNumericValueModel
    : public AbstractEditableNumericValueModel<TVal>
{
public:
  // Standard ITK stuff (can't use irisITKObjectMacro because of two template
  // parameters, comma breaks the macro).
  typedef ComponentEditableNumericValueModel<TVal, VDim> Self;
  typedef AbstractEditableNumericValueModel<TVal> Superclass;
  typedef SmartPtr<Self> Pointer;
  typedef SmartPtr<const Self> ConstPointer;
  itkTypeMacro(ComponentEditableNumericValueModel,
               AbstractEditableNumericValueModel)

  // The model we must wrap around
  typedef iris_vector_fixed<TVal, VDim> VectorType;
  typedef AbstractEditableNumericValueModel<VectorType> ParentModel;

  // For shorthand, we create a factory method with parameters
  static Pointer New(ParentModel *parent, unsigned int component)
  {
    Pointer smartPtr = new Self(parent, component);
    smartPtr->m_Parent = parent;
    smartPtr->m_Component = component;
    smartPtr->UnRegister();
    return smartPtr;
  }

  TVal GetValue()
  {
    return m_Parent->GetValue()[m_Component];
  }

  void SetValue(TVal value)
  {
    VectorType v = m_Parent->GetValue();
    v[m_Component] = value;
    m_Parent->SetValue(v);
  }

  NumericValueRange<TVal> GetRange()
  {
    NumericValueRange<VectorType> nr = m_Parent->GetRange();
    return NumericValueRange<TVal>(nr.Minimum[m_Component],
                                   nr.Maximum[m_Component],
                                   nr.StepSize[m_Component]);
  }

  bool IsValueNull()
  {
    return m_Parent->IsValueNull();
  }

protected:

  ComponentEditableNumericValueModel(ParentModel *parent, unsigned int component)
  {
    m_Parent = parent;
    m_Component = component;
    Rebroadcast(m_Parent,
                ValueChangedEvent(),
                ValueChangedEvent());
    Rebroadcast(m_Parent,
                RangeChangedEvent(),
                RangeChangedEvent());
  }

  ~ComponentEditableNumericValueModel() {}

  unsigned int m_Component;
  SmartPtr<ParentModel> m_Parent;

};

#endif // EDITABLENUMERICVALUEMODEL_H
