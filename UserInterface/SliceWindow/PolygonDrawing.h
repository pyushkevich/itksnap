/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: PolygonDrawing.h,v $
  Language:  C++
  Date:      $Date: 2010/05/27 11:16:22 $
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
#ifndef __PolygonDrawing_h_
#define __PolygonDrawing_h_

#include <FL/Fl.H>
#include "SNAPOpenGL.h"

#include <iostream>
#include <list>
#include "SNAPCommonUI.h"

class GenericSliceWindow;

namespace itk {
  template <class TPixel, unsigned int VDimensions> class Image;
};


/**
 * \class PolygonDrawing
 * \brief Code for drawing and editing polygons
 */
class PolygonDrawing
{
public:
  // Type of image passed in for rasterization
  typedef itk::Image<unsigned char, 2> ByteImageType;

  /** States that the polygon drawing is in */
  enum PolygonState { INACTIVE_STATE, DRAWING_STATE, EDITING_STATE };

  /** Vertex structure */
  struct Vertex 
    { 
    float x, y; 
    bool selected;
    bool control;
    Vertex(float x_, float y_, bool on_, bool ctl_) 
      : x(x_), y(y_), selected(on_), control(ctl_) {}
    Vertex() : x(0.0f), y(0.0f), selected(false), control(true) {}
    float &operator[](unsigned int i) 
      { return (i==0) ? x : y; }
    };

  PolygonDrawing(GenericSliceWindow *parent);
  virtual ~PolygonDrawing();
  
  void AcceptPolygon(ByteImageType *slice);
  void PastePolygon(void);
  void Draw(float pixel_x, float pixel_y);
  int  Handle(int event, int button, float x, float y, float pixel_x, float pixel_y);

  void Delete();
  void Insert();
  void Reset();

  /* In drawing mode, remove the last point drawn */
  void DropLastPoint();

  /* In drawing mode, close the polygon (same as RMB) */
  void ClosePolygon();

  /* Can the polygon be closed? */
  bool CanClosePolygon();

  /* Can last point be dropped? */
  bool CanDropLastPoint();

  /* Can edges be split? */
  bool CanInsertVertices();

  /** Get the current state of the polygon editor */
  irisGetMacro(State, PolygonState);
  
  /** How many vertices are selected */
  irisGetMacro(SelectedVertices,bool);

  /** How many vertices are selected */
  irisGetMacro(CachedPolygon,bool);

  /** Set the accuracy of freehand curve fitting */
  irisGetMacro(FreehandFittingRate, double);
  irisSetMacro(FreehandFittingRate, double);

private:
  // Array of vertices, cached vertices from last operation
  typedef std::list<Vertex> VertexList;
  typedef VertexList::iterator VertexIterator;
  typedef VertexList::reverse_iterator VertexRIterator;
  VertexList m_Vertices, m_Cache;
  VertexList m_DragVertices;

  // State of the system
  PolygonState m_State;

  bool m_CachedPolygon;
  bool m_SelectedVertices;
  bool m_DraggingPickBox;

  // contains selected points
  float m_EditBox[4];         

  // box the user drags to select new points
  float m_SelectionBox[4];         

  float m_StartX, m_StartY;

  void ComputeEditBox();
  void Add(float x, float y, int selected);
  void ProcessFreehandCurve();

  bool CheckClickOnVertex(float x, float y, float pixel_x, float pixel_y, int k);
  bool CheckClickOnLineSegment(float x, float y, float pixel_x, float pixel_y, int k);
  int GetNumberOfSelectedSegments();

  double m_FreehandFittingRate;

  // Colors used to draw polygon
  const static float 
    m_DrawingModeColor[], m_EditModeSelectedColor[], m_EditModeNormalColor[];

  // Parent object
  GenericSliceWindow *m_Parent;
};

#endif // __PolygonDrawing_h_

/*
 *$Log: PolygonDrawing.h,v $
 *Revision 1.7  2010/05/27 11:16:22  pyushkevich
 *Further improved polygon drawing interface
 *
 *Revision 1.6  2010/05/27 07:29:36  pyushkevich
 *New popup menu for polygon drawing, other improvements to polygon tool
 *
 *Revision 1.5  2007/12/30 04:05:28  pyushkevich
 *GPL License
 *
 *Revision 1.4  2007/12/25 15:46:23  pyushkevich
 *Added undo/redo functionality to itk-snap
 *
 *Revision 1.3  2007/10/01 00:13:15  pyushkevich
 *Polygon Drawing updates
 *
 *Revision 1.2  2007/09/18 18:42:40  pyushkevich
 *Added tablet drawing to polygon mode
 *
 *Revision 1.1  2006/12/02 04:22:27  pyushkevich
 *Initial sf checkin
 *
 *Revision 1.1.1.1  2006/09/26 23:56:18  pauly2
 *Import
 *
 *Revision 1.12  2005/12/19 03:43:12  pauly
 *ENH: SNAP enhancements and bug fixes for 1.4 release
 *
 *Revision 1.11  2005/02/04 17:01:09  lorensen
 *COMP: last of gcc 2.96 changes (I hope).
 *
 *Revision 1.10  2005/02/04 14:17:10  lorensen
 *COMP: gcc 2.96 problems.
 *
 *Revision 1.8  2004/07/22 19:22:51  pauly
 *ENH: Large image support for SNAP. This includes being able to use more screen real estate to display a slice, a fix to the bug with manual segmentation of images larger than the window size, and a thumbnail used when zooming into the image.
 *
 *Revision 1.7  2004/01/27 17:34:00  pauly
 *FIX: Compiling on Mac OSX, issue with GLU include file
 *
 *Revision 1.6  2003/10/09 22:45:15  pauly
 *EMH: Improvements in 3D functionality and snake parameter preview
 *
 *Revision 1.5  2003/10/02 20:57:46  pauly
 *FIX: Made sure that the previous check-in compiles on Linux
 *
 *Revision 1.4  2003/10/02 14:55:53  pauly
 *ENH: Development during the September code freeze
 *
 *Revision 1.1  2003/09/11 13:51:01  pauly
 *FIX: Enabled loading of images with different orientations
 *ENH: Implemented image save and load operations
 *
 *Revision 1.3  2003/08/27 14:03:23  pauly
 *FIX: Made sure that -Wall option in gcc generates 0 warnings.
 *FIX: Removed 'comment within comment' problem in the cvs log.
 *
 *Revision 1.2  2003/08/27 04:57:47  pauly
 *FIX: A large number of bugs has been fixed for 1.4 release
 *
 *Revision 1.1  2003/07/12 04:46:51  pauly
 *Initial checkin of the SNAP application into the InsightApplications tree.
 *
 *Revision 1.4  2003/07/12 01:34:18  pauly
 *More final changes before ITK checkin
 *
 *Revision 1.3  2003/07/11 23:28:10  pauly
 **** empty log message ***
 *
 *Revision 1.2  2003/06/08 23:27:56  pauly
 *Changed variable names using combination of ctags, egrep, and perl.
 *
 *Revision 1.1  2003/03/07 19:29:47  pauly
 *Initial checkin
 *
 *Revision 1.2  2002/12/16 16:40:19  pauly
 **** empty log message ***
 *
 *Revision 1.1.1.1  2002/12/10 01:35:36  pauly
 *Started the project repository
 *
 *
 *Revision 1.2  2002/03/08 14:06:30  moon
 *Added Header and Log tags to all files
 **/
