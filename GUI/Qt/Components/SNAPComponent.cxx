#include "SNAPComponent.h"
#include "LatentITKEventNotifier.h"
#include "QtWidgetActivator.h"
#include "GlobalUIModel.h"
#include "SNAPUIFlag.h"

SNAPComponent::SNAPComponent(QWidget *parent) :
    QWidget(parent)
{
}

void
SNAPComponent
::connectITK(itk::Object *src, const itk::EventObject &ev, const char *slot)
{
  LatentITKEventNotifier::connect(src, ev, this, slot);
}

void
SNAPComponent
::disconnectITK(itk::Object *src, unsigned long tag)
{
  LatentITKEventNotifier::disconnect(src, tag);
}

/**
  A QObject used as a convenience to detach observers from ITK objects.
  When destroyed, this object will disconnect itself from the ITK object
  */
/*
class QtObserverTagHolder : public QObject
{
public:
  QtObserverTagHolder(itk::Object *source, unsigned long tag)
    : m_Source(source), m_Tag(tag) {}

  ~QtObserverTagHolder()
  {
    m_Source->RemoveObserver(m_Tag);
  }

private:
  itk::Object *m_Source;
  unsigned long m_Tag;
};
*/
