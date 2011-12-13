#include "AbstractModel.h"
#include "EventBucket.h"
#include "Property.h"

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
    std::cout << "UPDATE in " << typeid(*this).name()
              << " BUCKET " << *m_EventBucket << std::endl;
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
#ifdef SNAP_DEBUG_EVENTS
  std::cout << "REBROADCAST " << typeid(*source).name() << ":"
            << evt.GetEventName() << " as "
            << typeid(*m_Model).name() << ":" << m_Event->GetEventName()
            << std::endl;
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
  return AddListener(src, srcEvent, reb, &Rebroadcaster::Broadcast);
}

unsigned long
AbstractModel::Rebroadcast(
    IRISObservable &src, const itk::EventObject &trgEvent)
{
  Rebroadcaster *reb = new Rebroadcaster(this, trgEvent);
  m_Rebroadcast.push_back(reb);
  return AddListener(&src, PropertyChangeEvent(), reb, &Rebroadcaster::Broadcast);
}
