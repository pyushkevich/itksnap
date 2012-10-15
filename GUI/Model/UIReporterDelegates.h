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


/**
  This is a progress reporter delegate that allows the SNAP model classes
  to report progress without knowing what GUI toolkit actually implements
  the progress dialog.
  */
class ProgressReporterDelegate
{
public:

  /** Set the progress value between 0 and 1 */
  virtual void SetProgressValue(double) = 0;



};

#endif // UIREPORTERDELEGATES_H
