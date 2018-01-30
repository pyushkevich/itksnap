#include "LatentITKEventNotifier.h"
#include <itkObject.h>
#include <QApplication>
#include <SNAPEventListenerCallbacks.h>

LatentITKEventNotifierCleanup
::LatentITKEventNotifierCleanup(QObject *parent)
  : QObject(parent)
{
  m_Source = NULL;
}

LatentITKEventNotifierCleanup
::~LatentITKEventNotifierCleanup()
{
  if(m_Source)
    {
    m_Source->RemoveObserver(m_Tag);
    m_Source->RemoveObserver(m_DeleteTag);
    }
}

void
LatentITKEventNotifierCleanup
::SetSource(itk::Object *source, unsigned long tag)
{
  // Store the source
  m_Source = source;
  m_Tag = tag;

  // Listen for delete events on the source
  m_DeleteTag = AddListenerConst(
        source, itk::DeleteEvent(),
        this, &LatentITKEventNotifierCleanup::DeleteCallback);
}

void
LatentITKEventNotifierCleanup
::DeleteCallback(const itk::Object *object, const itk::EventObject &evt)
{
#ifdef SNAP_DEBUG_EVENTS
  if(flag_snap_debug_events)
    {
    std::cout << "DELETE CALLBACK from " << object->GetNameOfClass()
              << " [" << typeid(*object).name() << "]"
              << " event " << evt.GetEventName() << std::endl << std::flush;
    }
#endif
  // Forget the source.
  m_Source = NULL;
}




LatentITKEventNotifierHelper
::LatentITKEventNotifierHelper(QObject *parent)
  : QObject(parent)
{
  // Emitting itkEvent will result in onQueuedEvent being called when
  // control returns to the main Qt loop
  QObject::connect(this, SIGNAL(itkEvent()),
                   this, SLOT(onQueuedEvent()),
                   Qt::QueuedConnection);
}

void
LatentITKEventNotifierHelper
::Callback(itk::Object *object, const itk::EventObject &evt)
{
#ifdef SNAP_DEBUG_EVENTS
  if(flag_snap_debug_events)
    {
    std::cout << "QUEUE Event " << evt.GetEventName()
              << " from " << object->GetNameOfClass()
              << " [" << object << "] "
              << " for " << parent()->metaObject()->className()
              << " named '" << qPrintable(parent()->objectName())
              << "'" << std::endl << std::flush;
    }
#endif

  // Register this event
  m_Bucket.PutEvent(evt, object);

  // Emit signal
  emit itkEvent();

  // Call parent's update
  // QApplication::postEvent(this, new QEvent(QEvent::User), 1000);
}

void
LatentITKEventNotifierHelper
::onQueuedEvent()
{
  static int invocation = 0;
  if(!m_Bucket.IsEmpty())
    {
#ifdef SNAP_DEBUG_EVENTS
    std::string class_name = parent()->metaObject()->className();
    std::string object_name = qPrintable(parent()->objectName());
    if(flag_snap_debug_events)
      {
      std::cout << "SEND " << m_Bucket
                << " to " << class_name
                << " named '" << object_name
                << "'" << std::endl << std::flush;
      }
#endif

    ++invocation;

    // Send the event to the target object - immediate
    emit dispatchEvent(m_Bucket);

    // Empty the bucket, so the rest of the events are ignored
    m_Bucket.Clear();
    }
}

/*
bool
LatentITKEventNotifierHelper
::event(QEvent *event)
{
  if(event->type() == QEvent::User)
    {
    if(!m_Bucket.IsEmpty())
      {
      // Send the event to the target object - immediate
      emit dispatchEvent(m_Bucket);

      // Empty the bucket, so the rest of the events are ignored
      m_Bucket.Clear();
      }
    return true;
    }
  else return false;
}
*/

void LatentITKEventNotifier
::connect(itk::Object *source, const itk::EventObject &evt,
          QObject *target, const char *slot)
{
  // Call common implementation
  LatentITKEventNotifierHelper *c = doConnect(evt, target, slot);

  // Listen to events from the source
  unsigned long tag = AddListener(source, evt, c,
                                  &LatentITKEventNotifierHelper::Callback);

  // Create an cleaner as a child of the helper
  LatentITKEventNotifierCleanup *clean = new LatentITKEventNotifierCleanup(c);
  clean->SetSource(source, tag);
}

LatentITKEventNotifierHelper*
LatentITKEventNotifier
::doConnect(const itk::EventObject &evt, QObject *target, const char *slot)
{
  // It is possible that there is already a helper attached to the target
  // object. In that case, we can economize by reusing that helper
  LatentITKEventNotifierHelper *c =
      target->findChild<LatentITKEventNotifierHelper *>();

  if(!c)
    {
    // Here the helper becomes property of QObject target, so that if the
    // target is deleted, the helper will also go away
    c = new LatentITKEventNotifierHelper(target);
    }

  // Connect to the target qobject.
  // VERY IMPORTANT: this uses Qt::UniqueConnection, otherwise the slot will get
  // called every time that an event is hooked up to the slot, making the slot be
  // called sometimes as many as six times per event. This may have been a factor in
  // the laggy GUI performance before
  QObject::connect(c, SIGNAL(dispatchEvent(const EventBucket &)), target, slot, Qt::UniqueConnection);

  return c;
}

void LatentITKEventNotifier
::disconnect(itk::Object *source, unsigned long tag)
{
  source->RemoveObserver(tag);
}



