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

/**
  This class represents the range information associated with a numeric
  value. This range information is used to set up the GUI controls with
  which the user interacts.
  */
template<class TVal> struct NumericValueRange
{
  TVal Minimum, Maximum, StepSize;

  NumericValueRange(TVal min, TVal max, TVal step) :
    Minimum(min), Maximum(max), StepSize(step) {}

  NumericValueRange(TVal min, TVal max) :
    Minimum(min), Maximum(max), StepSize(0.0) {}

  NumericValueRange() :
    Minimum(0.0), Maximum(0.0), StepSize(0.0) {}
};

/**
  A parent class for the hierarchy of models that provide access to
  a numeric value (or array of numeric values). This important class in
  SNAP provides one side of the automatic coupling between data and GUI
  widgets. When we want the user to interact with a numeric value, we
  expose the value using this model object. We then use the automatic
  coupling mechanism (in Qt, see QtWidgetCoupling.h) to attach this model
  to the corresponding widget. As the result, the programmer does not have
  to write any callbacks or glue between the widget and the data. The widget
  just updates in response to changes in the data, and its range is set
  correctly as well.

  Exposing data (doubles, ints, etc.) as numeric models requires a few lines
  of code on the model side of the program. However, the method
  makeChildNumericValueModel tries to make this code as short as possible.
  */
template<class TVal>
class AbstractEditableNumericValueModel : public AbstractModel
{
public:
  typedef NumericValueRange<TVal> RangeType;

  /** A compound getter method exposed by the model. Return false if the
    value is not valid, and the corresponding control should show a blank
    string instead of a value. If the range is not needed, a NULL pointer
    will be passed in. If range is needed, the current values stored in
    the GUI widget will be passed in.

    If the range is not handled by the model (i.e., a fixed range set in
    the GUI designer once and for all), the callback can just ignore the
    range parameter.
  */
  virtual bool GetValueAndRange(TVal &value, RangeType *range) = 0;

  /** A setter method */
  virtual void SetValue(TVal value) = 0;

  // virtual TVal GetValue() = 0;
  // virtual NumericValueRange<TVal> GetRange() = 0;
  // virtual bool IsValueNull() = 0;
};

/**
  A concrete implementation of AbstractEditableNumericValueModel that holds
  a numeric value and a range as a private variables.
  */
template<class TVal>
class EditableNumericValueModel
    : public AbstractEditableNumericValueModel<TVal>
{
public:

  irisITKObjectMacro(EditableNumericValueModel<TVal>,
                     AbstractEditableNumericValueModel<TVal>)

  typedef typename Superclass::RangeType RangeType;

  virtual bool GetValueAndRange(TVal &value, RangeType *range)
  {
    value = m_Value;
    if(range)
      *range = m_Range;
    return true;
  }

  irisSetWithEventMacro(Value, TVal, Superclass::ValueChangedEvent)
  irisSetWithEventMacro(Range, NumericValueRange<TVal>, Superclass::RangeChangedEvent)

protected:

  EditableNumericValueModel();
  virtual ~EditableNumericValueModel();

  TVal m_Value;
  NumericValueRange<TVal> m_Range;
};

/**
  This is a model that simply serves as a wrapper around an object and
  a pair of functions in the object. This makes it easier to construct
  these editable models without having to write a new class. The parent
  model must be an instance of AbstractModel and have a getter for the
  value that matches the signature of GetValueAndRange.

  TVal GetValueAndRange(bool &isValid, NumericValueRange<TVal> *range)
  {
    // range is NULL if not requested
    if(range) { ... } // fill range values

    // isValid automatically set to true, so only change if needed
    if(not valid) isValid = false;

    return value;
  }

  An optional setter can also be supplied.
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
  typedef typename Superclass::RangeType RangeType;
  itkTypeMacro(FunctionWrapperNumericValueModel,
               AbstractEditableNumericValueModel) \
  itkNewMacro(Self)

  // Function pointers to a setter method
  typedef void (TModel::*SetValueFunctionPointer)(TVal t);

  // Compound getter function pointer
  typedef bool (TModel::*GetValueAndRangeFunctionPointer)(
      TVal &value, NumericValueRange<TVal> *range);

  // The setup function
  void Initialize(TModel *model,
                  GetValueAndRangeFunctionPointer getValueAndRange,
                  SetValueFunctionPointer setValue = NULL)
  {
    m_Model = model;
    m_GetValueAndRangeFunctionPointer = getValueAndRange;
    m_SetValueFunctionPointer = setValue;
  }

  /**
    Set up the events fired by the parent model that this model should
    listen to and rebroadcast as ValueChangedEvent and RangeChangedEvent.
    */
  void SetEvents(const itk::EventObject &valueEvent,
                 const itk::EventObject &rangeEvent)
  {
    Rebroadcast(m_Model, valueEvent, ValueChangedEvent());
    Rebroadcast(m_Model, rangeEvent, RangeChangedEvent());
  }

  bool GetValueAndRange(TVal &value, RangeType *range)
  {
    return ((*m_Model).*(m_GetValueAndRangeFunctionPointer))(value, range);
  }

  void SetValue(TVal value)
  {
    if(m_SetValueFunctionPointer)
      {
      static_cast<AbstractModel *>(m_Model)->Update();
      ((*m_Model).*(m_SetValueFunctionPointer))(value);
      }
  }

protected:

  TModel *m_Model;
  SetValueFunctionPointer m_SetValueFunctionPointer;
  GetValueAndRangeFunctionPointer m_GetValueAndRangeFunctionPointer;

  FunctionWrapperNumericValueModel()
  {
    m_SetValueFunctionPointer = NULL;
    m_GetValueAndRangeFunctionPointer = NULL;
  }

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
  typedef typename Superclass::RangeType RangeType;
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

  bool GetValueAndRange(TVal &value, RangeType *range)
  {
    VectorType value_vec;
    bool rc;

    if(range)
      {
      NumericValueRange<VectorType> range_vec;
      range_vec.Minimum[m_Component] = range->Minimum;
      range_vec.Maximum[m_Component] = range->Maximum;
      range_vec.StepSize[m_Component] = range->StepSize;
      if(rc = m_Parent->GetValueAndRange(value_vec, &range_vec))
        {
        range->Minimum = range_vec.Minimum[m_Component];
        range->Maximum = range_vec.Maximum[m_Component];
        range->StepSize = range_vec.StepSize[m_Component];
        }
      }
    else
      {
      rc = m_Parent->GetValueAndRange(value_vec, NULL);
      }

    if(rc)
      value = value_vec[m_Component];

    return rc;
  }

  void SetValue(TVal value)
  {
    VectorType v;
    if(m_Parent->GetValueAndRange(v, NULL))
      {
      v[m_Component] = value;
      m_Parent->SetValue(v);
      }
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

/**
  A shorthand method for creating models that wrap around functions in a
  parent model. This is intended to reduce the amount of setup code in
  the parent models.
  */
template<class TModel, class TVal>
SmartPtr<AbstractEditableNumericValueModel<TVal> >
makeChildNumericValueModel(
    TModel *model,
    bool (TModel::*getValueAndRange)(TVal &, NumericValueRange<TVal> *),
    void (TModel::*setValue)(TVal),
    const itk::EventObject &valueEvent = ModelUpdateEvent(),
    const itk::EventObject &rangeEvent = ModelUpdateEvent())
{
  typedef FunctionWrapperNumericValueModel<TVal, TModel> WrapperType;
  SmartPtr<WrapperType> p = WrapperType::New();
  p->Initialize(model, getValueAndRange, setValue);
  p->SetEvents(valueEvent, rangeEvent);
  // p->UnRegister();

  SmartPtr<AbstractEditableNumericValueModel<TVal> > pout(p);
  return pout;
}

#endif // EDITABLENUMERICVALUEMODEL_H
