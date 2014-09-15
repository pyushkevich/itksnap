#ifndef ABSTRACTLAYERINFOITEMSETDOMAIN_H
#define ABSTRACTLAYERINFOITEMSETDOMAIN_H

#include <PropertyModel.h>
#include <IRISApplication.h>

/**
  This is a base class for domain descriptions that provide some piece
  of information about every ITK-SNAP layer. This is used in conjunction
  with the PropertyModel system to provide the GUI with tables and combos
  that list layers, along with specific properties of interest.
  */
template <class TDesc>
class AbstractLayerInfoItemSetDomain :
    public AbstractItemSetDomain<size_t, TDesc, LayerIterator>
{
public:

  // To avoid storing pointers, we cast them into size_t objects. Otherwise
  // this screws up some of the Qt stuff
  typedef size_t ValueType;

  /** Initializes domain that iterates through layers based on the filter */
  AbstractLayerInfoItemSetDomain(IRISApplication *app = NULL,
                                 int role_filter = ALL_ROLES)
    { m_Driver = app; m_RoleFilter = role_filter; }

  LayerIterator begin() const
  {
    return m_Driver->GetCurrentImageData()->GetLayers(m_RoleFilter);
  }

  LayerIterator end() const
  {
    return m_Driver->GetCurrentImageData()->GetLayers(m_RoleFilter).MoveToEnd();
  }

  LayerIterator find(const ValueType &value) const
  {
    return m_Driver->GetCurrentImageData()->GetLayers(m_RoleFilter).Find(
          reinterpret_cast<ImageWrapperBase *>(value));
  }

  ValueType GetValue(const LayerIterator &it) const
  {
    return reinterpret_cast<size_t>(it.GetLayer());
  }

  bool operator == (const AbstractLayerInfoItemSetDomain<TDesc> &cmp) const
    { return m_Driver == cmp.m_Driver && m_RoleFilter == cmp.m_RoleFilter; }

  bool operator != (const AbstractLayerInfoItemSetDomain<TDesc> &cmp) const
    { return m_Driver != cmp.m_Driver || m_RoleFilter != cmp.m_RoleFilter; }

  virtual bool isAtomic() { return false; }

protected:
  IRISApplication *m_Driver;
  int m_RoleFilter;
};

#endif // ABSTRACTLAYERINFOITEMSETDOMAIN_H
