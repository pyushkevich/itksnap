#ifndef REBROADCASTER_H
#define REBROADCASTER_H

#include <map>
#include <set>
#include <list>
#include <itkObject.h>
#include <itkEventObject.h>

class EventBucket;

/**
 * @brief The Rebroadcaster class
 * This class allows classes that inherit from itk::Object to rebroadcast
 * events from other itk::Objects as their own events. For example, if I
 * have a class A, which has an instance variable A::worker of class B, and
 * B invokes the event 'FinishedEvent()', I might want A to invoke the event
 * 'WorkerFinishedEvent()'. This is cumbersome to implement in ITK, but using
 * the Rebroadcaster class, it's quite easy:
 *
 * A a1;
 * unsigned long tag = Rebroadcaster::Rebroadcast(
 *                        a1.worker, FinishedEvent(), a1, WorkerFinishedEvent());
 *
 * Later on, if we want to stop rebroadcasting, we can use the tag to stop:
 *
 * Rebroadcaster::StopRebroadcasting(tag);
 *
 * The class handles object destruction gracefully. As soon as one of the classes
 * fires a itk::DeleteEvent(), the rebroadcasting stops.
 *
 * The class also has optional support for event buckets. If, the caller passes
 * in an EventBucket pointer to Rebroadcast(), then any events passed to the
 * target object from the source object will be added to the EventBucket. This
 * way, the target object (or another interested party) can keep track of what
 * events, and from whom, were rebroadcast.
 */
class Rebroadcaster
{
public:

  /**
   * Rebroadcast event sourceEvent from object source as the event
   * targetEvent from object target. Note that if the source event is of
   * a type derived from sourceEvent, then the broadcast will still occur.
   * See docs for the class (above) for notes about the EventBucket param.
   */
  static unsigned long Rebroadcast(
      itk::Object *source, const itk::EventObject &sourceEvent,
      itk::Object *target, const itk::EventObject &targetEvent,
      EventBucket *bucket = NULL);

  /**
   * Rebroadcast event sourceEvent from object source as the same event from
   * object target. An important property of this method is that if we call
   * this method with source event of type baseEvent, and the source object
   * fires event derivedEvent(), which derives from baseEvent(), then the
   * event rebroadcast by the target object will be of type derivedEvent().
   * This is a convenient mechanism for rebroadcasting whole groups of events.
   * For example, if we want every event fired by source to be rebroadcast by
   * target, we can just call this method with itk::AnyEvent() as sourceEvent.
   */
  static unsigned long RebroadcastAsSourceEvent(
      itk::Object *source, const itk::EventObject &sourceEvent,
      itk::Object *target, EventBucket *bucket = NULL);

protected:

  // We define our own event type RefireEvent which is used to indicate that
  // the event fired by the target object should be the same as the event
  // fired by the source object
  itkEventMacro(RefireEvent, itk::EventObject)

  // One of these workers is associated with every rebroadcast request
  class Association
  {
  public:
    Association(itk::Object *source, itk::Object *target, const itk::EventObject &targetEvent);
    ~Association();

    void Callback(itk::Object *source, const itk::EventObject &evt);
    void ConstCallback(const itk::Object *source, const itk::EventObject &evt);

    itk::Object *m_Source, *m_Target;
    itk::EventObject *m_TargetEvent;
    EventBucket *m_Bucket;
    unsigned long m_SourceTag;

    // Whether this association is in response to a delete event.
    bool m_IsForDeleteEvent;

    // For debug purposes, it helps to keep the names of the source and target
    // objects in memory, in case these objects are deleted
    const char *m_SourceObjectName, *m_TargetObjectName;

    bool m_RefireSource;
  };

  static void DeleteSourceCallback(
      itk::Object *source, const itk::EventObject &evt, void *cd);
  static void DeleteSourceConstCallback(
      const itk::Object *source, const itk::EventObject &evt, void *cd);
  static void DeleteTargetCallback(
      itk::Object *target, const itk::EventObject &evt, void *cd);
  static void DeleteTargetConstCallback(
      const itk::Object *target, const itk::EventObject &evt, void *cd);

  // typedef std::pair<itk::Object *, itk::EventObject> ObjectEventPair;
  typedef std::list<Association *> AssociationList;
  typedef AssociationList::iterator AssociationIterator;
  typedef std::map<const itk::Object *, AssociationList> DispatchMap;

  static DispatchMap m_SourceMap, m_TargetMap;
};

#endif // REBROADCASTER_H
