#include "QtReporterDelegates.h"
#include "UIReporterDelegates.h"

QtViewportReporter::QtViewportReporter(QWidget *parent)
  : QWidget(parent)
{

}

bool QtViewportReporter::CanReportSize()
{
  return true;
}

Vector2ui QtViewportReporter::GetViewportSize()
{
  return Vector2ui(parentWidget()->width(), parentWidget()->height());
}
