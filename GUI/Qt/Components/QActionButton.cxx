#include "QActionButton.h"
#include "SNAPQtCommon.h"
#include <QAction>
#include "QToolButton"

QActionButton::QActionButton(QWidget *parent) :
  QPushButton(parent)
{
  m_action = NULL;
}

void QActionButton::setAction(QString actionName)
{
  // Remove old connections
  if(m_action && m_action->objectName() != actionName)
    {
    disconnect(m_action, SIGNAL(changed()), this, SLOT(updateFromAction()));
    disconnect(this, SIGNAL(clicked()), m_action, SLOT(trigger()));
    }

  // Assign the action
  m_action = FindUpstreamAction(this, actionName);
  if(m_action)
    {
    // Update ourselves
    updateFromAction();

    // Create connections
    connect(m_action, SIGNAL(changed()), this, SLOT(updateFromAction()));
    connect(this, SIGNAL(clicked()), m_action, SLOT(trigger()));
    }
}

QString QActionButton::action() const
{
  return m_action->objectName();
}

void QActionButton::updateFromAction()
{
  setToolTip(m_action->toolTip());
  setEnabled(m_action->isEnabled());
  setStatusTip(m_action->statusTip());
}


