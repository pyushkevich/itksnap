#ifndef REGISTRYENTRYPROPERTYMODEL_H
#define REGISTRYENTRYPROPERTYMODEL_H

#include "PropertyModel.h"
#include "Registry.h"

/**
 * A model encapsulating a registry entry. The registry mechanism does not
 * support metadata, so the registry entry can only store the data itself.
 * The domain is supplied by the user.
 */
template <class TVal, class TDomain>
class RegistryEntryPropertyModel
    : public AbstractModel<TVal, TDomain>
{
public:

  // Standard ITK stuff (can't use irisITKObjectMacro because of two template
  // parameters, comma breaks the macro).
  typedef RegistryEntryPropertyModel<TVal, TDomain> Self;
  typedef AbstractPropertyModel<TVal, TDomain> Superclass;
  typedef SmartPtr<Self> Pointer;
  typedef SmartPtr<const Self> ConstPointer;

  itkTypeMacro(RegistryEntryPropertyModel, AbstractPropertyModel)
  itkNewMacro(Self)

  /**
   * Initialize the model with a pointer to the registry (which must remain
   * valid), the key, and a default value for the property
   */
  void Initialize(Registry *registry, const char *key, TVal deflt)
  {
    m_Registry = registry;
    m_Key = key;
    m_DefaultValue = deflt;

    // Listen to events from the corresponding folder (how)
  }

  irisSetWithEventMacro(Domain, TDomain, DomainChangedEvent)

  virtual bool GetValueAndDomain(TVal &value, TDomain *domain)
  {
    if(!m_Registry) return false;

    value = (*m_Registry)[m_Key][m_DefaultValue];
    if(domain)
      *domain = m_Domain;

    return true;
  }

  virtual void SetValue(TVal value)
  {
    assert(m_Registry);
    (*m_Registry)[m_Key] << value;
  }

protected:

  // The registry
  Registry *m_Registry;

  // Key
  const char *m_Key;

  // Default value
  TVal m_DefaultValue;

  // The domain is stored in the model
  TDomain m_Domain;
};

#endif // REGISTRYENTRYPROPERTYMODEL_H
