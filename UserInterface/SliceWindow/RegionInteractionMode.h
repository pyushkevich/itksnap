/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    $RCSfile: RegionInteractionMode.h,v $
  Language:  C++
  Date:      $Date: 2006/12/02 04:22:27 $
  Version:   $Revision: 1.1 $
  Copyright (c) 2003 Insight Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notices for more information.
=========================================================================*/
#ifndef __RegionInteractionMode_h_
#define __RegionInteractionMode_h_

#include "GenericSliceWindow.h"

/**
 * \class RegionInteractionMode
 * \brief UI interaction mode that takes care of ROI positioning.
 *
 * \see GenericSliceWindow
 */
class RegionInteractionMode : public GenericSliceWindow::EventHandler {
public:
  RegionInteractionMode(GenericSliceWindow *parent);
  int OnMousePress(const FLTKEvent &event);
  int OnMouseRelease(const FLTKEvent &event, const FLTKEvent &pressEvent);    
  int OnMouseDrag(const FLTKEvent &event, const FLTKEvent &pressEvent);
  void OnDraw();

private:  

  // The click detection radius (delta)
  static const unsigned int m_PixelDelta;

  // Four vertices in the region box (correspond to the two corners 
  // of the 3D region of interest
  Vector3f m_CornerDragStart[2];

  /**
   * The four edges in the rectangle, ordered first by orientation
   * (0 = horizontal), (1 = vertical) and then by adjacency to the 
   * first or second vertex of the cardinal cube defining the region
   * of interest (0 = closest to 0,0,0), (1 = closest to sx,sy,sz)
   */
  bool m_EdgeHighlighted[2][2];

  /**
   * Is any one of the edges highlighted?
   */
  bool m_IsAnyEdgeHighlighted;

  /** Map from system's ROI in image coordinates to 2D slice coords */
  void GetSystemROICorners(Vector3f corner[2]);

  /** Compute the slice-space vertices corresponding to an edge */
  void GetEdgeVertices(unsigned int direction,
    unsigned int index,Vector2f &x0,Vector2f &x1,const Vector3f corner[2]);

  /** Compute a distance to an edge */
  float GetEdgeDistance(unsigned int direction,
    unsigned int index,const Vector2f &point,const Vector3f corner[2]);

  /**
   * Update the region of interest in response to the dragging or release
   * operations.
   */
  void UpdateCorners(const FLTKEvent &event,const FLTKEvent &pressEvent);
};

#endif // __RegionInteractionMode_h_
