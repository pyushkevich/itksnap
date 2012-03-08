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
#include <SNAPCommon.h>
#include <SNAPEvents.h>

/**
  An abstract condition that returns true or false. Used to create dynamic
  boolean operators for widget activation and state management.

  These conditions can be used in the following form

  SmartPtr<BooleanCondition> p =
    AndCondition::New(
      MyCondition::New(BLAH), NotCondition::New(MyCondition::New(FOO)));

  (*p)() // BLAH && (!FOO)
  */
class BooleanCondition : public itk::Object
{
public:

  FIRES(StateMachineChangeEvent)

  virtual bool operator () () const = 0;

  virtual void OnStateChange();

protected:
  BooleanCondition() {}
  virtual ~BooleanCondition() {}

};

class BinaryBooleanCondition : public BooleanCondition
{
public:

protected:
  BinaryBooleanCondition(BooleanCondition *a, BooleanCondition *b);

  SmartPtr<BooleanCondition> m_A, m_B;
};

class AndCondition : public BinaryBooleanCondition
{  
public:
  typedef AndCondition Self;
  typedef BinaryBooleanCondition Superclass;

  itkTypeMacro(Self, Superclass)

  static SmartPtr<Self> New(BooleanCondition *a, BooleanCondition *b);

  bool operator() () const;

protected:

  AndCondition(BooleanCondition *a, BooleanCondition *b);
  virtual ~AndCondition() {}

};

class OrCondition : public BinaryBooleanCondition
{
public:
  typedef OrCondition Self;
  typedef BinaryBooleanCondition Superclass;

  itkTypeMacro(Self, Superclass)

  static SmartPtr<Self> New(BooleanCondition *a, BooleanCondition *b);

  bool operator() () const;

protected:

  OrCondition(BooleanCondition *a, BooleanCondition *b);
  virtual ~OrCondition() {}

};

class NotCondition : public BooleanCondition
{
public:
  typedef NotCondition Self;
  typedef BooleanCondition Superclass;

  itkTypeMacro(Self, Superclass)

  static SmartPtr<Self> New(BooleanCondition *a);

  bool operator () () const;

protected:

  NotCondition(BooleanCondition *a);

  virtual ~NotCondition() {}


private:
  SmartPtr<BooleanCondition> m_A;

};

#endif // STATEMANAGEMENT_H
