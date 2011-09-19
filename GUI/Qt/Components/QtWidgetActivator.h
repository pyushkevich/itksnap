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

#ifndef QTWIDGETACTIVATOR_H
#define QTWIDGETACTIVATOR_H

#include <QObject>
#include <StateManagement.h>

class QtWidgetActivator : public QObject
{
  Q_OBJECT
public:
  /**
    Creates an activator with widget parent and boolean condition. The
    condition will be deleted when this object is destroyed. The parent
    widget enabled property will mirror the BoolenaCondition
   */
  explicit QtWidgetActivator(QWidget *parent, BooleanCondition *cond);
  ~QtWidgetActivator();

  void OnStateChange();

private:
  QWidget *m_Target;
  BooleanCondition *m_Condition;
};

#endif // QTWIDGETACTIVATOR_H
