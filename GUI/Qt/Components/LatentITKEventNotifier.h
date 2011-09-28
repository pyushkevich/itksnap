#ifndef LATENTITKEVENTNOTIFIER_H
#define LATENTITKEVENTNOTIFIER_H

#include <QObject>
#include "EventBucket.h"
#include <map>

namespace latent_itk_event_notifier
{

class Helper : public QObject
{
  Q_OBJECT

public:
  explicit Helper(QObject *parent = 0);

  void Callback(itk::Object *object, const itk::EventObject &evt);
  bool event(QEvent *event);

public slots:
  void onQueuedEvent();

signals:
  void itkEvent();
  void dispatchEvent(const EventBucket &bucket);

protected:
  EventBucket m_Bucket;
};

} // namespace


/**
  This class is used to hook up Qt widgets to the itk event system used
  by the upstream objects.
  */
class LatentITKEventNotifier
{
public:

  /**
    Map itk events originating from object source to a slot in the object
    target. The signature of slot is void mySlot(const EventBucket &b).
    The slot will be called after the control has returned to the Qt
    main loop, and until then, events fired by the source object are
    pooled into a bucket. This bucket will be passed to the slot
    */
  static void connect(itk::Object *source, const itk::EventObject &evt,
                      QObject *target, const char *slot);

private:

  typedef latent_itk_event_notifier::Helper Helper;
  static std::map<QObject *, Helper *> m_HelperMap;

};


#endif // LATENTITKEVENTNOTIFIER_H
