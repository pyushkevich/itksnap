#ifndef UIREPORTERDELEGATES_H
#define UIREPORTERDELEGATES_H

#include "SNAPCommon.h"
#include "SNAPEvents.h"
#include "itkObject.h"

/**
  An interface used by models to request the viewport size from the GUI and
  for the GUI to notify models of changes in the viewport size.
  */
class ViewportSizeReporter : public itk::Object
{
public:
  itkEventMacro(ViewportResizeEvent,IRISEvent)

  /** Child classes are expected to fire this event when viewport resizes */
  FIRES(ViewportResizeEvent)

  virtual bool CanReportSize() = 0;
  virtual Vector2ui GetViewportSize() = 0;

protected:
  virtual ~ViewportSizeReporter() {}
};


#endif // UIREPORTERDELEGATES_H
