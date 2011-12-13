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

#ifndef SNAPEVENTS_H
#define SNAPEVENTS_H

// Define this if you want event to be debugged
#define SNAP_DEBUG_EVENTS 1

#include "itkObject.h"
#include "itkCommand.h"
#include "itkEventObject.h"

// Common events for the whole application
itkEventMacro(IRISEvent, itk::AnyEvent)

// Events fired by IRISApplication

/** 3D cursor update event */
itkEventMacro(CursorUpdateEvent, IRISEvent)

/** The set of layers loaded into SNAP has changed */
itkEventMacro(LayerChangeEvent, IRISEvent)

/** The size of the main image has changed. */
itkEventMacro(MainImageDimensionsChangeEvent, LayerChangeEvent)

/** The segmentation has changed */
itkEventMacro(SegmentationChangeEvent, LayerChangeEvent)

/** Generic event representing that a model has changed */
itkEventMacro(ModelUpdateEvent, IRISEvent)

/** Change in the linked zoom state */
itkEventMacro(LinkedZoomUpdateEvent, IRISEvent)

/** A change to the UI state (see UIFlags.h)  */
itkEventMacro(StateMachineChangeEvent, IRISEvent)

/** The value of the common zoom has changed */
itkEventMacro(ZoomLevelUpdateEvent, IRISEvent)

/** A generic property change (see Property) */
itkEventMacro(PropertyChangeEvent, IRISEvent)

/** Events used by numeric value models */
itkEventMacro(ValueChangedEvent, IRISEvent)

/** Events used by numeric value models */
itkEventMacro(RangeChangedEvent, IRISEvent)

/** A change to appearance of a renderer, etc */
itkEventMacro(AppearanceUpdateEvent, IRISEvent)

/** A change to the intensity curve */
itkEventMacro(IntensityCurveChangeEvent, IRISEvent)

// A setter method that fires events
#define irisSetWithEventMacro(name,type,event) \
    virtual void Set##name (type _arg) \
{ \
    if(this->m_##name != _arg) \
      { \
      this->m_##name = _arg; \
      this->InvokeEvent(event()); \
      } \
}

// A macro to add observers to specific events to objects. This is here just
// to make the code more readable, because otherwise you never know what class
// fires what events without looking in the code.
#define irisDeclareEventObserver(event) \
    virtual void AddObserver_##event (itk::Command *cmd) \
        { this->AddObserver( event() , cmd ); }

#define FIRES(event) virtual bool Fires##event() const { return true; }


template <class TObserver>
unsigned long AddListener(itk::Object *sender,
                 const itk::EventObject &event,
                 TObserver *observer,
                 void (TObserver::*memberFunction)())
{
  typedef itk::SimpleMemberCommand<TObserver> Cmd;
  typename Cmd::Pointer cmd = Cmd::New();
  cmd->SetCallbackFunction(observer, memberFunction);
  return sender->AddObserver(event, cmd);
}

template <class TObserver>
unsigned long AddListener(itk::Object *sender,
                 const itk::EventObject &event,
                 TObserver *observer,
                 void (TObserver::*memberFunction)(itk::Object*, const itk::EventObject &))
{
  typedef itk::MemberCommand<TObserver> Cmd;
  typename Cmd::Pointer cmd = Cmd::New();
  cmd->SetCallbackFunction(observer, memberFunction);
  return sender->AddObserver(event, cmd);
}

#endif // SNAPEVENTS_H
