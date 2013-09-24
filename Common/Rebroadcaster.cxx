#include "Rebroadcaster.h"
#include "SNAPEventListenerCallbacks.h"
#include "SNAPCommon.h"
#include "EventBucket.h"

Rebroadcaster::DispatchMap Rebroadcaster::m_SourceMap;
Rebroadcaster::DispatchMap Rebroadcaster::m_TargetMap;

unsigned long Rebroadcaster
::Rebroadcast(itk::Object *source, const itk::EventObject &sourceEvent,
              itk::Object *target, const itk::EventObject &targetEvent,
              EventBucket *bucket)
{
  // TODO: for now, we allow the user to call this method twice with the same
  // input without checking if the rebroadcast has already been set up. This
  // might cause some issues if users are not careful.
  Association *assoc = new Association(source, target, targetEvent);

  // Add listeners to the source object, unless the source event is a delete
  // event, in which case, we already are going to register for it in order
  // to delete the associate when the source is deleted.
  assoc->m_IsForDeleteEvent = itk::DeleteEvent().CheckEvent(&sourceEvent);
  if(!assoc->m_IsForDeleteEvent)
    {
    assoc->m_SourceTag = AddListenerPair(
          source, sourceEvent, assoc, &Association::Callback, &Association::ConstCallback);
    }

  // Pass the bucket pointer
  assoc->m_Bucket = bucket;

#ifdef SNAP_DEBUG_EVENTS
  if(flag_snap_debug_events)
    {
    std::cout << "ESTABLISHED REBROADCAST event " <<  sourceEvent.GetEventName()
              << " from " << source->GetNameOfClass()
              << " [" << source << "] "
              << " as " << targetEvent.GetEventName()
              << " from " << target->GetNameOfClass()
              << " [" << target << "] "
              << std::endl << std::flush;
    }
#endif // SNAP_DEBUG_EVENTS



  // This can explode if the target object is deleted and the source object
  // fires an event. One solution would be to require the target object to
  // stop broadcasting before being deleted. However, itk sends us delete
  // callbacks for all objects, so we can use these callbacks to uncouple
  // everything when the source or target are deleted.

  // Is this a known target object?
  if(m_TargetMap.find(target) == m_TargetMap.end())
    {
    // Target object is new. Register to receive delete events from it
    SmartPtr<itk::CStyleCommand> cmd_target_delete = itk::CStyleCommand::New();
    cmd_target_delete->SetCallback(&Rebroadcaster::DeleteTargetCallback);
    cmd_target_delete->SetConstCallback(&Rebroadcaster::DeleteTargetConstCallback);
    target->AddObserver(itk::DeleteEvent(), cmd_target_delete);
    }

  // Add the association to the target's list
  m_TargetMap[target].push_back(assoc);

  // Is this a known source object?
  if(m_SourceMap.find(source) == m_SourceMap.end())
    {
    // Target object is new. Register to receive delete events from it
    SmartPtr<itk::CStyleCommand> cmd_source_delete = itk::CStyleCommand::New();
    cmd_source_delete->SetCallback(&Rebroadcaster::DeleteSourceCallback);
    cmd_source_delete->SetConstCallback(&Rebroadcaster::DeleteSourceConstCallback);
    source->AddObserver(itk::DeleteEvent(), cmd_source_delete);
    }

  // Add the association to the source's list
  m_SourceMap[source].push_back(assoc);

  // Return the tag
  return assoc->m_SourceTag;
}

unsigned long Rebroadcaster
::RebroadcastAsSourceEvent(
    itk::Object *source, const itk::EventObject &sourceEvent,
    itk::Object *target, EventBucket *bucket)
{
  // We just call the main rebroadcast method with RefireEvent() to indicate
  // that the source event should be refired
  return Rebroadcast(source, sourceEvent, target, RefireEvent(), bucket);
}

void Rebroadcaster::DeleteTargetCallback(
    itk::Object *target, const itk::EventObject &evt, void *cd)
{
  DeleteTargetConstCallback(target, evt, cd);
}

void Rebroadcaster::DeleteTargetConstCallback(
    const itk::Object *target, const itk::EventObject &evt, void *cd)
{
  // The target object is being deleted. This means that the corresponding
  // associations should be detatched from their source objects.

  // Find the list of associations for the target object.
  DispatchMap::iterator itmap = m_TargetMap.find(target);
  if(itmap == m_TargetMap.end())
    return;

  // Destroy all the associations
  AssociationList &l = m_TargetMap[target];
  for(AssociationIterator it = l.begin(); it != l.end(); ++it)
    {
    Association *assoc = *it;

    // Remove the observer from the source. If the source is the same object
    // as the target (being deleted), we can skip this
    if(target != assoc->m_Source)
      assoc->m_Source->RemoveObserver(assoc->m_SourceTag);

    // Remove the association from the source's list
    m_SourceMap[assoc->m_Source].remove(assoc);

    // Delete the association
    delete assoc;
    }

  // Remove the target from m_TargetMap
  m_TargetMap.erase(itmap);
}

void Rebroadcaster::DeleteSourceCallback(
    itk::Object *source, const itk::EventObject &evt, void *cd)
{
  DeleteSourceConstCallback(source, evt, cd);
}

void Rebroadcaster::DeleteSourceConstCallback(
    const itk::Object *source, const itk::EventObject &evt, void *cd)
{
  // The source object is being deleted. We need to destroy all of its
  // associations. We don't need to remove any observers though.

  // Find the list of associations for the source object.
  DispatchMap::iterator itmap = m_SourceMap.find(source);
  if(itmap == m_SourceMap.end())
    return;

  // Destroy all the associations
  AssociationList &l = m_SourceMap[source];
  for(AssociationIterator it = l.begin(); it != l.end(); ++it)
    {
    Association *assoc = *it;

    // Remove the association from the target's list
    m_TargetMap[assoc->m_Target].remove(assoc);

    // If the association is for a delete event, then it must be triggered,
    // because these associations are not hooked up to the source objects using
    // AddListener as non-delete associations
    if(assoc->m_IsForDeleteEvent)
      {
      // This call will propagate the event downstream, even though the
      // association itself will be deleted
      assoc->ConstCallback(source, evt);
      }

    // Delete the association
    delete assoc;
    }

  // Remove the source from the source map
  m_SourceMap.erase(itmap);
}


Rebroadcaster::Association::Association(
    itk::Object *source, itk::Object *target, const itk::EventObject &targetEvent)
{
  m_Source = source;
  m_Target = target;
  m_TargetEvent = targetEvent.MakeObject();
  m_Bucket = NULL;
  m_SourceTag = 0;

  m_SourceObjectName = source->GetNameOfClass();
  m_TargetObjectName = target->GetNameOfClass();

  // Are we refiring the source event or firing the target event?
  m_RefireSource = Rebroadcaster::RefireEvent().CheckEvent(m_TargetEvent);


}

Rebroadcaster::Association::~Association()
{
  delete m_TargetEvent;
}

void Rebroadcaster::Association::Callback(itk::Object *source, const itk::EventObject &evt)
{
  this->ConstCallback((const itk::Object *) source, evt);
}

void Rebroadcaster::Association::ConstCallback(const itk::Object *source, const itk::EventObject &evt)
{
  // Decide what to do
  const itk::EventObject *firedEvent = m_RefireSource ? &evt : m_TargetEvent;

#ifdef SNAP_DEBUG_EVENTS
  if(flag_snap_debug_events)
    {
    std::cout << "REBROADCAST event " <<  evt.GetEventName()
              << " from " << m_SourceObjectName
              << " [" << source << "] "
              << " as " << firedEvent->GetEventName()
              << " from " << m_TargetObjectName
              << " [" << m_Target << "] "
              << std::endl << std::flush;
    }
#endif // SNAP_DEBUG_EVENTS

  // Rebroadcast the target event
  m_Target->InvokeEvent(*firedEvent);

  // If there is a bucket, record in it
  if(m_Bucket)
    m_Bucket->PutEvent(evt, source);
}


