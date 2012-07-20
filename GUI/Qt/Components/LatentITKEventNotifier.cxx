#include "LatentITKEventNotifier.h"
#include <QApplication>

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
  std::cout << "DELETE CALLBACK from " << object->GetNameOfClass()
            << " [" << typeid(*object).name() << "]"
            << " event " << evt.GetEventName() << std::endl;
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
  std::cout << "QUEUE Event " << evt.GetEventName()
            << " from " << object->GetNameOfClass()
            << " [" << object << "] "
            << " for " << parent()->metaObject()->className()
            << " named '" << qPrintable(parent()->objectName())
            << "'" << std::endl;
#endif

  // Register this event
  m_Bucket.PutEvent(evt);

  // Emit signal
  emit itkEvent();

  // Call parent's update
  // QApplication::postEvent(this, new QEvent(QEvent::User), 1000);
}

void
LatentITKEventNotifierHelper
::onQueuedEvent()
{
  if(!m_Bucket.IsEmpty())
    {
#ifdef SNAP_DEBUG_EVENTS
    std::cout << "SEND " << m_Bucket
              << " to " << parent()->metaObject()->className()
              << " named '" << qPrintable(parent()->objectName())
              << "'" << std::endl;
#endif

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

std::map<QObject *, LatentITKEventNotifierHelper *>
LatentITKEventNotifier::m_HelperMap;


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
  // Try to find the helper object
  LatentITKEventNotifierHelper *c;
  if(m_HelperMap.find(target) == m_HelperMap.end())
    {
    c = new LatentITKEventNotifierHelper(target);
    m_HelperMap[target] = c;
    }
  else
    {
    c = m_HelperMap[target];
    }

  // Connect to the target qobject
  QObject::connect(c, SIGNAL(dispatchEvent(const EventBucket &)), target, slot);

  return c;
}

void LatentITKEventNotifier
::disconnect(itk::Object *source, unsigned long tag)
{
  source->RemoveObserver(tag);
}



