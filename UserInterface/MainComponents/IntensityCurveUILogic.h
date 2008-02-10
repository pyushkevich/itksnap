/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: IntensityCurveUILogic.h,v $
  Language:  C++
  Date:      $Date: 2008/02/10 23:55:22 $
  Version:   $Revision: 1.3 $
  Copyright (c) 2007 Paul A. Yushkevich
  
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

  -----

  Copyright (c) 2003 Insight Software Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notices for more information. 

=========================================================================*/
#ifndef __IntensityCurveUILogic_h_
#define __IntensityCurveUILogic_h_

#include "GreyImageWrapper.h"
#include "IntensityCurveUI.h"
#include "IntensityCurveInterface.h"

#include "itkSmartPointer.h"
#include "itkEventObject.h"
namespace itk{
  class Object;
  class Command;
};

/**
 * \class IntensityCurveUILogic
 * \brief Logic for the UI interaction for intensity curve remapping.
 */
class IntensityCurveUILogic : public IntensityCurveUI {
public:
  // Event storage and propagation
  typedef itk::Object EventSystemType;
  typedef itk::SmartPointer<EventSystemType> EventSystemPointer;

  IntensityCurveUILogic();
  virtual ~IntensityCurveUILogic() {}

  /**
   * Assign a grey image wrapper to the object.  This is necessary in order
   * to display true intensity ranges instead of just the 0 to 1 range
   */
  void SetImageWrapper(GreyImageWrapper *wrapper);

  /**
   * Assign a pointer to an intensity spline to this object (call after MakeWindow)
   */
  void SetCurve(IntensityCurveInterface *curve);

  /**
   * Display the dialog window (call after MakeWindow)
   */
  void DisplayWindow();

  /**
   * Get access to the cuver editor box
   */
  IntensityCurveBox *GetCurveEditorBox() {
    return m_BoxCurve;
  }

  /**
   * Return the event system associated with this object
   */
  irisGetMacro(EventSystem,EventSystemPointer);

  /**
   * Curve update event object
   */
  itkEventMacro(CurveUpdateEvent,itk::AnyEvent);

  // Callbacks made from the user interface
  void OnClose();
  void OnReset();

  /** 
   * This method adjusts the level and the window so that the 
   * window covers the range between 1st and 99th percentiles
   * if image intensity. This is effective at leaving out the
   * hypo/hyper-intensities
   */
  void OnAutoFitWindow();
  void OnWindowLevelChange();
  void OnControlPointNumberChange();
  void OnUpdateHistogram();
  
  // Called when the curve gets changed
  void OnCurveChange();
 
protected:
  
  // Event system for this class -> register events here
  EventSystemPointer m_EventSystem;

  // The intensity curve (same pointer stored in the m_BoxCurve)
  IntensityCurveInterface *m_Curve;
  
  // The associated image wrapper
  GreyImageWrapper *m_ImageWrapper;

  // A friend
  friend class IntensityCurveBox;

  // Updates the window and level values displayed to the user
  void UpdateWindowAndLevel();
};

#endif // __IntensityCurveUILogic_h_
