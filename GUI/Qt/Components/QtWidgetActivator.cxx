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
#include <QWidget>

QtWidgetActivator
::QtWidgetActivator(QWidget *parent, BooleanCondition *cond)
  : QObject(parent)
{
  // Register to listen to the state change events
  m_Target = parent;
  m_Condition = cond;
  AddListener(cond, StateChangeEvent(),
              this, &QtWidgetActivator::OnStateChange);

  // Update the state of the widget
  this->OnStateChange();
}

QtWidgetActivator::~QtWidgetActivator()
{
  delete m_Condition;
}

void QtWidgetActivator::OnStateChange()
{
  // Update the state of the widget based on the condition
  bool active = (*m_Condition)();
  if(m_Target->isEnabled() != active)
    m_Target->setEnabled(active);
}
