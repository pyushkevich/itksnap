/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    $RCSfile: PaintbrushInteractionMode.h,v $
  Language:  C++
  Date:      $Date: 2006/12/02 04:22:27 $
  Version:   $Revision: 1.1 $
  Copyright (c) 2003 Insight Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notices for more information.
=========================================================================*/
#ifndef __PaintbrushInteractionMode_h_
#define __PaintbrushInteractionMode_h_

#include "GenericSliceWindow.h"
#include "GlobalState.h"

/**
 * \class PaintbrushInteractionMode
 * \brief UI interaction mode that takes care of painting with a shaped mask (brush).
 *
 * \see GenericSliceWindow
 */
class PaintbrushInteractionMode : public GenericSliceWindow::EventHandler 
{
public:
  PaintbrushInteractionMode(GenericSliceWindow *parent);
  virtual ~PaintbrushInteractionMode();

  int OnMousePress(const FLTKEvent &event);
  int OnKeyDown(const FLTKEvent &event);
  int OnMouseRelease(const FLTKEvent &event, const FLTKEvent &pressEvent);
  int OnMouseDrag(const FLTKEvent &event, const FLTKEvent &pressEvent);
  int OnShortcut(const FLTKEvent &event);
  int OnMouseMove(const FLTKEvent &event);
  void OnDraw();  
private:

  // The paintbrush shape (depends on the settings parameters)
  typedef itk::Image<unsigned char, 2> MaskType;
  MaskType::Pointer m_Mask;

  // The edges in the paintbrush
  typedef std::pair<Vector2d, Vector2d> EdgeType;
  typedef std::list<EdgeType> EdgeList;
  EdgeList m_MaskEdges;

  // Build a display list to represent the paint brush
  void BuildBrush(const PaintbrushSettings &ps);

  // The window handler needs to access our privates
  friend class IRISSliceWindow;
};



#endif // __PaintbrushInteractionMode_h_
