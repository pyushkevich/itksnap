#ifndef QTREQUESTDELEGATES_H
#define QTREQUESTDELEGATES_H

#include <QWidget>
#include "UIReporterDelegates.h"

class QtViewportReporter : public QWidget, public ViewportSizeReporter
{
  Q_OBJECT

public:

  explicit QtViewportReporter(QWidget *parent);

  bool CanReportSize();

  Vector2ui GetViewportSize();




};

#endif // QTREQUESTDELEGATES_H
