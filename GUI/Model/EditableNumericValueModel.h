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
    Minimum(min), Maximum(max)
  {
    StepSize = (TVal) 0;
  }

  NumericValueRange()
  {
    Minimum = static_cast<TVal>(0);
    Maximum = (TVal) 0;
    StepSize = (TVal) 0;
  }

  void Set(TVal min, TVal max, TVal step)
    { Minimum = min; Maximum = max; StepSize = step; }
};

/**
  This class represents a domain that allows all values in a data type. It
  can be used with the class AbstractPropertyModel when there is no need to
  communicate domain information to the widget
  */
class TrivialDomain
{

};

/**
  A parent class for a family of models that encapsulate a single value of
  a particular type. These models use events to communicate changes in state
  and can be coupled to GUI widgets to allow seamless connections between the
  GUI and the model layer. The model is parameterized by the data type of the
  value (TVal), which would normally be a number, a string, a vector, etc. It
  is also parameterized by the domain type, which describes the set of values
  from which the value is drawn. The domain is used to configure GUI widgets
  to that the user is restricted to choosing a value in a valid range. For
  example, for TVal=double, the natural domain is the NumericValueRange class,
  consisting of a min, max and a step size. For TVal=string, the natural
  domain is a set of strings.

  In addition to supplying a value for the encapsulated property, the model
  can return a boolean flag as to whether the model/property is in a valid
  state. For example, a model describing the minumum intensity of an image
  would be in an invalid state if there is no image currently loaded. The
  corresponding GUI widget can then be set to indicate that the value is
  invalid or null.

  This type of model is meant to be matched to a widget in the GUI. Since the
  number of widgets is small (10s or 100s), it is acceptable for these models
  to be somewhat heavyweight. They inherit from AbstractModel, which in turn
  inherits from itk::Object.
  */
template <class TVal, class TDomain = TrivialDomain>
class AbstractPropertyModel : public AbstractModel
{
public:

  /** The atomic type encompassed by the model */
  typedef TVal ValueType;

  /** The type of the domain */
  typedef TDomain DomainType;

  /** A setter method */
  virtual void SetValue(TVal value) = 0;

  /**
    A compound getter method exposed by the model. Return false if the
    value is not valid, and the corresponding control should show a blank
    string instead of a value. If the domain is not needed, a NULL pointer
    will be passed in. If domain is needed, the current values stored in
    the GUI widget will be passed in.

    If the domain is not handled by the model (i.e., a fixed range set in
    the GUI designer once and for all), the callback can just ignore the
    domain parameter.
  */
  virtual bool GetValueAndDomain(TVal &value, TDomain *domain) = 0;

};


/**
  A concrete implementation of AbstractPropertyModel that holds the value,
  the validity flag, and the domain as private variables. The validity flag
  is initialized to true. The parent model is responsible for setting the
  value, domain, and validity flag inside of this concrete model.
  */
template <class TVal, class TDomain>
class ConcretePropertyModel : public AbstractPropertyModel<TVal, TDomain>
{
public:
  // Standard ITK stuff
  typedef ConcretePropertyModel<TVal, TDomain> Self;
  typedef AbstractPropertyModel<TVal, TDomain> Superclass;
  typedef SmartPtr<Self> Pointer;
  typedef SmartPtr<const Self> ConstPointer;
  itkTypeMacro(ConcretePropertyModel, AbstractPropertyModel)
  itkNewMacro(Self)

  virtual bool GetValueAndDomain(TVal &value, TDomain *domain)
  {
    value = m_Value;
    if(domain)
      *domain = m_Domain;
    return m_IsValid;
  }

  irisSetWithEventMacro(Value, TVal, Superclass::ValueChangedEvent)
  irisSetWithEventMacro(Range, TDomain, Superclass::RangeChangedEvent)
  irisSetWithEventMacro(IsValid, bool, Superclass::ValueChangedevent)

protected:

  ConcretePropertyModel()
  {
    m_IsValid = true;
  }

  virtual ~ConcretePropertyModel();

  TVal m_Value;
  TDomain m_Domain;
  bool m_IsValid;
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
  This class is only used to define some typedefs
  */
template <class TVal>
class AbstractRangedPropertyModel
{
public:
  typedef NumericValueRange<TVal> DomainType;
  typedef AbstractPropertyModel<TVal, DomainType> Type;
};

typedef AbstractRangedPropertyModel<double>::Type AbstractDoubleRangedPropertyModel;
typedef AbstractRangedPropertyModel<int>::Type AbstractIntRangedPropertyModel;



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
  An implementation of the AbstractPropertyModel that serves as a wrapper
  around a getter function and a setter function, both members of a parent
  model class. The primary use for this class is to make it easy to create
  AbstractPropertyModels that describe a derived property.

  The parent model must include a getter with one of the following three
  signatures.

      bool GetValueAndDomain(TVal &value, TDomain *domain)

      bool GetValue(TVal &value)

      TVal GetValue()

  It may also optionally include a setter method with the signature

      void SetValue(TVal value)

  In addition to value and domain, this class is templated over the parent
  model type (TModel) and the traits object describing the signature of the
  getter. This is because we want to support all three of the getter signatures
  without having to check which one the user supplied dynamically.

  Note that this class is not meant to be used directly in the GUI model code.
  Instead, one should make use of the function makeChildPropertyModel, which
  serves as a factory for creating models of this type.
*/
template<class TVal, class TDomain, class TModel, class GetterTraits>
class FunctionWrapperPropertyModel
    : public AbstractPropertyModel<TVal, TDomain>
{
public:

  // Standard ITK stuff (can't use irisITKObjectMacro because of two template
  // parameters, comma breaks the macro).
  typedef FunctionWrapperPropertyModel<TVal, TDomain, TModel, GetterTraits> Self;
  typedef AbstractPropertyModel<TVal, TDomain> Superclass;
  typedef SmartPtr<Self> Pointer;
  typedef SmartPtr<const Self> ConstPointer;

  itkTypeMacro(FunctionWrapperPropertyModel, AbstractPropertyModel)
  itkNewMacro(Self)

  // Function pointers to a setter method
  typedef void (TModel::*Setter)(TVal t);

  // The function pointer to the getter is provided by the traits
  typedef typename GetterTraits::Getter Getter;

  /** Initializes a model with a parent model and function pointers */
  void Initialize(TModel *model, Getter getter, Setter setter = NULL)
  {
    m_Model = model;
    m_Getter = getter;
    m_Setter = setter;
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


  bool GetValueAndDomain(TVal &value, TDomain *domain)
  {
    return GetterTraits::GetValueAndDomain(m_Model, m_Getter, value, domain);
  }

  void SetValue(TVal value)
  {
    if(m_Setter)
      {
      static_cast<AbstractModel *>(m_Model)->Update();
      ((*m_Model).*(m_Setter))(value);
      }
  }

  /**
    A factory method used to create new models of this type. The user should
    not need to call this directly, instead use the makeChildPropertyModel
    methods below. The last two paremeters are the events fired by the parent
    model that should be rebroadcast as the value and domain change events by
    the property model.
    */
  static SmartPtr<Superclass> CreatePropertyModel(
      TModel *parentModel,
      Getter getter, Setter setter,
      const itk::EventObject &valueEvent,
      const itk::EventObject &rangeEvent)
  {
    SmartPtr<Self> p = Self::New();
    p->Initialize(parentModel, getter, setter);
    p->SetEvents(valueEvent, rangeEvent);

    // p->UnRegister();

    SmartPtr<Superclass> pout(p);
    return pout;
  }

protected:

  TModel *m_Model;
  Getter m_Getter;
  Setter m_Setter;

  FunctionWrapperPropertyModel()
    : m_Model(NULL), m_Getter(NULL), m_Setter(NULL)  {}

  ~FunctionWrapperPropertyModel() {}

};

/**
  Getter traits for the FunctionWrapperPropertyModel that use the compound
  getter signature,

    bool GetValueAndDomain(TVal &value, TDomain &domain);
*/
template <class TVal, class TDomain, class TModel>
class FunctionWrapperPropertyModelCompoundGetterTraits
{
public:
  // Signature of the getter
  typedef bool (TModel::*Getter)(TVal &t, TDomain *domain);

  // Implementation of the get method, just calls the getter directly
  static bool GetValueAndDomain(
      TModel *model, Getter getter, TVal &value, TDomain *domain)
  {
    return ((*model).*(getter))(value, domain);
  }
};

/**
  Getter traits for the FunctionWrapperPropertyModel that use the rangeless
  getter signature,

    bool GetValue(TVal &value);
*/
template <class TVal, class TDomain, class TModel>
class FunctionWrapperPropertyModelRangelessGetterTraits
{
public:
  // Signature of the getter
  typedef bool (TModel::*Getter)(TVal &t);

  // Implementation of the get method, just calls the getter directly
  static bool GetValueAndDomain(
      TModel *model, Getter getter, TVal &value, TDomain *domain)
  {
    return ((*model).*(getter))(value);
  }
};

/**
  Getter traits for the FunctionWrapperPropertyModel that use the simple
  getter signature,

    TVal GetValue();
*/
template <class TVal, class TDomain, class TModel>
class FunctionWrapperPropertyModelSimpleGetterTraits
{
public:
  // Signature of the getter
  typedef TVal (TModel::*Getter)();

  // Implementation of the get method, just calls the getter directly
  static bool GetValueAndDomain(
      TModel *model, Getter getter, TVal &value, TDomain *domain)
  {
    value = ((*model).*(getter))();
    return true;
  }
};



/**
  This code creates an AbstractPropertyModel that wraps around a pair of
  functions (a getter and a setter) in the parent model object. There are
  three versions of this function, corresponding to different signatures
  of the getter function.
  */
template<class TModel, class TVal, class TDomain>
SmartPtr< AbstractPropertyModel<TVal, TDomain> >
makeChildPropertyModel(
    TModel *model,
    bool (TModel::*getter)(TVal &, TDomain *),
    void (TModel::*setter)(TVal) = NULL,
    const itk::EventObject &valueEvent = ModelUpdateEvent(),
    const itk::EventObject &rangeEvent = ModelUpdateEvent())
{
  typedef FunctionWrapperPropertyModelCompoundGetterTraits<
      TVal, TDomain, TModel> TraitsType;

  typedef FunctionWrapperPropertyModel<
      TVal, TDomain, TModel, TraitsType> ModelType;

  return ModelType::CreatePropertyModel(model, getter, setter,
                                        valueEvent, rangeEvent);
}


template<class TModel, class TVal>
SmartPtr< AbstractPropertyModel<TVal> >
makeChildPropertyModel(
    TModel *model,
    bool (TModel::*getter)(TVal &),
    void (TModel::*setter)(TVal) = NULL,
    const itk::EventObject &valueEvent = ModelUpdateEvent(),
    const itk::EventObject &rangeEvent = ModelUpdateEvent())
{
  typedef FunctionWrapperPropertyModelRangelessGetterTraits<
      TVal, TrivialDomain, TModel> TraitsType;

  typedef FunctionWrapperPropertyModel<
      TVal, TrivialDomain, TModel, TraitsType> ModelType;

  return ModelType::CreatePropertyModel(model, getter, setter,
                                        valueEvent, rangeEvent);
}

template<class TModel, class TVal>
SmartPtr< AbstractPropertyModel<TVal> >
makeChildPropertyModel(
    TModel *model,
    TVal (TModel::*getter)(),
    void (TModel::*setter)(TVal) = NULL,
    const itk::EventObject &valueEvent = ModelUpdateEvent(),
    const itk::EventObject &rangeEvent = ModelUpdateEvent())
{
  typedef FunctionWrapperPropertyModelSimpleGetterTraits<
      TVal, TrivialDomain, TModel> TraitsType;

  typedef FunctionWrapperPropertyModel<
      TVal, TrivialDomain, TModel, TraitsType> ModelType;

  return ModelType::CreatePropertyModel(model, getter, setter,
                                        valueEvent, rangeEvent);
}



/**
  This is a model that simply serves as a wrapper around an object and
  a pair of functions in the object. This makes it easier to construct
  these editable models without having to write a new class. The parent
  model must be an instance of AbstractModel and have a getter for the
  value that matches the signature of GetValueAndRange or that matches
  the signature of GetValue

  bool GetValueAndRange(TVal &value, NumericValueRange<TVal> *range)
  {
    // range is NULL if not requested
    if(range) { ... } // fill range values

    // isValid automatically set to true, so only change if needed
    if(not valid) return false;

    value = ...
    return true;
  }

  TVal GetValue()
  {
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

  // Simple getter function pointer
  typedef TVal (TModel::*GetValueFunctionPointer)();

  // The setup function using the range getter
  void Initialize(TModel *model,
                  GetValueAndRangeFunctionPointer getValueAndRange,
                  SetValueFunctionPointer setValue = NULL)
  {
    m_Model = model;
    m_GetValueAndRangeFunctionPointer = getValueAndRange;
    m_SetValueFunctionPointer = setValue;
    m_GetValueFunctionPointer = NULL;
  }

  // The setup function using the simple getter
  void Initialize(TModel *model,
                  GetValueFunctionPointer getValue,
                  SetValueFunctionPointer setValue = NULL)
  {
    m_Model = model;
    m_GetValueFunctionPointer = getValue;
    m_SetValueFunctionPointer = setValue;
    m_GetValueAndRangeFunctionPointer = NULL;
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
    if(m_GetValueAndRangeFunctionPointer)
      {
      return ((*m_Model).*(m_GetValueAndRangeFunctionPointer))(value, range);
      }
    else
      {
      value = ((*m_Model).*(m_GetValueFunctionPointer))();
      return true;
      }
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
  GetValueFunctionPointer m_GetValueFunctionPointer;

  FunctionWrapperNumericValueModel()
  {
    m_SetValueFunctionPointer = NULL;
    m_GetValueAndRangeFunctionPointer = NULL;
    m_GetValueFunctionPointer = NULL;
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
    void (TModel::*setValue)(TVal) = NULL,
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

/**
  A shorthand method for creating models that wrap around functions in a
  parent model. This is intended to reduce the amount of setup code in
  the parent models. This version uses the simple getter method, i.e., it
  is assumed that the value is always valid and that the domain is managed
  by the GUI, not the model.
  */
template<class TModel, class TVal>
SmartPtr<AbstractEditableNumericValueModel<TVal> >
makeChildNumericValueModel(
    TModel *model,
    TVal (TModel::*getValue)(),
    void (TModel::*setValue)(TVal) = NULL,
    const itk::EventObject &valueEvent = ModelUpdateEvent(),
    const itk::EventObject &rangeEvent = ModelUpdateEvent())
{
  typedef FunctionWrapperNumericValueModel<TVal, TModel> WrapperType;
  SmartPtr<WrapperType> p = WrapperType::New();
  p->Initialize(model, getValue, setValue);
  p->SetEvents(valueEvent, rangeEvent);
  // p->UnRegister();

  SmartPtr<AbstractEditableNumericValueModel<TVal> > pout(p);
  return pout;
}

#endif // EDITABLENUMERICVALUEMODEL_H
