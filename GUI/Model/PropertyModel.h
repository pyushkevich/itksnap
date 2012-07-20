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
#include <map>

/**
  This class represents the range information associated with a numeric
  value. This range information is used to set up the GUI controls with
  which the user interacts.
  */
template<class TVal> struct NumericValueRange
{
  typedef NumericValueRange<TVal> Self;

  // These values define a numeric value range
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

  bool operator == (const Self &comp)
  {
    return (Minimum == comp.Minimum) && (Maximum == comp.Maximum) && (StepSize == comp.StepSize);
  }

  bool operator != (const Self &comp)
  {
    return (Minimum != comp.Minimum) || (Maximum != comp.Maximum) || (StepSize != comp.StepSize);
  }
};

/**
  This class represents a domain that allows all values in a data type. It
  can be used with the class AbstractPropertyModel when there is no need to
  communicate domain information to the widget
  */
class TrivialDomain
{
public:
  bool operator == (const TrivialDomain &cmp) { return true; }
  bool operator != (const TrivialDomain &cmp) { return false; }

};

/**
  Another type of a domain is a set of items/options from which the user is
  able to choose. Examples can be lists of strings, lists of color labels, and
  so on. The value is of type TVal, but this is not necessarily the information
  that it presented to the user. For example, in a color label chooser, the
  value held by a property is the ID of the label, but the user is shown the
  color and the description of the label.

  The signature for this type of domain consists of a const_iterator typedef,
  begin() and end() methods that return a const_iterator(), the method
  GetValue(it) which returns the numeric value associated with an iterator
  and the method GetDescription(it), which returns the information used by
  the GUI to present the choice to the user.

  The actual implementations of this domain are normally wrappers around STL
  structures.
*/
template <class TVal, class TDesc, class TIterator>
class AbstractItemSetDomain
{
public:
  typedef TIterator const_iterator;
  typedef TVal ValueType;
  typedef TDesc DescriptorType;

  virtual const_iterator begin() const = 0;
  virtual const_iterator end() const  = 0;
  virtual const_iterator find(const TVal &value) const = 0;
  virtual TVal GetValue(const const_iterator &it) const  = 0;
  virtual TDesc GetDescription(const const_iterator &it) const  = 0;
};

/**
  This is an implementation of the domain that wraps around an stl::map from
  values to descriptors. The map is not stored in the domain, but referenced
  from another object to avoid duplicating data.
  */
template <class TVal, class TDesc>
class STLMapItemSetDomain :
    public AbstractItemSetDomain<TVal, TDesc,
                                 typename std::map<TVal,TDesc>::const_iterator>
{
public:
  typedef STLMapItemSetDomain<TVal, TDesc> Self;
  typedef typename std::map<TVal, TDesc> MapType;
  typedef typename MapType::const_iterator const_iterator;

  STLMapItemSetDomain() { m_SourceMap = NULL; }
  STLMapItemSetDomain(const MapType *refmap) { m_SourceMap = refmap; }

  const_iterator begin() const
    { assert(m_SourceMap); return m_SourceMap->begin(); }

  const_iterator end() const
    { assert(m_SourceMap); return m_SourceMap->end(); }

  const_iterator find(const TVal &value) const
    { assert(m_SourceMap); return m_SourceMap->find(value); }

  TVal GetValue(const const_iterator &it) const
    { return it->first; }

  TDesc GetDescription(const const_iterator &it) const
    { return it->second; }

  bool operator == (const Self &cmp) const
    { return m_SourceMap == cmp.m_SourceMap; }

  bool operator != (const Self &cmp) const
    { return m_SourceMap != cmp.m_SourceMap; }

private:
  const MapType *m_SourceMap;
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

  /**
    A getter with a simple signature. Not meant to be overridden by the
    child class.
    */
  TVal GetValue()
  {
    TVal value;
    GetValueAndDomain(value, NULL);
    return value;
  }
};


/**
  A concrete implementation of AbstractPropertyModel that holds the value,
  the validity flag, and the domain as private variables. The validity flag
  is initialized to true. The parent model is responsible for setting the
  value, domain, and validity flag inside of this concrete model.
  */
template <class TVal, class TDomain = TrivialDomain>
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

  irisSetWithEventMacro(Value, TVal, ValueChangedEvent)
  irisSetWithEventMacro(Domain, TDomain, DomainChangedEvent)
  irisSetWithEventMacro(IsValid, bool, ValueChangedEvent)

protected:

  ConcretePropertyModel()
  {
    m_IsValid = true;
  }

  virtual ~ConcretePropertyModel() {}

  TVal m_Value;
  TDomain m_Domain;
  bool m_IsValid;
};

// Definitions of common concrete property models with numeric ranges
typedef ConcretePropertyModel<double, NumericValueRange<double> > RangedDoublePropertyModel;
typedef ConcretePropertyModel<float, NumericValueRange<float> > RangedFloatPropertyModel;
typedef ConcretePropertyModel<int, NumericValueRange<int> > RangedIntPropertyModel;
typedef ConcretePropertyModel<unsigned int, NumericValueRange<unsigned int> > RangedUIntPropertyModel;
typedef ConcretePropertyModel<short, NumericValueRange<short> > RangedShortPropertyModel;
typedef ConcretePropertyModel<unsigned short, NumericValueRange<unsigned short> > RangedUShortPropertyModel;
typedef ConcretePropertyModel<char, NumericValueRange<int> > RangedCharPropertyModel;
typedef ConcretePropertyModel<unsigned char, NumericValueRange<unsigned char> > RangedUCharPropertyModel;

// Definitions of common concrete property models without numeric ranges
typedef ConcretePropertyModel<bool> BoolPropertyModel;

// A macro to generate functions GetXXX(), SetXXX() and GetXXXModel() in a class
// that contains a ConcretePropertyModel of a certain type named m_XXXModel
#define irisRangedPropertyAccessMacro(name,type) \
  virtual void Set##name (type _arg) \
    { this->m_##name##Model->SetValue(_arg); } \
  virtual type Get##name () const \
    { return this->m_##name##Model->GetValue(); } \
  virtual AbstractPropertyModel<type, NumericValueRange<type> > * Get##name##Model () const \
    { return this->m_##name##Model; }

#define irisSimplePropertyAccessMacro(name,type) \
  virtual void Set##name (type _arg) \
    { this->m_##name##Model->SetValue(_arg); } \
  virtual type Get##name () const \
    { return this->m_##name##Model->GetValue(); } \
  virtual AbstractPropertyModel<type, TrivialDomain > * Get##name##Model () const \
    { return this->m_##name##Model; }

// A factory function to initialize properties - again, for shorter code
template <class TVal>
SmartPtr< ConcretePropertyModel<TVal, NumericValueRange<TVal> > >
NewRangedConcreteProperty(TVal val, TVal rmin, TVal rmax, TVal rstep)
{
  typedef ConcretePropertyModel<TVal, NumericValueRange<TVal> > Prop;
  SmartPtr<Prop> p = Prop::New();
  p->SetValue(val);
  p->SetDomain(NumericValueRange<TVal>(rmin, rmax, rstep));
  return p;
}

template <class TVal>
SmartPtr< ConcretePropertyModel<TVal, TrivialDomain > >
NewSimpleConcreteProperty(TVal val)
{
  typedef ConcretePropertyModel<TVal, TrivialDomain > Prop;
  SmartPtr<Prop> p = Prop::New();
  p->SetValue(val);
  return p;
}



/**
  This class is only used to define some typedefs. It allows us to write

      AbstractRangedPropertyModel<double>::Type

  as shorthand for

      AbstractPropertyModel<double, NumericValueRange<double> >
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
    listen to and rebroadcast as ValueChangedEvent and DomainChangedEvent.
    */
  void SetEvents(const itk::EventObject &valueEvent,
                 const itk::EventObject &rangeEvent)
  {
    Rebroadcast(m_Model, valueEvent, ValueChangedEvent());
    Rebroadcast(m_Model, rangeEvent, DomainChangedEvent());
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

#endif // EDITABLENUMERICVALUEMODEL_H
