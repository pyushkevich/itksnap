#ifndef LATENTITKEVENTNOTIFIER_H
#define LATENTITKEVENTNOTIFIER_H

#include <QObject>
#include "EventBucket.h"
#include <map>

class LatentITKEventNotifierHelper : public QObject
{
  Q_OBJECT

public:
  explicit LatentITKEventNotifierHelper(QObject *parent = 0);

  void Callback(itk::Object *object, const itk::EventObject &evt);
  // bool event(QEvent *event);

public slots:
  void onQueuedEvent();

signals:
  void itkEvent();
  void dispatchEvent(const EventBucket &bucket);

protected:
  EventBucket m_Bucket;
};

/**
  This object is in charge of cleaning up when the parent QObject is deleted.
  This makes sure that the itk::Object being observed does not try to send
  commands to a non-existing observer. It also makes sure that if the source
  itk::Object is deleted, it will not be accessed
  */
class LatentITKEventNotifierCleanup : public QObject
{
  Q_OBJECT

public:
  explicit LatentITKEventNotifierCleanup(QObject *parent = 0);
  ~LatentITKEventNotifierCleanup();

  void SetSource(itk::Object *source, unsigned long tag);

  void DeleteCallback(const itk::Object *object, const itk::EventObject &evt);

protected:
  itk::Object *m_Source;
  unsigned long m_Tag, m_DeleteTag;
};


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
    pooled into a bucket. This bucket will be passed to the slot.

    Connection is handled by a helper object that becomes the child of the
    target qWidget. This helper object will automatically disconnect from
    the target object if the target qWidget is destroyed. The helper object
    will also automatically disappear if the source itk::Object is deleted.
    */
  static void connect(itk::Object *source,
                      const itk::EventObject &evt,
                      QObject *target,
                      const char *slot);

  static void disconnect(itk::Object *source, unsigned long tag);

private:

  static LatentITKEventNotifierHelper *doConnect(
      const itk::EventObject &evt, QObject *target, const char *slot);

};


#endif // LATENTITKEVENTNOTIFIER_H
