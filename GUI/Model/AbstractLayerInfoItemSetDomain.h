#ifndef ABSTRACTLAYERINFOITEMSETDOMAIN_H
#define ABSTRACTLAYERINFOITEMSETDOMAIN_H

#include <PropertyModel.h>
#include <GenericImageData.h>

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
  AbstractLayerInfoItemSetDomain(GenericImageData *gid = NULL,
                                 int role_filter = 0xffffffff)
    { m_ImageData = gid; m_RoleFilter = role_filter; }

  LayerIterator begin() const
  {
    assert(m_ImageData);
    return m_ImageData->GetLayers(m_RoleFilter);
  }

  LayerIterator end() const
  {
    assert(m_ImageData);
    return m_ImageData->GetLayers(m_RoleFilter).MoveToEnd();
  }

  LayerIterator find(const ValueType &value) const
  {
    assert(m_ImageData);
    return m_ImageData->GetLayers(m_RoleFilter).Find(
          reinterpret_cast<ImageWrapperBase *>(value));
  }

  ValueType GetValue(const LayerIterator &it) const
  {
    assert(m_ImageData);
    return reinterpret_cast<size_t>(it.GetLayer());
  }

  bool operator == (const AbstractLayerInfoItemSetDomain<TDesc> &cmp) const
    { return m_ImageData == cmp.m_ImageData && m_RoleFilter == cmp.m_RoleFilter; }

  bool operator != (const AbstractLayerInfoItemSetDomain<TDesc> &cmp) const
    { return m_ImageData != cmp.m_ImageData || m_RoleFilter != cmp.m_RoleFilter; }

protected:
  GenericImageData *m_ImageData;
  int m_RoleFilter;
};

#endif // ABSTRACTLAYERINFOITEMSETDOMAIN_H
