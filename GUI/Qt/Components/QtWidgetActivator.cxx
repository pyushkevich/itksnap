/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: Filename.cxx,v $
  Language:  C++
  Date:      $Date: 2010/10/18 11:25:44 $
  Version:   $Revision: 1.12 $
  Copyright (c) 2011 Paul A. Yushkevich

  This file is part of ITK-SNAP

  ITK-SNAP is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

=========================================================================*/

#include "QtWidgetActivator.h"
#include "LatentITKEventNotifier.h"
#include <QWidget>
#include <QAction>
#include <StateManagement.h>
#include "GlobalUIModel.h"
#include "SNAPUIFlag.h"

QtWidgetActivator
::QtWidgetActivator(QObject *parent, BooleanCondition *cond, Options options)
  : QObject(parent)
{
  // Register to listen to the state change events
  m_TargetWidget = dynamic_cast<QWidget *>(parent);
  m_TargetAction = dynamic_cast<QAction *>(parent);
  m_Condition = cond;
  m_Options = options;

  // Give it a name
  setObjectName(QString("Activator:%1").arg(parent->objectName()));

  // React to events after control returns to the main UI loop
  LatentITKEventNotifier::connect(
        cond, StateMachineChangeEvent(), this, SLOT(OnStateChange(const EventBucket &)));

  // Update the state of the widget
  EventBucket dummy;
  this->OnStateChange(dummy);
}

QtWidgetActivator::~QtWidgetActivator()
{
}

void QtWidgetActivator::OnStateChange(const EventBucket &)
{
  // Update the state of the widget based on the condition
  bool active = (*m_Condition)();
  if(m_TargetWidget)
    {
    bool status = m_TargetWidget->isEnabledTo(m_TargetWidget->parentWidget());
    if(status != active)
      {
      m_TargetWidget->setEnabled(active);
      if(m_Options & HideInactive)
        m_TargetWidget->setVisible(active);
      }
    }
  else if(m_TargetAction)
    {
    bool status = m_TargetAction->isEnabled();
    if(status != active)
      {
      m_TargetAction->setEnabled(active);
      if(m_Options & HideInactive)
        m_TargetAction->setVisible(active);
      }
    }
}
