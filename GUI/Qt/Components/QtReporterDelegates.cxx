#include "QtReporterDelegates.h"
#include "UIReporterDelegates.h"
#include <QResizeEvent>



QtViewportReporter::QtViewportReporter()
{
  m_ClientWidget = NULL;
  m_Filter = new EventFilter();
  m_Filter->m_Owner = this;
}

QtViewportReporter::~QtViewportReporter()
{
  if(m_ClientWidget)
    m_ClientWidget->removeEventFilter(m_Filter);

  delete m_Filter;
}

void QtViewportReporter::SetClientWidget(QWidget *widget)
{
  // In case we are changing widgets, make sure the filter is cleared
  if(m_ClientWidget)
    m_ClientWidget->removeEventFilter(m_Filter);

  // Store the widget
  m_ClientWidget = widget;

  // Capture events from the widget
  m_ClientWidget->installEventFilter(m_Filter);
}

bool QtViewportReporter::CanReportSize()
{
  return m_ClientWidget != NULL;
}

Vector2ui QtViewportReporter::GetViewportSize()
{
  return Vector2ui(m_ClientWidget->width(), m_ClientWidget->height());
}

bool
QtViewportReporter::EventFilter
::eventFilter(QObject *object, QEvent *event)
{
  if(object == m_Owner->m_ClientWidget && event->type() == QEvent::Resize)
    {
    m_Owner->InvokeEvent(ViewportResizeEvent());
    }
  return QObject::eventFilter(object, event);
}
