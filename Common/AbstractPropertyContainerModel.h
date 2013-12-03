#ifndef ABSTRACTPROPERTYCONTAINERMODEL_H
#define ABSTRACTPROPERTYCONTAINERMODEL_H

#include "PropertyModel.h"
#include "Registry.h"

/**
 * A helper class for AbstractPropertyContainerModel. This is a typeless parent
 * for ConcretePropertyHolder.
 */
class ConcretePropertyHolderBase : public itk::Object
{
public:
  irisITKAbstractObjectMacro(ConcretePropertyHolderBase, itk::Object)

  virtual void DeepCopy(const ConcretePropertyHolderBase *source) = 0;
  virtual bool Equals(const ConcretePropertyHolderBase *other) = 0;
  virtual void Serialize(Registry &folder) const = 0;
  virtual void Deserialize(Registry &folder) = 0;
};

/**
 * A helper class for AbstractPropertyContainerModel that holds a pointer to
 * a ConcretePointerModel and supports copy and serialization operations.
 */
template <class TAtomic, class TDomain>
class ConcretePropertyHolder : public ConcretePropertyHolderBase
{
public:
  // ITK stuff
  typedef ConcretePropertyHolder<TAtomic, TDomain> Self;
  typedef ConcretePropertyHolderBase Superclass;
  typedef SmartPtr<Self> Pointer;
  typedef SmartPtr<const Self> ConstPointer;

  itkTypeMacro(ConcretePropertyHolder, ConcretePropertyHolderBase)
  itkNewMacro(Self)

  // Held property type
  typedef ConcretePropertyModel<TAtomic, TDomain> PropertyType;

  virtual void DeepCopy(const ConcretePropertyHolderBase *source)
  {
    const Self *source_cast = static_cast<const Self *>(source);
    PropertyType *source_prop = source_cast->m_Property;
    m_Property->DeepCopy(source_prop);
  }

  virtual bool Equals(const ConcretePropertyHolderBase *other)
  {
    const Self *source_cast = static_cast<const Self *>(other);
    PropertyType *source_prop = source_cast->m_Property;
    return m_Property->Equals(source_prop);
  }

  virtual void Serialize(Registry &folder) const
  {
    TAtomic value;
    if(m_Property->GetValueAndDomain(value, NULL))
      folder[m_RegistryKey] << value;
  }

  virtual void Deserialize(Registry &folder)
  {
    RegistryValue &rv = folder.Entry(m_RegistryKey);
    if(!rv.IsNull())
      m_Property->SetValue(rv[m_Property->GetValue()]);
  }

  irisGetSetMacro(Property, PropertyType *)
  irisGetSetMacro(RegistryKey, const std::string &)

protected:

  // Pointer to the property
  SmartPtr<PropertyType> m_Property;

  // Registry key for serialization
  std::string m_RegistryKey;

};


/**
 * This class is intended to serve as a parent class for models that hold
 * a (large) number of individual ConcretePropertyModel objects. For example,
 * we may want a FooSettings class that holds a bunch of properties that
 * influence the behavior of Foo. Normally, one would write a struct with
 * these different fields:
 *
 *   struct FooSettings {
 *     int FooWidth;
 *     double FooAspectRatio;
 *     bool IsFooable;
 *   }
 *
 * However, in the model/view paradigm in ITK-SNAP, we want each of the fields
 * to be represented by a PropertyModel so that observers can listen to changes
 * in the individual fields, and so that the fields can be hooked up to GUI
 * widgets. So instead, FooSettings is represented like this:
 *
 *    class FooSettingsModel : public AbstractModel
 *    {
 *    public:
 *      ...
 *      irisRangedPropertyAccessMacro(FooWidth, int)
 *      irisRangedPropertyAccessMacro(FooAspectRatio, double)
 *      irisSimplePropertyAccessMacro(IsFooable, bool)
 *    protected:
 *      ...
 *      SmartPtr<ConcreteRangedIntProperty> m_FooWidth;
 *      SmartPtr<ConcreteRangedDoubleProperty> m_FooAspectRatio;
 *      SmartPtr<ConcreteSimpleBooleanProperty> m_IsFooable;
 *    }
 *
 * However, this class does not automatically offer comparison operators or
 * means to copy the model (deep copy). Writing that code by hand is a pain,
 * especially when there are lots of fields. So this is where the class
 * AbstractPropertyContainerModel comes in. It allows the fields to be registered
 * in the constructor, and provides default implementations of the comparison
 * operators and DeepCopy function.
 *
 * The class AbstractPropertyContainerModel is a way to create a container of
 * heterogeneous property models that supports comparison, serialization, and
 * copy operations with little coding. The FooSettingsModel class will still have
 * a similar structure to the one above.
 *
 *    class FooSettingsModel : public AbstractPropertyContainerModel
 *    {
 *    public:
 *      ...
 *      irisRangedPropertyAccessMacro(FooWidth, int)
 *      irisRangedPropertyAccessMacro(FooAspectRatio, double)
 *      irisSimplePropertyAccessMacro(IsFooable, bool)
 *    protected:
 *      ...
 *      SmartPtr<ConcreteRangedIntProperty> m_FooWidth;
 *      SmartPtr<ConcreteRangedDoubleProperty> m_FooAspectRatio;
 *      SmartPtr<ConcreteSimpleBooleanProperty> m_IsFooable;
 *    }
 *
 * The difference is that in the constructor, when the properties are created
 * using the NewSimpleProperty/NewRangedProperty methods, they are automatically
 * added to an internal list, allowing comparison, copy, and serialization.
 *
 *    FooSettingsModel::FooSettingsModel
 *    {
 *      m_FooWidth = NewRangedProperty("FooWidth", 2, 0, 4, 1);
 *      m_FooAspectRatio = NewSimpleProperty("FooAspectRatio", 0.25, 0.0, 1.0, 0.01);
 *      m_IsFooable = NewSimpleProperty("FooWidth", true);
 *    }
 */
class AbstractPropertyContainerModel : public AbstractModel
{
public:
  irisITKObjectMacro(AbstractPropertyContainerModel, AbstractModel)

  void DeepCopy(const AbstractPropertyContainerModel *source);

  virtual bool operator == (const AbstractPropertyContainerModel &source);

  virtual bool operator != (const AbstractPropertyContainerModel &source);

  void WriteToRegistry(Registry &folder) const;

  void ReadFromRegistry(Registry &folder);

protected:

  // Register a child model with this class. This should be called in the
  // constructor when the model is created. The model should be a concrete
  // property model. This method should only be called in the constructor
  template <class TAtomic, class TDomain>
  SmartPtr< ConcretePropertyModel<TAtomic, TDomain> >
  RegisterProperty(const std::string &key,
                   SmartPtr< ConcretePropertyModel<TAtomic, TDomain> > model)
  {
    typedef ConcretePropertyHolder<TAtomic, TDomain> HolderType;
    SmartPtr<HolderType> holder = HolderType::New();
    holder->SetProperty(model);
    holder->SetRegistryKey(key);
    m_Properties.insert(std::make_pair(key, holder));
    return model;
  }

  // Retrieve a property
  template <class TAtomic, class TDomain>
  ConcretePropertyModel<TAtomic, TDomain> *
  GetProperty(const std::string &key) const
  {
    const ConcretePropertyHolder<TAtomic, TDomain> *holder =
        static_cast<const ConcretePropertyHolder<TAtomic, TDomain> *> (
          m_Properties.find(key)->second.GetPointer());

    return holder->GetProperty();
  }

  // A convenience method that creates a new simple concrete property and
  // then registers it using RegisterModel
  template <class TAtomic>
  SmartPtr< ConcretePropertyModel<TAtomic, TrivialDomain> >
  NewSimpleProperty(const std::string &key, const TAtomic &value)
  {
    return RegisterProperty(key, NewSimpleConcreteProperty(value));
  }

  // A convenience method that creates a new simple concrete property and
  // then registers it using RegisterModel
  template <class TAtomic>
  SmartPtr< ConcretePropertyModel<TAtomic, NumericValueRange<TAtomic> > >
  NewRangedProperty(const std::string &key,
                    const TAtomic &value,
                    const TAtomic &minval,
                    const TAtomic &maxval,
                    const TAtomic &step)
  {
    return RegisterProperty(key, NewRangedConcreteProperty(value, minval, maxval, step));
  }

private:

  // The storage for the fields
  typedef SmartPtr<ConcretePropertyHolderBase> HolderPointer;
  typedef std::map<std::string, HolderPointer> PropertyMap;
  typedef PropertyMap::iterator PropertyMapIter;
  typedef PropertyMap::const_iterator PropertyMapCIter;

  PropertyMap m_Properties;

};

/*
 * These typedefs are commented out because I felt that they made the code too
 * hard to read. They can reduce coding, but can sow confusion.
 *
#define apcmRangedPropertyAccessMacro(name,type) \
  virtual AbstractPropertyModel<type, NumericValueRange<type> > * Get##name##Model () const \
    { return GetProperty<type, NumericValueRange<type> >("##name##"); } \
  virtual void Set##name (type _arg) \
    { this->Get##name##Model()->SetValue(_arg); } \
  virtual type Get##name () const \
    { return this->Get##name##Model()->GetValue(); }

#define apcmSimplePropertyAccessMacro(name,type) \
  virtual AbstractPropertyModel<type, TrivialDomain > * Get##name##Model () const \
    { return GetProperty<type, TrivialDomain >("##name##"); } \
  virtual void Set##name (type _arg) \
    { this->Get##name##Model()->SetValue(_arg); } \
  virtual type Get##name () const \
    { return this->Get##name##Model()->GetValue(); }

#define apcmGenericPropertyAccessMacro(name,type,domaintype) \
  virtual AbstractPropertyModel<type, domaintype > * Get##name##Model () const \
    { return GetProperty<type, domaintype >("##name##"); } \
  virtual void Set##name (type _arg) \
    { this->Get##name##Model()->SetValue(_arg); } \
  virtual type Get##name () const \
    { return this->Get##name##Model()->GetValue(); }
*/


#endif // ABSTRACTPROPERTYCONTAINERMODEL_H
