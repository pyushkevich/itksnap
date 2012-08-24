#ifndef UIREPORTERDELEGATES_H
#define UIREPORTERDELEGATES_H

#include "SNAPCommon.h"

/**
  An interface used by models to request the viewport size from the GUI.
  */
class ViewportSizeReporter
{
public:
  virtual bool CanReportSize() = 0;
  virtual Vector2ui GetViewportSize() = 0;
  virtual ~ViewportSizeReporter() {};
};


#endif // UIREPORTERDELEGATES_H
