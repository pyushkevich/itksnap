#ifndef SNAPCOMPONENT_H
#define SNAPCOMPONENT_H

#include <QWidget>
#include "UIState.h"

class EventBucket;
class GlobalUIModel;

namespace itk {
class EventObject;
class Object;
}

/**
  \class SNAPComponent
  \brief A base class for all SNAP components that consist of a set of widgets
  layed out in a parent widget. This class has mostly helper functionality to
  reduce the number of includes and shorten the common calls made from SNAP
  widgets
  */
class SNAPComponent : public QWidget
{
  Q_OBJECT
public:
  explicit SNAPComponent(QWidget *parent = 0);



public slots:

  // Default slot for model updates
  virtual void onModelUpdate(const EventBucket &bucket) {}

protected:

  /** Register to receive ITK events from object src. Events will be cached in
    an event bucket and delivered once execution returns to the UI loop */
  void connectITK(itk::Object *src, const itk::EventObject &ev,
                  const char *slot = SLOT(onModelUpdate(const EventBucket &)));

  void disconnectITK(itk::Object *src, unsigned long tag);

};

#endif // SNAPCOMPONENT_H
