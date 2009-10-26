/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: IRISSliceWindow.h,v $
  Language:  C++
  Date:      $Date: 2009/10/26 16:00:56 $
  Version:   $Revision: 1.7 $
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
#ifndef __IRISSliceWindow_h_
#define __IRISSliceWindow_h_

#include "GenericSliceWindow.h"
#include "PolygonDrawing.h"

// Forward references to interaction modes that work with this window
class PolygonInteractionMode;
class RegionInteractionMode;
class PaintbrushInteractionMode;
class AnnotationInteractionMode;

/**
 * \class IRISSliceWindow
 * \brief 2D slice window used in the IRIS part of the application.
 * 
 * These windows allow polygon editing and region of interest selection.
 */
class IRISSliceWindow : public GenericSliceWindow 
{
public:
  
  IRISSliceWindow(int id, UserInterfaceBase *parentUI, FLTKCanvas *canvas);
  virtual ~IRISSliceWindow();

  /** Enter the polygon editing mode of operation */
  void EnterPolygonMode();

  /** Enter the region of interest mode of operation */
  void EnterRegionMode();

  /** Enter the paintbrush mode of operation */
  void EnterPaintbrushMode();

  /** Enter the annotation mode of operation */
  void EnterAnnotationMode();

  /**
   * The initialize method extends the parent's version, sets up some 
   * polygon drawing attributes
   */
  void InitializeSlice(GenericImageData *imageData);

  /**
   * CachedPolygon()
   *
   * purpose:
   * returns the m_CachedPolygon flag of the polygon drawing object
   *
   * post:
   * return value is 1 if polygon drawing has a cached polygon, 0 otherwise
   */
  int  CachedPolygon();

  /**
   * AcceptPolygon()
   *
   * purpose:
   * the gui calls this when the user presses the accept polygon button; this
   * means the polygon that was being edited will be rasterized into the voxel
   * data set according to the current drawing color and painting mode
   *
   * pre:
   * Register() and InitializeSlice() have been called
   * vox data has data with extents matching this window's width and height
   *
   * post:
   * if drawing lock held and polygon_drawing state was EDITING_STATE,
   *   voxels interior to polygon and "writable" are set with the current 
   *   color obtained from m_GlobalState->GetDrawingColor(); the three coverage
   *   modes determine which voxels inside the polygon are written over:
   *   PAINT_OVER_ALL:    all voxels
   *   PAINT_OVER_COLORS: all voxels that aren't labeled clear
   *   PAINT_OVER_ONE:    all voxels of the color obtained from
   *                      m_GlobalState->GetOverWriteColor()
   *   drawing lock released and polygon_drawing state is INACTIVE_STATE
   * else state not changed.
   */
  bool AcceptPolygon();

  /**
   * PastePolygon()
   *
   * purpose:
   * brings the last drawn polygon back for reuse
   *
   * pre:
   * Register() and InitializeSlice() have been called
   *
   * post:
   * if polygon drawing state previously INACTIVE_STATE, drawing lock
   * obtainable, & polygon_drawing has a cached polygon,
   *   cached polygon becomes the edited polygon,
   *   polygon drawing state is EDITING_STATE
   *   drawing lock is held
   * else state does not change
   */
  void PastePolygon();
  
  /** Clear the polygon currently being edited */
  void ClearPolygon();

  /** Delete currently selected polygon points */
  void DeleteSelectedPolygonPoints();

  /** Insert points between selected polygon points */
  void InsertPolygonPoints();

  /** Set the rate at which freehand is converted to polygons */
  void SetFreehandFittingRate(double rate)
    { 
    m_PolygonDrawing->SetFreehandFittingRate(rate);
    }

  /** Get the polygon drawing object */
  irisGetMacro(PolygonDrawing,PolygonDrawing *);

  // Allow friendly access by interactors
  friend class RegionInteractionMode;

protected:

  /** Interaction mode used to select the region of interest */
  RegionInteractionMode *m_RegionMode;

  /** Interaction mode used to position the crosshairs */
  PolygonInteractionMode *m_PolygonMode;

  /** Interaction mode for paintbrush tools */
  PaintbrushInteractionMode *m_PaintbrushMode;

  /** Annotation mode */
  AnnotationInteractionMode *m_AnnotationMode;

  /** polygon drawing object */
  PolygonDrawing *m_PolygonDrawing;

  // Type definition for the slice used for polygon rendering
  typedef itk::Image<unsigned char,2> PolygonSliceType;
  typedef itk::SmartPointer<PolygonSliceType> PolygonSlicePointer;
  
  /** Slice used for polygon drawing and merging */
  PolygonSlicePointer m_PolygonSlice;

  /** Draw the region and polygon interactors */
  void DrawOverlays();
};

#endif


