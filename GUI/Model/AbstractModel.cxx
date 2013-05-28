#include "AbstractModel.h"
#include "EventBucket.h"

#include <IRISException.h>
#include <vtkObject.h>
#include <vtkCommand.h>

#include "SNAPEventListenerCallbacks.h"


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
                << " with " << *m_EventBucket << std::endl << std::flush;
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
              << std::endl << std::flush;
    }
#endif // SNAP_DEBUG_EVENTS
  m_Model->m_EventBucket->PutEvent(evt, source);
  m_Model->InvokeEvent(*m_Event);
}

void
AbstractModel::Rebroadcaster
::BroadcastVTK(vtkObject *source, unsigned long event, void *)
{
#ifdef SNAP_DEBUG_EVENTS
  if(flag_snap_debug_events)
    {
    std::cout << "REBROADCAST VTK event "
              <<  vtkCommand::GetStringFromEventId(event)
              << " from " << source->GetClassName()
              << " [" << source << "] "
              << " as " << m_Event->GetEventName()
              << " from " << m_Model->GetNameOfClass()
              << " [" << m_Model << "] "
              << std::endl << std::flush;
    }
#endif // SNAP_DEBUG_EVENTS

  // TODO: how to package this up for the bucket?
  m_Model->m_EventBucket->PutEvent(VTKEvent(), NULL);
  m_Model->InvokeEvent(*m_Event);
}

#include "Rebroadcaster.h"

unsigned long
AbstractModel::Rebroadcast(
    itk::Object *src, const itk::EventObject &srcEvent, const itk::EventObject &trgEvent)
{
  /*
  Rebroadcaster *reb = new Rebroadcaster(this, trgEvent);
  m_Rebroadcast.push_back(reb);
  return AddListenerPair(src, srcEvent, reb, &Rebroadcaster::Broadcast, &Rebroadcaster::Broadcast);
  */
  return ::Rebroadcaster::Rebroadcast(src, srcEvent, this, trgEvent, m_EventBucket);
}

unsigned long
AbstractModel::Rebroadcast(
    vtkObject *src, unsigned long srcEvent, const itk::EventObject &trgEvent)
{
  Rebroadcaster *reb = new Rebroadcaster(this, trgEvent);
  m_Rebroadcast.push_back(reb);
  return AddListenerVTK(src, srcEvent, reb, &Rebroadcaster::BroadcastVTK);
}
