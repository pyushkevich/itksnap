#ifndef PROPERTY_H
#define PROPERTY_H

#include "IRISObserverPattern.h"
#include "SNAPEvents.h"

/*

template <class TAtomic> class ConstProperty : public IRISObservable
{
public:
  FIRES(ProperyChangeEvent)

  ConstProperty() {}
  virtual ~ConstProperty() {}

  operator TAtomic() { return m_Value; }
  operator const TAtomic() const { return m_Value; }


protected:
  TAtomic m_Value;
};

template <class TAtomic> class Property : public ConstProperty<TAtomic>
{
public:

  Property() {}
  virtual ~Property() {}

  // Standard operators
  Property<TAtomic> & operator = (const TAtomic &a)
  {
    if(this->m_Value != a)
      {
      this->m_Value = a;
      this->InvokeEvent(PropertyChangeEvent());
      }
    return *this;
  }

protected:
};

template <class TAtomic, class NameTraits> class NamedProperty
    : public Property<TAtomic>
{
public:

  NamedProperty() {}
  virtual ~NamedProperty() {}

  static const char *GetName() { return NameTraits::Name(); }

  virtual NamedProperty<TAtomic, NameTraits> & operator = (const TAtomic &a)
  {
    Property<TAtomic>::operator =(a);
    return *this;
  }

};

*/

/**
  Declare a property, usually in the private or protected section of the
  code. For example

  irisPropertyDecl(Cheese, int)

  creates property m_Cheese that subclasses from Property<int>
  */
//#define irisPropertyDeclMacro(name, type) \
//  struct _PropertyTraits##name  { static const char* Name() { return #name; } }; \
//  NamedProperty<type, _PropertyTraits##name> m_##name;

/**
  Accessor for a property. Exposes the property, but usage is the same as
  irisGetMacro for accessing the value of the property.
  */
//#define irisGetPropertyMacro(name, type) \
//  ConstProperty<type> &Get##name() { return m_##name; }

//#define irisSetPropertyMacro(name, type) irisSetMacro(name, type)

#endif // PROPERTY_H
