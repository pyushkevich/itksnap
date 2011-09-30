#include "LatentITKEventNotifier.h"
#include <QApplication>
#include "IRISObserverPattern.h"

namespace latent_itk_event_notifier
{


Helper::Helper(QObject *parent)
  : QObject(parent)
{
  // Emitting itkEvent will result in onQueuedEvent being called when
  // control returns to the main Qt loop
  /* connect(this, SIGNAL(itkEvent()),
          SLOT(onQueuedEvent()), Qt::QueuedConnection); */
}


void
Helper::Callback(itk::Object *object, const itk::EventObject &evt)
{
#ifdef SNAP_DEBUG_EVENTS
  std::cout << "QUEUE " << typeid(*object).name() << ":"
            << evt.GetEventName() << " for "
            << typeid(*parent()).name() << std::endl;
#endif

  // Register this event
  m_Bucket.PutEvent(evt);

  // Emit signal
  // emit itkEvent();

  // Call parent's update
  QApplication::postEvent(this, new QEvent(QEvent::User), 1000);
}

void Helper::onQueuedEvent()
{
  if(!m_Bucket.IsEmpty())
    {
#ifdef SNAP_DEBUG_EVENTS
    std::cout << "SEND BUCKET " << m_Bucket
              << " to " << typeid(*parent()).name() << std::endl;
#endif

    // Send the event to the target object - immediate
    emit dispatchEvent(m_Bucket);

    // Empty the bucket, so the rest of the events are ignored
    m_Bucket.Clear();
    }
}


bool
Helper::event(QEvent *event)
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

} // namespace

std::map<QObject *, LatentITKEventNotifier::Helper *>
LatentITKEventNotifier::m_HelperMap;


void LatentITKEventNotifier
::connect(itk::Object *source, const itk::EventObject &evt,
          QObject *target, const char *slot)
{
  // Call common implementation
  Helper *c = doConnect(evt, target, slot);

  // Listen to events from the source
  AddListener(source, evt, c, &Helper::Callback);
}

void LatentITKEventNotifier
::connect(IRISObservable *source, const itk::EventObject &evt,
          QObject *target, const char *slot)
{
  // Call common implementation
  Helper *c = doConnect(evt, target, slot);

  // Listen to events from the source
  AddListener(source, evt, c, &Helper::Callback);
}

LatentITKEventNotifier::Helper*
LatentITKEventNotifier
::doConnect(const itk::EventObject &evt, QObject *target, const char *slot)
{
  // Try to find the helper object
  Helper *c;
  if(m_HelperMap.find(target) == m_HelperMap.end())
    {
    c = new Helper(target);
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



