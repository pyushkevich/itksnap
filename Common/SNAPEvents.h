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

#include "itkEventObject.h"
#include <ostream>


/**
  To enable event debugging, ITK-SNAP must be compiled with the
  flag SNAP_DEBUG_EVENTS set. Also, the SNAP executable must be
  launched with the --debug-events option
  */
#define SNAP_DEBUG_EVENTS 1

// This defines the stream for event debugging
#ifdef SNAP_DEBUG_EVENTS
extern bool flag_snap_debug_events;
#endif

// Common events for the whole application
itkEventMacro(IRISEvent, itk::AnyEvent)

// Any event originating in VTK
itkEventMacro(VTKEvent, IRISEvent)

// Events fired by IRISApplication

/** Event that never gets invoked */
itkEventMacro(NullEvent, IRISEvent)

/** 3D cursor update event */
itkEventMacro(CursorUpdateEvent, IRISEvent)

/** The set of layers loaded into SNAP has changed */
itkEventMacro(LayerChangeEvent, IRISEvent)

/** The size of the main image has changed. */
itkEventMacro(MainImageDimensionsChangeEvent, LayerChangeEvent)

/** The pose (orientation, spacing, origin) of the main image has changed */
itkEventMacro(MainImagePoseChangeEvent, LayerChangeEvent)

/** The segmentation has changed */
itkEventMacro(SegmentationChangeEvent, IRISEvent)

/** The level set image has changed (due to iteration) */
itkEventMacro(LevelSetImageChangeEvent, IRISEvent)

/** Change to the speed image */
itkEventMacro(SpeedImageChangedEvent, LayerChangeEvent)

/** Generic event representing that a model has changed */
itkEventMacro(ModelUpdateEvent, IRISEvent)

/** Change in the linked zoom state */
itkEventMacro(LinkedZoomUpdateEvent, IRISEvent)

/** A change to the UI state (see UIFlags.h)  */
itkEventMacro(StateMachineChangeEvent, IRISEvent)

/** The value of the common zoom has changed */
itkEventMacro(ZoomLevelUpdateEvent, IRISEvent)

/** Toolbar mode change */
itkEventMacro(ToolbarModeChangeEvent, IRISEvent)

/** Events used by numeric value models */

// The value of the numeric model changed
itkEventMacro(ValueChangedEvent, IRISEvent)

// The domain of the numeric model changed
itkEventMacro(DomainChangedEvent, IRISEvent)

// A special event where the domain description has changed, rather than
// the whole domain. This never occurs for NumericRange domains, but does
// occur for domains that are lists/sets of items. In that case, this event
// represents the situation where the set of items remains the same, but the
// description of some of the items has changed. The GUI needs to update how
// the items are displayed, but does not need to rebuild the list of items.
itkEventMacro(DomainDescriptionChangedEvent, IRISEvent)

// An event used by property containers that indicates that one of the child
// properties has been modified. For the time being, there does not seem to
// be a need to distinguish between child property value changes and domain
// changes, so there is just a single event.
itkEventMacro(ChildPropertyChangedEvent, IRISEvent)

/** A change to appearance of a renderer, etc */
itkEventMacro(AppearanceUpdateEvent, IRISEvent)

/** Parent event for events fired by the image wrapper */
itkEventMacro(WrapperChangeEvent, IRISEvent)

/** A change to a image wrapper property (e.g., nickname) */
itkEventMacro(WrapperMetadataChangeEvent, WrapperChangeEvent)

/** A change to the visibility of a layer (stickiness, visibility) */
itkEventMacro(WrapperVisibilityChangeEvent, WrapperMetadataChangeEvent)

/** A change to the display mapping of an image wrapper (e.g. color map) */
itkEventMacro(WrapperDisplayMappingChangeEvent, WrapperChangeEvent)

/** A change to wrapper-associated user data */
itkEventMacro(WrapperUserDataChangeEvent, WrapperChangeEvent)

/** A change to wrapper-associated image processing settings */
itkEventMacro(WrapperProcessingSettingsChangeEvent, WrapperUserDataChangeEvent)

/** A change to the intensity curve */
itkEventMacro(IntensityCurveChangeEvent, WrapperDisplayMappingChangeEvent)

/** A change to the color map */
itkEventMacro(ColorMapChangeEvent, WrapperDisplayMappingChangeEvent)

/** Changes to the color label table */
itkEventMacro(SegmentationLabelChangeEvent, IRISEvent)
itkEventMacro(SegmentationLabelConfigurationChangeEvent, SegmentationLabelChangeEvent)
itkEventMacro(SegmentationLabelPropertyChangeEvent, SegmentationLabelChangeEvent)

/** Label under cursor changed */
itkEventMacro(LabelUnderCursorChangedEvent, IRISEvent)

/** Segmentation ROI changed */
itkEventMacro(SegmentationROIChangedEvent, IRISEvent)

/** The mapping between display coordinates and anatomical coordinates changed */
itkEventMacro(DisplayToAnatomyCoordinateMappingChangeEvent, IRISEvent)

// A setter method that fires events
#define irisSetWithEventMacro(name,type,event) \
    virtual void Set##name (type _arg) \
{ \
    if(this->m_##name != _arg) \
      { \
      this->m_##name = _arg; \
      this->Modified(); \
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




#endif // SNAPEVENTS_H
