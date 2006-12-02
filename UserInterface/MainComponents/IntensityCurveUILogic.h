/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    $RCSfile: IntensityCurveUILogic.h,v $
  Language:  C++
  Date:      $Date: 2006/12/02 04:22:22 $
  Version:   $Revision: 1.1 $
  Copyright (c) 2003 Insight Consortium. All rights reserved.
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
