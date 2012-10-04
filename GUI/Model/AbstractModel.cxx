#include "AbstractModel.h"
#include "EventBucket.h"

#include <IRISException.h>

template <class TKey, class TItem>
class StandardMapContainerTraits
{
public:
  typedef TKey KeyType;
  typedef TItem ItemType;
  typedef std::map<TKey, TItem> ContainerType;
  typedef ContainerType * ContainerPointer;
  typedef typename ContainerType::iterator Iterator;

  static Iterator begin(ContainerType *p)
  {
    return p->begin();
  }

  static Iterator end(ContainerType *p)
  {
    return p->end();
  }

  static KeyType key(ContainerType *p, Iterator &it)
  {
    return it->first;
  }

  static TItem &value(ContainerType *p, Iterator &it)
  {
    return it->second;
  }

  static Iterator find(ContainerType *p, KeyType key)
  {
    return p->find(key);
  }
};

template <class TItem>
class StandardVectorContainerTraits
{
public:
  typedef unsigned int KeyType;
  typedef TItem ItemType;
  typedef std::vector<TItem> ContainerType;
  typedef ContainerType * ContainerPointer;
  typedef typename ContainerType::iterator Iterator;

  static Iterator begin(ContainerType *p)
  {
    return p->begin();
  }

  static Iterator end(ContainerType *p)
  {
    return p->end();
  }

  static KeyType key(ContainerType *p, Iterator &it)
  {
    return it - p->begin();
  }

  static TItem &value(ContainerType *p, Iterator &it)
  {
    return *it;
  }

  static Iterator find(ContainerType *p, KeyType key)
  {
    return p->begin() + key;
  }
};

template<class TContainerTraits>
class RandomAccessCollectionNullFilter
{
public:
  typedef typename TContainerTraits::KeyType KeyType;
  typedef typename TContainerTraits::ItemType ItemType;
  bool Check(KeyType &key, ItemType &item) { return true; }
};

/**
  An abstract parent type for models that allow random access to items of
  some type. This abstract class is agnostic to the actual storage type of
  the source container.
  */
template <class TItem>
class AbstractRandomAccessCollectionModel : public AbstractModel
{
public:
  typedef TItem ItemType;
  virtual unsigned int GetSize() = 0;
  virtual TItem &operator[] (unsigned int n) = 0;
};

/**
  This class exposes an indexed collection of items (map/vector) as a random
  access list. This makes it possible to couple this collection with GUI widgets
  such as lists, combo boxes, etc. Internally, the model caches an array of indices
  into the source list. If the source collection changes, the model must be notified
  with an event so that the cache can be rebuilt.
*/
template <class TContainerTraits,
          class TFilter = RandomAccessCollectionNullFilter<TContainerTraits> >
class RandomAccessCollectionModel
    : public AbstractRandomAccessCollectionModel<typename TContainerTraits::ItemType>
{
public:

  // They key/item types
  typedef typename TContainerTraits::KeyType KeyType;
  typedef typename TContainerTraits::ItemType ItemType;

  // The type of the source array
  typedef typename TContainerTraits::ContainerPointer ContainerPointer;

  RandomAccessCollectionModel()
  {
    m_Filter = NULL;
    m_SourceContainer = NULL;
  }

  void SetSourceContainer(ContainerPointer p)
  {
    if(p != m_SourceContainer)
      {
      m_SourceContainer = p;
      m_ArrayDirty = true;
      }
  }

  void SetFilter(TFilter *filter)
  {
    if(filter != m_Filter)
      {
      m_Filter = filter;
      m_ArrayDirty = true;
      }
  }

  // Number of items in the collection
  unsigned int GetSize()
  {
    // Respond to accumulated events
    this->Update();

    // If the cache is dirty, rebuild it
    if(m_ArrayDirty)
      RebuildArray();

    // Return the cached array size
    return m_Array.size();
  }

  // Access to the elements
  ItemType &operator[](unsigned int n)
  {
    // Respond to accumulated events
    this->Update();

    // If the cache is dirty, rebuild it
    if(m_ArrayDirty)
      RebuildArray();

    // Get the item
    typename TContainerTraits::Iterator it =
        TContainerTraits::find(m_SourceContainer, m_Array[n]);
    if(it == TContainerTraits::end(m_SourceContainer))
      throw IRISException("Access violation in RandomAccessCollectionModel");
    return TContainerTraits::value(m_SourceContainer, it);
  }

protected:

  // Rebuild the array (in response to an event(
  void RebuildArray()
  {
    m_Array.clear();
    typename TContainerTraits::Iterator it;
    for(it = TContainerTraits::begin(m_SourceContainer);
        it != TContainerTraits::end(m_SourceContainer);
        ++it)
      {
      // What is the key for this iterator?
      KeyType key = TContainerTraits::key(m_SourceContainer, it);

      // Apply filter!
      if(m_Filter)
        {
        ItemType &item = TContainerTraits::value(m_SourceContainer, it);
        if(m_Filter->Check(key, item))
          m_Array.push_back(key);
        }
      else
        m_Array.push_back(key);
      }

    m_ArrayDirty = false;
  }

  virtual void OnUpdate()
  {
    // If any event has been received, dirty the cache
    m_ArrayDirty = true;
  }

  // Pointer to the source array
  ContainerPointer m_SourceContainer;

  // Filter, if present
  TFilter *m_Filter;

  // Cached array of indices into the source array
  std::vector<KeyType> m_Array;

  // Whether the array is dirty, needs to be rebuilt
  bool m_ArrayDirty;
};

#include <ColorLabel.h>

typedef StandardMapContainerTraits<LabelType, ColorLabel> TTest;
template class RandomAccessCollectionModel<TTest>;






AbstractModel::AbstractModel()
  : itk::Object()
{
  m_EventBucket = new EventBucket();
}

AbstractModel::~AbstractModel()
{
  // Cleanup
  delete m_EventBucket;
  for(std::list<Rebroadcaster *>::iterator it = m_Rebroadcast.begin();
      it != m_Rebroadcast.end(); ++it)
    {
    delete *it;
    }
}

void AbstractModel::Update()
{
  if(!m_EventBucket->IsEmpty())
    {
#ifdef SNAP_DEBUG_EVENTS
    if(flag_snap_debug_events)
      {
      std::cout << "UPDATE called in model " << this->GetNameOfClass()
                << " [" << this << "] "
                << " with " << *m_EventBucket << std::endl;
      }
#endif
    this->OnUpdate();
    m_EventBucket->Clear();
    }
}


AbstractModel::Rebroadcaster
::Rebroadcaster(AbstractModel *model, const itk::EventObject &evt)
{          
  m_Model = model;
  m_Event = evt.MakeObject();
}

AbstractModel::Rebroadcaster
::~Rebroadcaster()
{
  delete m_Event;
}

void
AbstractModel::Rebroadcaster
::Broadcast(itk::Object *source, const itk::EventObject &evt)
{
  this->Broadcast((const itk::Object *) source, evt);
}

void
AbstractModel::Rebroadcaster
::Broadcast(const itk::Object *source, const itk::EventObject &evt)
{
#ifdef SNAP_DEBUG_EVENTS
  if(flag_snap_debug_events)
    {
    std::cout << "REBROADCAST event " <<  evt.GetEventName()
              << " from " << source->GetNameOfClass()
              << " [" << source << "] "
              << " as " << m_Event->GetEventName()
              << " from " << m_Model->GetNameOfClass()
              << " [" << m_Model << "] "
              << std::endl;
    }
#endif // SNAP_DEBUG_EVENTS
  m_Model->m_EventBucket->PutEvent(evt);
  m_Model->InvokeEvent(*m_Event);
}

unsigned long
AbstractModel::Rebroadcast(
    itk::Object *src, const itk::EventObject &srcEvent, const itk::EventObject &trgEvent)
{
  Rebroadcaster *reb = new Rebroadcaster(this, trgEvent);
  m_Rebroadcast.push_back(reb);
  return AddListenerPair(src, srcEvent, reb, &Rebroadcaster::Broadcast, &Rebroadcaster::Broadcast);
}
