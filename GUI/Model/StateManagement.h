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

#ifndef STATEMANAGEMENT_H
#define STATEMANAGEMENT_H

#include <itkObject.h>
#include <SNAPEvents.h>

class StateChangeEvent : public IRISEvent {};

/**
  An abstract condition that returns true or false. Used to create dynamic
  boolean operators for widget activation and state management
  */
class BooleanCondition : public itk::Object
{
public:
  virtual bool operator () () const = 0;

  virtual void OnStateChange()
    {
    this->InvokeEvent(StateChangeEvent());
    }
};

class BinaryBooleanCondition : public BooleanCondition
{
public:
  BinaryBooleanCondition(BooleanCondition *a, BooleanCondition *b)
    {
    m_A = a;
    m_B = b;

    // Make sure events propagate up
    AddListener<BinaryBooleanCondition>(a, StateChangeEvent(),
                this, &BinaryBooleanCondition::OnStateChange);

    AddListener<BinaryBooleanCondition>(b, StateChangeEvent(),
                this, &BinaryBooleanCondition::OnStateChange);
    }

  virtual ~BinaryBooleanCondition()
    { delete m_A; delete m_B; }

protected:
   BooleanCondition *m_A, *m_B;
};

class AndCondition : public BinaryBooleanCondition
{
public:
  AndCondition(BooleanCondition *a, BooleanCondition *b)
    : BinaryBooleanCondition(a, b) {}

  bool operator() () const
    { return (*m_A)() && (*m_B)(); }
};

class OrCondition : public BinaryBooleanCondition
{
public:
  OrCondition(BooleanCondition *a, BooleanCondition *b)
    : BinaryBooleanCondition(a, b) {}

  bool operator() () const
    { return (*m_A)() || (*m_B)(); }
};

class NotCondition : public BooleanCondition
{
public:
  NotCondition(BooleanCondition *a)
    {
    m_A = a;
    AddListener<NotCondition>(a, StateChangeEvent(),
                this, &NotCondition::OnStateChange);
    }

  virtual ~NotCondition()
    { delete m_A; }

  bool operator () () const
    { return ! (*m_A)(); }

private:
  BooleanCondition *m_A;

};

#endif // STATEMANAGEMENT_H
