#ifndef COLLECTIONMODEL_H
#define COLLECTIONMODEL_H

#include "AbstractModel.h"


/**
 * A model representing a collection of items. Each item has a unique value
 * (like a key in a database) and a descriptor (object describing the item's
 * properties). The model supports iteration. The model can be wrapped around
 * an existing stl container, or it can subclass from an stl container.
 *
 * The model emits events when there are changes in the structure of the
 * collection, as well as when there are changes in the descriptors of the
 * collection.
 *
 * The model is meant to interface with GUI list/combo/table widgets.
 */
template <class TVal, class TDesc, class TIterator>
class AbstractCollectionModel : public AbstractModel
{
public:
  typedef TIterator const_iterator;
  typedef TVal ValueType;
  typedef TDesc DescriptorType;

  FIRES(CollectionStructureChangedEvent)
  FIRES(CollectionDataChangedEvent)

  virtual const_iterator begin() const = 0;
  virtual const_iterator end() const  = 0;
  virtual const_iterator find(const TVal &value) const = 0;
  virtual TVal GetValue(const const_iterator &it) const  = 0;
  virtual TDesc GetDescription(const const_iterator &it) const  = 0;
  virtual void SetDescription(const const_iterator &it, const TDesc &desc) const {}
  virtual ~AbstractItemSetDomain() {}
};


/**
  This is an implementation of the domain that wraps around an stl::map from
  values to descriptors. The map is not stored in the domain, but referenced
  from another object to avoid duplicating data.
  */
template <class TVal, class TDesc>
class STLMapWrapperModel :
    public AbstractCollectionModel<TVal, TDesc,
                                 typename std::map<TVal,TDesc>::const_iterator>
{
public:

  typedef typename std::map<TVal, TDesc> MapType;
  typedef typename MapType::const_iterator const_iterator;
  typedef AbstractCollectionModel<TVal, TDesc, const_iterator> Superclass;
  typedef STLMapWrapperModel<TVal, TDesc> Self;

  itkNewMacro(Self)
  itkTypeMacro(STLMapWrapperModel, AbstractCollectionModel)

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
  STLMapWrapperModel() { m_SourceMap = NULL; }
  STLMapWrapperModel(const MapType *refmap) { m_SourceMap = refmap; }
  virtual ~STLMapWrapperModel() {}

  const MapType *m_SourceMap;
};


/**
  This is an implementation of the domain that wraps around an stl::vector
  of descriptors. TVal should be an integer type that can be used as an index
  (int, unsigned int, enum, etc)
  */
template <class TVal, class TDesc>
class STLVectorWrapperModel :
    public AbstractCollectionModel<TVal, TDesc,
                                 typename std::vector<TDesc>::const_iterator>
{
public:

  typedef STLVectorWrapperModel<TVal, TDesc> Self;
  typedef typename std::vector<TDesc> VectorType;
  typedef typename VectorType::const_iterator const_iterator;
  typedef AbstractCollectionModel<TVal, TDesc, const_iterator> Superclass;

  itkNewMacro(Self)
  itkTypeMacro(STLMapWrapperModel, AbstractCollectionModel)

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
  STLVectorWrapperModel() { m_SourceVector = NULL; }
  STLVectorWrapperModel(const VectorType *refvec) { m_SourceVector = refvec; }
  virtual ~STLVectorWrapperModel() {}

  const VectorType *m_SourceVector;
};

/**
  This is an item domain implementation that is just an stl::map, i.e., it
  owns the data, as opposed to STLMapWrapperItemSetDomain, which references
  the data from another map. This implementation is useful for small domains
  where there is no cost in passing the domain by value.
  */
template<class TVal, class TDesc>
class ConcreteItemCollectionModel : public
    AbstractCollectionModel<TVal, TDesc, typename std::map<TVal,TDesc>::const_iterator>
{
public:
  typedef std::map<TVal, TDesc> MapType;
  typedef typename MapType::const_iterator const_iterator;
  typedef ConcreteItemCollectionModel<TVal, TDesc> Self;
  typedef AbstractCollectionModel<TVal, TDesc, const_iterator> Superclass;

  itkNewMacro(Self)
  itkTypeMacro(ConcreteItemCollectionModel, AbstractCollectionModel)

  const_iterator begin() const
    { return m_Map.begin(); }

  const_iterator end() const
    { return m_Map.end(); }

  const_iterator find(const TVal &value) const
    { return m_Map.find(value); }

  TVal GetValue(const const_iterator &it) const
    { return it->first; }

  TDesc GetDescription(const const_iterator &it) const
    { return it->second; }

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
  SimpleItemSetDomain() : Superclass() { }

  MapType m_Map;
};




#endif // COLLECTIONMODEL_H
