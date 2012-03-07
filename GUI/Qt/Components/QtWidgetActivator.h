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
#include <SNAPCommon.h>
#include <SNAPUIFlag.h>

class BooleanCondition;
class QAction;

class QtWidgetActivator : public QObject
{
  Q_OBJECT
public:
  /**
    Creates an activator with widget parent and boolean condition. The
    condition will be deleted when this object is destroyed. The parent
    widget enabled property will mirror the BoolenaCondition

    The widget can be a QWidget or a QAction. Otherwise, this will do nothing
   */
  explicit QtWidgetActivator(QObject *parent, BooleanCondition *cond);
  ~QtWidgetActivator();

public slots:

  void OnStateChange();

private:
  QWidget *m_TargetWidget;
  QAction *m_TargetAction;
  SmartPtr<BooleanCondition> m_Condition;
};

template<class TModel, class TStateEnum>
void activateOnFlag(QObject *w, TModel *m, TStateEnum flag)
{
  typedef SNAPUIFlag<TModel, TStateEnum> FlagType;
  SmartPtr<FlagType> f = FlagType::New(m, flag);
  new QtWidgetActivator(w, f);
}

template<class TModel, class TStateEnum>
void activateOnNotFlag(QObject *w, TModel *m, TStateEnum flag)
{
  typedef SNAPUIFlag<TModel, TStateEnum> FlagType;
  SmartPtr<FlagType> f = FlagType::New(m, flag);
  SmartPtr<NotCondition> nf = NotCondition::New(f);
  new QtWidgetActivator(w, nf);
}


template<class TModel, class TStateEnum>
void activateOnAllFlags(QObject *w, TModel *m,
                        TStateEnum flag1, TStateEnum flag2)
{
  typedef SNAPUIFlag<TModel, TStateEnum> FlagType;
  SmartPtr<FlagType> f1 = FlagType::New(m, flag1);
  SmartPtr<FlagType> f2 = FlagType::New(m, flag2);
  SmartPtr<AndCondition> f = AndCondition::New(f1, f2);
  new QtWidgetActivator(w, f);
}

template<class TModel, class TStateEnum>
void activateOnAnyFlags(QObject *w, TModel *m,
                        TStateEnum flag1, TStateEnum flag2)
{
  typedef SNAPUIFlag<TModel, TStateEnum> FlagType;
  SmartPtr<FlagType> f1 = FlagType::New(m, flag1);
  SmartPtr<FlagType> f2 = FlagType::New(m, flag2);
  SmartPtr<OrCondition> f = OrCondition::New(f1, f2);
  new QtWidgetActivator(w, f);
}


#endif // QTWIDGETACTIVATOR_H
