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

  NumericValueRange(const NumericValueRange<TVal> &ref)
  {
    Minimum = ref.Minimum;
    Maximum = ref.Maximum;
    StepSize = ref.StepSize;
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

  // An atomic domain holds its own state, so it is possible to compare two
  // atomic domains to determine if they are the same or different. Domains
  // that store references to external objects are not atomic.
  virtual bool isAtomic() { return true; }


};

/**
  This computes a step size that is a power of 10
  */
inline double CalculatePowerOfTenStepSize(double min, double max, size_t minNumSteps)
{
  double stepUB = (max - min) / minNumSteps;
  return pow(10, floor(log10(stepUB)));
}


/**
  An abstract parent type for models that allow random access to items of
  some type. This abstract class is agnostic to the actual storage type of
  the source container. For implementations, see RandomAccessCollectionModel

  TODO: reconcile this with domains!

  */
template <class TItem>
class AbstractRandomAccessCollectionModel : public AbstractModel
{
public:
  typedef TItem ItemType;
  virtual unsigned int GetSize() = 0;
  virtual TItem operator[] (unsigned int n) = 0;
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

  // An atomic domain holds its own state, so it is possible to compare two
  // atomic domains to determine if they are the same or different. Domains
  // that store references to external objects are not atomic.
  bool isAtomic() { return true; }
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
  virtual ~AbstractItemSetDomain() {}
};

/**
  This is an implementation of the domain that wraps around an stl::map from
  values to descriptors. The map is not stored in the domain, but referenced
  from another object to avoid duplicating data.
  */
template <class TVal, class TDesc>
class STLMapWrapperItemSetDomain :
    public AbstractItemSetDomain<TVal, TDesc,
                                 typename std::map<TVal,TDesc>::const_iterator>
{
public:
  typedef STLMapWrapperItemSetDomain<TVal, TDesc> Self;
  typedef typename std::map<TVal, TDesc> MapType;
  typedef typename MapType::const_iterator const_iterator;

  STLMapWrapperItemSetDomain() { m_SourceMap = NULL; }
  STLMapWrapperItemSetDomain(const MapType *refmap) { m_SourceMap = refmap; }
  virtual ~STLMapWrapperItemSetDomain() {}

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

  void SetWrappedMap(const MapType *refmap) { m_SourceMap = refmap; }

  virtual bool operator == (const Self &cmp) const
    { return m_SourceMap == cmp.m_SourceMap; }

  virtual bool operator != (const Self &cmp) const
    { return m_SourceMap != cmp.m_SourceMap; }

  // An atomic domain holds its own state, so it is possible to compare two
  // atomic domains to determine if they are the same or different. Domains
  // that store references to external objects are not atomic.
  virtual bool isAtomic() { return false; }

protected:
  const MapType *m_SourceMap;
};


/**
  This is an implementation of the domain that wraps around an stl::vector
  of descriptors. TVal should be an integer type that can be used as an index
  (int, unsigned int, enum, etc)
  */
template <class TVal, class TDesc>
class STLVectorWrapperItemSetDomain :
    public AbstractItemSetDomain<TVal, TDesc,
                                 typename std::vector<TDesc>::const_iterator>
{
public:
  typedef STLVectorWrapperItemSetDomain<TVal, TDesc> Self;
  typedef typename std::vector<TDesc> VectorType;
  typedef typename VectorType::const_iterator const_iterator;

  STLVectorWrapperItemSetDomain() { m_SourceVector = NULL; }
  STLVectorWrapperItemSetDomain(const VectorType *refvec) { m_SourceVector = refvec; }
  virtual ~STLVectorWrapperItemSetDomain() {}

  const_iterator begin() const
    { assert(m_SourceVector); return m_SourceVector->begin(); }

  const_iterator end() const
    { assert(m_SourceVector); return m_SourceVector->end(); }

  const_iterator find(const TVal &value) const
    { assert(m_SourceVector); return m_SourceVector->begin() + value; }

  TVal GetValue(const const_iterator &it) const
    { assert(m_SourceVector); return it - m_SourceVector->begin(); }

  TDesc GetDescription(const const_iterator &it) const
    { assert(m_SourceVector); return *it; }

  virtual bool operator == (const Self &cmp) const
    { return m_SourceVector == cmp.m_SourceVector; }

  virtual bool operator != (const Self &cmp) const
    { return m_SourceVector != cmp.m_SourceVector; }

  // An atomic domain holds its own state, so it is possible to compare two
  // atomic domains to determine if they are the same or different. Domains
  // that store references to external objects are not atomic.
  virtual bool isAtomic() { return false; }

protected:
  const VectorType *m_SourceVector;
};

/**
  This is an item domain implementation that is just an stl::map, i.e., it
  owns the data, as opposed to STLMapWrapperItemSetDomain, which references
  the data from another map. This implementation is useful for small domains
  where there is no cost in passing the domain by value.
  */
template<class TVal, class TDesc>
class SimpleItemSetDomain : public
    AbstractItemSetDomain<TVal, TDesc, typename std::map<TVal,TDesc>::const_iterator>
{
public:
  typedef std::map<TVal, TDesc> MapType;
  typedef typename MapType::const_iterator const_iterator;
  typedef SimpleItemSetDomain<TVal, TDesc> Self;
  typedef AbstractItemSetDomain<TVal, TDesc, const_iterator> Superclass;

  SimpleItemSetDomain() : Superclass() { }

  const_iterator begin() const
    { return m_Map.begin(); }

  const_iterator end() const
    { return m_Map.end(); }

  const_iterator find(const TVal &value) const
    { return m_Map.find(value); }

  void clear()
    { m_Map.clear(); }

  TVal GetValue(const const_iterator &it) const
    { return it->first; }

  TDesc GetDescription(const const_iterator &it) const
    { return it->second; }

  unsigned int size() const
    { return m_Map.size(); }

  // Standard stl::map operator
  TDesc & operator [] (const TVal &key) { return m_Map[key]; }

  const TDesc & operator [] (const TVal &key) const { return m_Map[key]; }

  virtual bool operator == (const Self &cmp) const
    { return m_Map == cmp.m_Map; }

  virtual bool operator != (const Self &cmp) const
    { return m_Map != cmp.m_Map; }

  // An atomic domain holds its own state, so it is possible to compare two
  // atomic domains to determine if they are the same or different. Domains
  // that store references to external objects are not atomic.
  virtual bool isAtomic() { return true; }

protected:
  MapType m_Map;
};

/**
 * States that can be checked for property models. We place this enum outside
 * of the class AbstractPropertyModel because this class is templated. This
 * enum is meant to be used with the SNAPUIFlag framework.
 */
enum PropertyModelUIState
{
  // Indicates that the
  UIF_PROPERTY_IS_VALID = 0
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

  irisITKAbstractObjectMacro(AbstractPropertyModel, AbstractModel)

  /** The atomic type encompassed by the model */
  typedef TVal ValueType;

  /** The type of the domain */
  typedef TDomain DomainType;

  /** The model fires two types of events: ValueChangedEvent and
    DomainChangedEvent, in response to either the value or the domain
    having changed. */
  FIRES(ValueChangedEvent)
  FIRES(DomainChangedEvent)
  FIRES(StateMachineChangeEvent)

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

  /**
   * The model can participate in the state management mechanism with SNAPUIFlag.
   * At the moment, the only flag available is the Validity flag.
   */
  bool CheckState(PropertyModelUIState flag)
  {
    if(flag == UIF_PROPERTY_IS_VALID)
      {
      TVal value;
      return GetValueAndDomain(value, NULL);
      }
    else return false;
  }

  /**
    Sometimes it is useful to have the model rebroadcast value and domain
    change events from another model. An example may be a model that wraps
    around another model, e.g., if model A is of a compound type T and
    model B is used to access an attribute T.x in T, then we want the value
    change events in A to be rebroadcast as value change events in B. This
    function simplifies making this connection
    */
  void RebroadcastFromSourceProperty(AbstractModel *source)
  {
    Rebroadcast(source, ValueChangedEvent(), ValueChangedEvent());
    Rebroadcast(source, DomainChangedEvent(), DomainChangedEvent());
  }

protected:

  AbstractPropertyModel()
  {
    Rebroadcast(this, ValueChangedEvent(), StateMachineChangeEvent());
    Rebroadcast(this, DomainChangedEvent(), StateMachineChangeEvent());
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

  // Simple implementation of the deep copy function
  void DeepCopy(const Self *source)
  {
    // Copy the relevant stuff
    this->SetValue(source->m_Value);
    this->SetDomain(source->m_Domain);
    this->SetIsValid(source->m_IsValid);
  }

  /** Compare with another model (by value only, not domain) */
  bool Equals(const Self *source) const
  {
    // Cast to the right type
    return(source->m_Value == m_Value &&
           source->m_IsValid == m_IsValid);
  }


protected:

  ConcretePropertyModel()
    : m_Value(TVal()), m_Domain(TDomain()), m_IsValid(true) {}

  virtual ~ConcretePropertyModel() {}

  TVal m_Value;
  TDomain m_Domain;
  bool m_IsValid;
};

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

#define irisGenericPropertyAccessMacro(name,type,domaintype) \
  virtual void Set##name (type _arg) \
    { this->m_##name##Model->SetValue(_arg); } \
  virtual type Get##name () const \
    { return this->m_##name##Model->GetValue(); } \
  virtual AbstractPropertyModel<type, domaintype> * Get##name##Model () const \
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

template <class TVal, class TDomain>
SmartPtr< ConcretePropertyModel<TVal, TDomain> >
NewConcreteProperty(TVal val, TDomain domain)
{
  typedef ConcretePropertyModel<TVal, TDomain> Prop;
  SmartPtr<Prop> p = Prop::New();
  p->SetValue(val);
  p->SetDomain(domain);
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

template <class TVal>
class ConcreteRangedPropertyModel
{
public:
  typedef NumericValueRange<TVal> DomainType;
  typedef ConcretePropertyModel<TVal, DomainType> Type;
};

/*
Make some typedefs. The macros below define types

  AbstractSimpleXXXProperty
  AbstractRangedXXXProperty
  ConcreteSimpleXXXProperty
  ConcreteRangedXXXProperty
*/

#define MAKE_TYPEDEF_PM_RANGED(type,name) \
  typedef AbstractPropertyModel< type > AbstractSimple##name##Property; \
  typedef AbstractRangedPropertyModel< type >::Type AbstractRanged##name##Property; \
  typedef ConcretePropertyModel< type > ConcreteSimple##name##Property; \
  typedef ConcreteRangedPropertyModel< type >::Type ConcreteRanged##name##Property;

#define MAKE_TYPEDEF_PM_NONRNG(type,name) \
  typedef AbstractPropertyModel< type > AbstractSimple##name##Property; \
  typedef ConcretePropertyModel< type > ConcreteSimple##name##Property;

// Macros for standard types that support ranges
MAKE_TYPEDEF_PM_RANGED(double,          Double)
MAKE_TYPEDEF_PM_RANGED(float,           Float)
MAKE_TYPEDEF_PM_RANGED(long,            Long)
MAKE_TYPEDEF_PM_RANGED(unsigned long,   ULong)
MAKE_TYPEDEF_PM_RANGED(int,             Int)
MAKE_TYPEDEF_PM_RANGED(unsigned int,    UInt)
MAKE_TYPEDEF_PM_RANGED(short,           Short)
MAKE_TYPEDEF_PM_RANGED(unsigned short,  UShort)
MAKE_TYPEDEF_PM_RANGED(char,            Char)
MAKE_TYPEDEF_PM_RANGED(unsigned char,   UChar)
MAKE_TYPEDEF_PM_RANGED(bool,            Boolean)

// Common SNAP typedefs
MAKE_TYPEDEF_PM_RANGED(LabelType,       LabelType)

// Common vector types
MAKE_TYPEDEF_PM_RANGED(Vector2d,        DoubleVec2)
MAKE_TYPEDEF_PM_RANGED(Vector3d,        DoubleVec3)
MAKE_TYPEDEF_PM_RANGED(Vector2i,        IntVec2)
MAKE_TYPEDEF_PM_RANGED(Vector3i,        IntVec3)
MAKE_TYPEDEF_PM_RANGED(Vector2ui,       UIntVec2)
MAKE_TYPEDEF_PM_RANGED(Vector3ui,       UIntVec3)
MAKE_TYPEDEF_PM_RANGED(Vector2b,        BooleanVec2)
MAKE_TYPEDEF_PM_RANGED(Vector3b,        BooleanVec3)

// Macros for non-ranged types
MAKE_TYPEDEF_PM_NONRNG(std::string,     String)




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
  Instead, one should make use of the function wrapGetterSetterPairAsProperty, which
  serves as a factory for creating models of this type.
*/
template<class TVal, class TDomain, class TModel, class GetterTraits, class SetterTraits>
class FunctionWrapperPropertyModel
    : public AbstractPropertyModel<TVal, TDomain>
{
public:

  // Standard ITK stuff (can't use irisITKObjectMacro because of two template
  // parameters, comma breaks the macro).
  typedef FunctionWrapperPropertyModel<TVal, TDomain, TModel, GetterTraits, SetterTraits> Self;
  typedef AbstractPropertyModel<TVal, TDomain> Superclass;
  typedef SmartPtr<Self> Pointer;
  typedef SmartPtr<const Self> ConstPointer;

  itkTypeMacro(FunctionWrapperPropertyModel, AbstractPropertyModel)
  itkNewMacro(Self)

  // Function pointers to a setter method
  typedef typename SetterTraits::Setter Setter;

  // The function pointer to the getter is provided by the traits
  typedef typename GetterTraits::Getter Getter;

  /** Initializes a model with a parent model and function pointers */
  void Initialize(TModel *model, Getter getter, Setter setter = NULL)
  {
    m_Model = model;
    m_Getter = getter;
    m_Setter = setter;
  }

  /** Get a reference to the getter traits */
  GetterTraits &GetGetterTraits() { return m_GetterTraits; }

  /** Get a reference to the setter traits */
  SetterTraits &GetSetterTraits() { return m_SetterTraits; }

  /**
    Set up the events fired by the parent model that this model should
    listen to and rebroadcast as ValueChangedEvent and DomainChangedEvent.
    */
  void SetEvents(const itk::EventObject &valueEvent,
                 const itk::EventObject &rangeEvent)
  {
    this->Rebroadcast(m_Model, valueEvent, ValueChangedEvent());
    this->Rebroadcast(m_Model, rangeEvent, DomainChangedEvent());
  }


  bool GetValueAndDomain(TVal &value, TDomain *domain)
  {
    // This is important! Before calling the getter function, we should allow
    // the model to respond to whatever events it may have received that led
    // to this data request. Otherwise, we would have to make an Update()
    // call in each of the Getter functions we write.
    m_Model->Update();

    // Call the getter function with the help of the traits object
    return m_GetterTraits.GetValueAndDomain(m_Model, m_Getter, value, domain);
  }

  void SetValue(TVal value)
  {
    if(m_Setter)
      {
      static_cast<AbstractModel *>(m_Model)->Update();
      m_SetterTraits.SetValue(m_Model, m_Setter, value);
      }
  }

  /**
    A factory method used to create new models of this type. The user should
    not need to call this directly, instead use the wrapGetterSetterPairAsProperty
    methods below. The last two paremeters are the events fired by the parent
    model that should be rebroadcast as the value and domain change events by
    the property model.
    */
  static SmartPtr<Superclass> CreatePropertyModel(
      TModel *parentModel,
      Getter getter, Setter setter,
      const itk::EventObject &valueEvent,
      const itk::EventObject &rangeEvent,
      GetterTraits getterTraits = GetterTraits(),
      SetterTraits setterTraits = SetterTraits())
  {
    SmartPtr<Self> p = Self::New();
    p->Initialize(parentModel, getter, setter);
    p->SetEvents(valueEvent, rangeEvent);
    p->m_SetterTraits = setterTraits;
    p->m_GetterTraits = getterTraits;

    // p->UnRegister();

    SmartPtr<Superclass> pout(p);
    return pout;
  }

protected:

  TModel *m_Model;
  Getter m_Getter;
  Setter m_Setter;
  GetterTraits m_GetterTraits;
  SetterTraits m_SetterTraits;

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
  bool GetValueAndDomain(
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
  bool GetValueAndDomain(
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
  bool GetValueAndDomain(
      TModel *model, Getter getter, TVal &value, TDomain *domain)
  {
    value = ((*model).*(getter))();
    return true;
  }
};

/**
  Basic setter traits
  */
template <class TVal, class TModel>
class FunctionWrapperPropertyModelSimpleSetterTraits
{
public:
  // Signature of the getter
  typedef void (TModel::*Setter)(TVal value);

  // Implementation of the get method, just calls the getter directly
  void SetValue(
      TModel *model, Setter getter, TVal &value)
  {
    ((*model).*(getter))(value);
  }
};

class FunctionWrapperPropertyModelIndexedTraits
{
public:

  void SetIndex(int index) { m_Index = index; }
  int GetIndex() const { return m_Index; }

protected:
  int m_Index;
};

/**
  Getter traits for the FunctionWrapperPropertyModel that use the compound
  getter signature, with a parameter variable i. This is useful when we need
  to create an array of models that wrap around function of the form

    bool GetValueAndDomain(int i, TVal &value, TDomain &domain);
*/
template <class TVal, class TDomain, class TModel>
class FunctionWrapperPropertyModelIndexedCompoundGetterTraits :
    public FunctionWrapperPropertyModelIndexedTraits
{
public:
  // Signature of the getter
  typedef bool (TModel::*Getter)(int i, TVal &t, TDomain *domain);

  // Implementation of the get method, just calls the getter directly
  bool GetValueAndDomain(
      TModel *model, Getter getter, TVal &value, TDomain *domain)
  {
    return ((*model).*(getter))(GetIndex(), value, domain);
  }
};


template <class TVal, class TDomain, class TModel>
class FunctionWrapperPropertyModelIndexedRangelessGetterTraits :
    public FunctionWrapperPropertyModelIndexedTraits
{
public:
  // Signature of the getter
  typedef bool (TModel::*Getter)(int index, TVal &t);

  // Implementation of the get method, just calls the getter directly
  bool GetValueAndDomain(
      TModel *model, Getter getter, TVal &value, TDomain *domain)
  {
    return ((*model).*(getter))(GetIndex(), value);
  }
};


/**
  Indexed setter traits
  */
template <class TVal, class TModel>
class FunctionWrapperPropertyModelIndexedSimpleSetterTraits :
    public FunctionWrapperPropertyModelIndexedTraits
{
public:
  // Signature of the getter
  typedef void (TModel::*Setter)(int index, TVal value);

  // Implementation of the get method, just calls the getter directly
  void SetValue(
      TModel *model, Setter getter, TVal &value)
  {
    ((*model).*(getter))(GetIndex(), value);
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
wrapGetterSetterPairAsProperty(
    TModel *model,
    bool (TModel::*getter)(TVal &, TDomain *),
    void (TModel::*setter)(TVal) = NULL,
    const itk::EventObject &valueEvent = ModelUpdateEvent(),
    const itk::EventObject &rangeEvent = ModelUpdateEvent())
{
  typedef FunctionWrapperPropertyModelCompoundGetterTraits<
      TVal, TDomain, TModel> GetterTraitsType;

  typedef FunctionWrapperPropertyModelSimpleSetterTraits<
      TVal, TModel> SetterTraitsType;

  typedef FunctionWrapperPropertyModel<
      TVal, TDomain, TModel, GetterTraitsType, SetterTraitsType> ModelType;

  return ModelType::CreatePropertyModel(model, getter, setter,
                                        valueEvent, rangeEvent);
}

template<class TModel, class TVal>
SmartPtr< AbstractPropertyModel<TVal> >
wrapGetterSetterPairAsProperty(
    TModel *model,
    bool (TModel::*getter)(TVal &),
    void (TModel::*setter)(TVal) = NULL,
    const itk::EventObject &valueEvent = ModelUpdateEvent(),
    const itk::EventObject &rangeEvent = ModelUpdateEvent())
{
  typedef FunctionWrapperPropertyModelRangelessGetterTraits<
      TVal, TrivialDomain, TModel> GetterTraitsType;

  typedef FunctionWrapperPropertyModelSimpleSetterTraits<
      TVal, TModel> SetterTraitsType;

  typedef FunctionWrapperPropertyModel<
      TVal, TrivialDomain, TModel, GetterTraitsType, SetterTraitsType> ModelType;

  return ModelType::CreatePropertyModel(model, getter, setter,
                                        valueEvent, rangeEvent);
}

template<class TModel, class TVal>
SmartPtr< AbstractPropertyModel<TVal> >
wrapGetterSetterPairAsProperty(
    TModel *model,
    TVal (TModel::*getter)(),
    void (TModel::*setter)(TVal) = NULL,
    const itk::EventObject &valueEvent = ModelUpdateEvent(),
    const itk::EventObject &rangeEvent = ModelUpdateEvent())
{
  typedef FunctionWrapperPropertyModelSimpleGetterTraits<
      TVal, TrivialDomain, TModel> GetterTraitsType;

  typedef FunctionWrapperPropertyModelSimpleSetterTraits<
      TVal, TModel> SetterTraitsType;

  typedef FunctionWrapperPropertyModel<
      TVal, TrivialDomain, TModel, GetterTraitsType, SetterTraitsType> ModelType;

  return ModelType::CreatePropertyModel(model, getter, setter,
                                        valueEvent, rangeEvent);
}

/**
  A version of wrapGetterSetterPairAsProperty that creates a model that wraps around
  a function GetXXXValueAndDomain(int i, TVal &value, TDomain *). This is
  useful when we want to create multiple models that wrap around the same
  getter/setter function.
  */
template<class TModel, class TVal, class TDomain>
SmartPtr< AbstractPropertyModel<TVal, TDomain> >
wrapIndexedGetterSetterPairAsProperty(
    TModel *model,
    int index,
    bool (TModel::*getter)(int, TVal &, TDomain *),
    void (TModel::*setter)(int, TVal) = NULL,
    const itk::EventObject &valueEvent = ModelUpdateEvent(),
    const itk::EventObject &rangeEvent = ModelUpdateEvent())
{
  typedef FunctionWrapperPropertyModelIndexedCompoundGetterTraits<
      TVal, TDomain, TModel> GetterTraitsType;

  typedef FunctionWrapperPropertyModelIndexedSimpleSetterTraits<
      TVal, TModel> SetterTraitsType;

  typedef FunctionWrapperPropertyModel<
      TVal, TDomain, TModel, GetterTraitsType, SetterTraitsType> ModelType;

  // Assign the index to the traits
  GetterTraitsType getterTraits;
  getterTraits.SetIndex(index);

  SetterTraitsType setterTraits;
  setterTraits.SetIndex(index);

  // Create the property model
  return ModelType::CreatePropertyModel(model, getter, setter,
                                        valueEvent, rangeEvent,
                                        getterTraits, setterTraits);
}

template<class TModel, class TVal>
SmartPtr< AbstractPropertyModel<TVal> >
wrapIndexedGetterSetterPairAsProperty(
    TModel *model, int index,
    bool (TModel::*getter)(int, TVal &),
    void (TModel::*setter)(int, TVal) = NULL,
    const itk::EventObject &valueEvent = ModelUpdateEvent(),
    const itk::EventObject &rangeEvent = ModelUpdateEvent())
{
  typedef FunctionWrapperPropertyModelIndexedRangelessGetterTraits<
      TVal, TrivialDomain, TModel> GetterTraitsType;

  typedef FunctionWrapperPropertyModelIndexedSimpleSetterTraits<
      TVal, TModel> SetterTraitsType;

  typedef FunctionWrapperPropertyModel<
      TVal, TrivialDomain, TModel, GetterTraitsType, SetterTraitsType> ModelType;

  // Assign the index to the traits
  GetterTraitsType getterTraits;
  getterTraits.SetIndex(index);

  SetterTraitsType setterTraits;
  setterTraits.SetIndex(index);

  return ModelType::CreatePropertyModel(model, getter, setter,
                                        valueEvent, rangeEvent,
                                        getterTraits, setterTraits);
}



/**
 * This model is used to decorate a member in a C++ struct as a property. This
 * is useful when we have a property of type X, and we want to be able to access
 * X.a as a property.
 */
template <class TStruct, class TMember, class TDomain>
class StructMemberWrapperPropertyModel
    : public AbstractPropertyModel<TMember, TDomain>
{
public:

  // Standard ITK stuff (can't use irisITKObjectMacro because of two template
  // parameters, comma breaks the macro).
  typedef StructMemberWrapperPropertyModel<TStruct, TMember, TDomain> Self;
  typedef AbstractPropertyModel<TMember, TDomain> Superclass;
  typedef SmartPtr<Self> Pointer;
  typedef SmartPtr<const Self> ConstPointer;

  itkTypeMacro(StructMemberWrapperPropertyModel, AbstractPropertyModel)
  itkNewMacro(Self)

  // Set the source model (the one that manages the TStruct)
  typedef AbstractPropertyModel<TStruct> ParentModel;
  void Initialize(ParentModel *model, size_t offset, const TDomain &domain)
  {
    m_ParentModel = model;
    m_MemberOffset = offset;
    m_Domain = domain;

    // Respond to value change events
    AbstractModel::Rebroadcast(m_ParentModel, ValueChangedEvent(), ValueChangedEvent());
  }

  bool GetValueAndDomain(TMember &value, TDomain *domain)
  {
    TStruct parentValue;
    if(m_ParentModel && m_ParentModel->GetValueAndDomain(parentValue,  NULL))
      {
      // Get the value of the member based on the offset
      TMember *valuePtr =
          reinterpret_cast<TMember *>(
            reinterpret_cast<char *>(&parentValue) + m_MemberOffset);

      value = *valuePtr;
      if(domain)
        *domain = m_Domain;

      return true;
      }
    return false;
  }

  void SetValue(TMember value)
  {
    TStruct parentValue;
    if(m_ParentModel && m_ParentModel->GetValueAndDomain(parentValue,  NULL))
      {
      // Get the value of the member based on the offset
      TMember *valuePtr =
          reinterpret_cast<TMember *>(
            reinterpret_cast<char *>(&parentValue) + m_MemberOffset);

      // Set the value
      if(*valuePtr != value)
        {
        *valuePtr = value;
        m_ParentModel->SetValue(parentValue);
        }
      }
  }

  static SmartPtr<Superclass> CreatePropertyModel(
      ParentModel *parentModel,
      size_t offset,
      TDomain domain)
  {
    SmartPtr<Self> p = Self::New();
    p->Initialize(parentModel, offset, domain);

    // p->UnRegister();

    SmartPtr<Superclass> pout(p);
    return pout;
  }

protected:
  StructMemberWrapperPropertyModel()
    : m_ParentModel(NULL), m_MemberOffset(0) {}


  ParentModel *m_ParentModel;
  size_t m_MemberOffset;
  TDomain m_Domain;
};

template <class TStruct, class TField>
SmartPtr< AbstractPropertyModel<TField> >
wrapStructMemberAsSimpleProperty(
    AbstractPropertyModel<TStruct> *parentModel,
    size_t offset)
{
  typedef StructMemberWrapperPropertyModel<TStruct, TField, TrivialDomain> ModelType;
  TrivialDomain td;
  return ModelType::CreatePropertyModel(parentModel, offset, td);
}

template <class TStruct, class TField>
SmartPtr< AbstractPropertyModel<TField, NumericValueRange<TField> > >
wrapStructMemberAsRangedProperty(
    AbstractPropertyModel<TStruct> *parentModel,
    size_t offset,
    const NumericValueRange<TField> &domain)
{
  typedef StructMemberWrapperPropertyModel<TStruct, TField, NumericValueRange<TField> > ModelType;
  return ModelType::CreatePropertyModel(parentModel, offset, domain);
}



#endif // EDITABLENUMERICVALUEMODEL_H
