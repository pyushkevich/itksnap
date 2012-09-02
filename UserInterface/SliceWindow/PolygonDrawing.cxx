/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: PolygonDrawing.cxx,v $
  Language:  C++
  Date:      $Date: 2010/10/13 17:01:08 $
  Version:   $Revision: 1.15 $
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
#include "PolygonDrawing.h"
#include "PolygonScanConvert.h"
#include "SNAPCommonUI.h"
#include "GenericSliceWindow.h"
#include "UserInterfaceBase.h"

#include "SNAPOpenGL.h"
#include <iostream>
#include <cstdlib>
#include <algorithm>
#include <set>
#include <vnl/vnl_random.h>
#include "FL/Fl_Menu_Button.H"

#include "itkImage.h"
#include "itkPointSet.h"

#include "itkBSplineScatteredDataPointSetToImageFilter.h"
#include "SNAPAppearanceSettings.h"

using namespace std;

// glu Tess callbacks

/*
#ifdef WIN32
typedef void (CALLBACK *TessCallback)();
#else
typedef void (*TessCallback)();
#endif

void 
#ifdef WIN32
CALLBACK 
#endif
BeginCallback(GLenum which)
{
  glBegin(which);
}

void 
#ifdef WIN32
CALLBACK 
#endif
EndCallback(void) 
{
  glEnd();
}

void 
#ifdef WIN32
CALLBACK
#endif
ErrorCallback(GLenum errorCode)
{
  const GLubyte *estring;

  estring = gluErrorString(errorCode);
  cerr << "Tesselation Error-Exiting: " << estring << endl;
  exit(-1);
}


void 
#ifdef WIN32
CALLBACK 
#endif
CombineCallback(GLdouble coords[3], 
                GLdouble **irisNotUsed(vertex_data),  
                GLfloat *irisNotUsed(weight), 
                GLdouble **dataOut) 
{
  GLdouble *vertex;

  vertex = new GLdouble[3];
  vertex[0] = coords[0];
  vertex[1] = coords[1];
  vertex[2] = coords[2];
  *dataOut = vertex;
}
*/

const float PolygonDrawing::m_DrawingModeColor[] = { 1.0f, 0.0f, 0.5f };
const float PolygonDrawing::m_EditModeNormalColor[] = { 1.0f, 0.0f, 0.0f };
const float PolygonDrawing::m_EditModeSelectedColor[] = { 0.0f, 1.0f, 0.0f };

/**
 * PolygonDrawing()
 *
 * purpose: 
 * create initial vertex and m_Cache arrays, init GLUm_Tesselatorelator
 */
PolygonDrawing
::PolygonDrawing(GenericSliceWindow *parent)
{
  m_CachedPolygon = false;
  m_State = INACTIVE_STATE;
  m_SelectedVertices = false;
  m_DraggingPickBox = false;
  m_Parent = parent;
}

/**
 * ~PolygonDrawing()
 *
 * purpose: 
 * free arrays and GLUm_Tesselatorelator
 */
PolygonDrawing
::~PolygonDrawing()
{

}

/**
 * ComputeEditBox()
 *
 * purpose: 
 * compute the bounding box around selected vertices
 * 
 * post:
 * if m_Vertices are selected, sets m_SelectedVertices to 1, else 0
 */
void
PolygonDrawing
::ComputeEditBox() 
{
  VertexIterator it;

  // Find the first selected vertex and initialize the selection box
  m_SelectedVertices = false;
  for (it = m_Vertices.begin(); it!=m_Vertices.end();++it) 
    {
    if (it->selected) 
      {
      m_EditBox[0] = m_EditBox[1] = it->x;
      m_EditBox[2] = m_EditBox[3] = it->y;
      m_SelectedVertices = true;
      break;
      }
    }

  // Continue only if a selection exists
  if (!m_SelectedVertices) return;

  // Grow selection box to fit all selected vertices
  for(it = m_Vertices.begin(); it!=m_Vertices.end();++it)
    {
    if (it->selected) 
      {
      if (it->x < m_EditBox[0]) m_EditBox[0] = it->x;
      else if (it->x > m_EditBox[1]) m_EditBox[1] = it->x;

      if (it->y < m_EditBox[2]) m_EditBox[2] = it->y;
      else if (it->y > m_EditBox[3]) m_EditBox[3] = it->y;
      }
    }
}

/**
 * Add()
 *
 * purpose:
 * to add a vertex to the existing contour
 * 
 * pre: 
 * m_NumberOfAllocatedVertices > 0
 */
/*
void
PolygonDrawing
::Add(float x, float y, int selected)
{
  // add a new vertex
  Vertex vNew;
  vNew.x = x; vNew.y = y; vNew.selected = selected;


  m_Vertices[m_NumberOfUsedVertices].x = x;
  m_Vertices[m_NumberOfUsedVertices].y = y;
  m_Vertices[m_NumberOfUsedVertices].selected = selected;
    
  m_NumberOfUsedVertices++;
}
*/

void
PolygonDrawing
::DropLastPoint()
{
  if(m_State == DRAWING_STATE)
    {
    if(m_Vertices.size())
      m_Vertices.pop_back();
    }
}

void
PolygonDrawing
::ClosePolygon()
{
  if(m_State == DRAWING_STATE)
    {
    m_State = EDITING_STATE;
    m_SelectedVertices = true;

    for(VertexIterator it = m_Vertices.begin(); it!=m_Vertices.end(); ++it)
      it->selected = false;

    ComputeEditBox();
    }
}

/**
 * Delete()
 *
 * purpose: 
 * delete all vertices that are selected
 * 
 * post: 
 * if all m_Vertices removed, m_State becomes INACTIVE_STATE
 * length of m_Vertices array does not decrease
 */
void
PolygonDrawing
::Delete() 
{
  VertexIterator it=m_Vertices.begin();
  while(it!=m_Vertices.end())
    {
    if(it->selected)
      it = m_Vertices.erase(it);
    else ++it;
    }
  
  if (m_Vertices.empty()) 
    {
    m_State = INACTIVE_STATE;
    m_SelectedVertices = false;
    }
  
  ComputeEditBox();
}

void 
PolygonDrawing
::Reset()
{
  m_State = INACTIVE_STATE;
  m_Vertices.clear();
  ComputeEditBox();
}

/**
 * Insert()
 *
 * purpose:
 * insert vertices between adjacent selected vertices
 * 
 * post: 
 * length of m_Vertices array does not decrease
 */
void
PolygonDrawing
::Insert() 
{
  // Insert a vertex between every pair of adjacent vertices
  VertexIterator it = m_Vertices.begin();
  while(it != m_Vertices.end())
    {
    // Get the itNext iterator to point to the next point in the list
    VertexIterator itNext = it;
    if(++itNext == m_Vertices.end()) 
      itNext = m_Vertices.begin();

    // Check if the insertion is needed
    if(it->selected && itNext->selected)
      {
      // Insert a new vertex
      Vertex vNew(0.5 * (it->x + itNext->x), 0.5 * (it->y + itNext->y), true, true);
      it = m_Vertices.insert(++it, vNew);
      }

    // On to the next point
    ++it;
    }
}

int 
PolygonDrawing
::GetNumberOfSelectedSegments()
{
  int isel = 0;
  for(VertexIterator it = m_Vertices.begin(); it != m_Vertices.end(); it++)
    {
    // Get the itNext iterator to point to the next point in the list
    VertexIterator itNext = it;
    if(++itNext == m_Vertices.end()) 
      itNext = m_Vertices.begin();

    // Check if the insertion is needed
    if(it->selected && itNext->selected)
      isel++;
    }
  return isel;
}

void
PolygonDrawing
::ProcessFreehandCurve()
{
  // Special case: no fitting  
  if(m_FreehandFittingRate == 0.0)
    {
    for(VertexIterator it = m_DragVertices.begin(); 
      it != m_DragVertices.end(); ++it)
      {
      m_Vertices.push_back(*it);
      }
    m_DragVertices.clear();
    return;
    }

  // We will fit a b-spline of the 0-th order to the freehand curve
  if(m_Vertices.size() > 0)
    {
    // Prepend the last vertex before freehand drawing
    m_DragVertices.push_front(m_Vertices.back());
    m_Vertices.pop_back();
    }

  // Create a list of input points
  typedef itk::Vector<double, 2> VectorType;
  typedef itk::Image<VectorType, 1> ImageType;
  typedef itk::PointSet<VectorType, 1> PointSetType;
  PointSetType::Pointer pointSet = PointSetType::New();

  double len = 0;
  double t = 0, dt = 1.0 / (m_DragVertices.size());
  size_t i = 0;
  Vertex last;
  for(VertexIterator it = m_DragVertices.begin(); 
    it != m_DragVertices.end(); ++it)
    {
    PointSetType::PointType point;
    point[0] = t;
    pointSet->SetPoint(i,point);
    VectorType v;
    v[0] = it->x; v[1] = it->y;
    pointSet->SetPointData(i, v);
    t+=dt; i++;
    if(it != m_DragVertices.begin())
      {
      double dx = last.x - it->x;
      double dy = last.y - it->y;
      len += sqrt(dx * dx + dy * dy);
      }
    last = *it;
    }

  // Compute the number of control points
  size_t nctl = (size_t)ceil(len / m_FreehandFittingRate);
  if(nctl < 3)
    nctl = 3;

  // Compute the number of levels and the control points at coarsest level
  size_t nl = 1; size_t ncl = nctl;
  while(ncl >= 8)
    { ncl >>= 1; nl++; }
  

  // Create the scattered interpolator
  typedef itk::BSplineScatteredDataPointSetToImageFilter<
    PointSetType, ImageType> FilterType;
  FilterType::Pointer filter = FilterType::New();

  ImageType::SpacingType spacing; spacing.Fill( 0.001 );
  ImageType::SizeType size; size.Fill((int)(1.0/spacing[0]));
  ImageType::PointType origin; origin.Fill(0.0);
  
  filter->SetSize( size );
  filter->SetOrigin( origin );
  filter->SetSpacing( spacing );
  filter->SetInput( pointSet );
  filter->SetSplineOrder( 1 );
  FilterType::ArrayType ncps;
  ncps.Fill(ncl);
  filter->SetNumberOfLevels(nl);
  filter->SetNumberOfControlPoints(ncps);
  filter->SetGenerateOutputImage(false);

  // Run the filter
  filter->Update();

  ImageType::Pointer lattice = filter->GetPhiLattice();
  size_t n = lattice->GetBufferedRegion().GetNumberOfPixels();
  for(size_t i = 0; i < n; i++)
    {
    ImageType::IndexType idx;
    idx.Fill(i);
    VectorType v = lattice->GetPixel(idx);
    m_Vertices.push_back(Vertex(v[0],v[1],false,true));
    }

  /*

  // Get the control points?
  double du = 1.0 / nctl;
  for(double u = 0; u < 1.00001; u += du)
    {
    if(u > 1.0) u = 1.0;
    PointSetType::PointType point;
    point[0] = u;
    VectorType v;
    filter->Evaluate(point,v);
    m_Vertices.push_back(Vertex(v[0],v[1],false));
    }
    */

  // Empty the drag list
  // m_DragVertices.clear();
}

bool PolygonVertexTest(const PolygonDrawing::Vertex &v1, const PolygonDrawing::Vertex &v2)
{
  return v1.x == v2.x && v1.y == v2.y;
}

/**
 * AcceptPolygon()
 *
 * purpose:
 * to rasterize the current polygon into a buffer & copy the edited polygon
 * into the polygon m_Cache
 *
 * parameters:
 * buffer - an array of unsigned chars interpreted as an RGBA buffer
 * width  - the width of the buffer
 * height - the height of the buffer 
 *
 * pre: 
 * buffer array has size width*height*4
 * m_State == EDITING_STATE
 *
 * post: 
 * m_State == INACTIVE_STATE
 */
void 
PolygonDrawing
::AcceptPolygon(ByteImageType *image) 
{
  // Remove duplicates from the vertex array
  VertexIterator itEnd = std::unique(m_Vertices.begin(), m_Vertices.end(), PolygonVertexTest);
  m_Vertices.erase(itEnd, m_Vertices.end());

  // There may still be duplicates in the array, in which case we should
  // add a tiny offset to them. Thanks to Jeff Tsao for this bug fix! 
  std::set< std::pair<float, float> > xVertexSet;
  vnl_random rnd;
  for(VertexIterator it = m_Vertices.begin(); it != m_Vertices.end(); ++it)
    {
    while(xVertexSet.find(make_pair(it->x, it->y)) != xVertexSet.end())
      {
      it->x += 0.0001 * rnd.drand32(-1.0, 1.0);
      it->y += 0.0001 * rnd.drand32(-1.0, 1.0);
      }
    xVertexSet.insert(make_pair(it->x, it->y));
    }


  // Scan convert the points into the slice
  typedef PolygonScanConvert<
    unsigned char, GL_UNSIGNED_BYTE, VertexIterator> ScanConvertType;
  
  ScanConvertType::RasterizeFilled(
    m_Vertices.begin(), m_Vertices.size(), image);

  // Copy polygon into polygon m_Cache
  m_CachedPolygon = true;
  m_Cache = m_Vertices;

  // Reset the vertex array for next time
  m_Vertices.clear();
  m_SelectedVertices = false;

  // Set the state
  m_State = INACTIVE_STATE;
}

/**
 * PastePolygon()
 *
 * purpose:
 * copy the m_Cached polygon to the edited polygon
 * 
 * pre: 
 * m_CachedPolygon == 1
 * m_State == INACTIVE_STATE
 * 
 * post: 
 * m_State == EDITING_STATE
 */
void 
PolygonDrawing
::PastePolygon(void)
{
  // Copy the cache into the vertices
  m_Vertices = m_Cache;

  // Select everything
  for(VertexIterator it = m_Vertices.begin(); it!=m_Vertices.end();++it)
    it->selected = false;

  // Set the state
  m_SelectedVertices = false;
  m_State = EDITING_STATE;
  
  // Compute the edit box
  ComputeEditBox();
}

/**
 * Draw()
 *
 * purpose: 
 * draw the polyline being drawn or the polygon being edited
 *
 * parameters:
 * pixel_x - this is a width in the polygon's space that is a single 
 *           pixel width on screen
 * pixel_y - this is a height in the polygon's space that is a single 
 *           pixel height on screen
 *  
 * pre: none - expected to exit if m_State is INACTIVE_STATE
 */
void
PolygonDrawing
::Draw(float pixel_x, float pixel_y)
{
  // Must be in active state
  if (m_State == INACTIVE_STATE) return;

  // Get a pointer to the appearance settings
  SNAPAppearanceSettings *app = m_Parent->GetParentUI()->GetAppearanceSettings();
  SNAPAppearanceSettings::Element &aeDraw = app->GetUIElement(SNAPAppearanceSettings::POLY_DRAW_MAIN);
  SNAPAppearanceSettings::Element &aeClose = app->GetUIElement(SNAPAppearanceSettings::POLY_DRAW_CLOSE);
  SNAPAppearanceSettings::Element &aeEdit = app->GetUIElement(SNAPAppearanceSettings::POLY_EDIT);

  // Push the line state
  glPushAttrib(GL_LINE_BIT | GL_COLOR_BUFFER_BIT);  

  // set line and point drawing parameters
  glPointSize(4);
  // glLineWidth(2);
  // glEnable(GL_LINE_SMOOTH);
  // glHint(GL_LINE_SMOOTH_HINT,GL_NICEST);
  // glEnable(GL_BLEND);
  // glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  // Draw the line segments
  VertexIterator it, itNext;
  if (m_State == EDITING_STATE)
  {
    glPushAttrib(GL_LINE_BIT | GL_COLOR_BUFFER_BIT);  
    SNAPAppearanceSettings::ApplyUIElementLineSettings(aeEdit);

    glBegin(GL_LINES);
    for(it = m_Vertices.begin(); it!=m_Vertices.end();++it)
      {
      // Point to the next vertex
      itNext = it; ++itNext; 
      if(itNext == m_Vertices.end())
        itNext = m_Vertices.begin();

      // Set the color based on the mode
      if (it->selected && itNext->selected) 
        glColor3dv(aeEdit.ActiveColor.data_block());
      else 
        glColor3dv(aeEdit.NormalColor.data_block());
  
      // Draw the line
      glVertex3f(it->x, it->y,0);
      glVertex3f(itNext->x, itNext->y, 0);
    }
    glEnd();

    glPopAttrib();
  }
  // Not editing state
  else 
  {
    glPushAttrib(GL_LINE_BIT | GL_COLOR_BUFFER_BIT);
    SNAPAppearanceSettings::ApplyUIElementLineSettings(aeDraw);

    // Draw the vertices
    glBegin(GL_LINE_STRIP);
    glColor3dv(aeDraw.NormalColor.data_block());
    for(it = m_Vertices.begin(); it!=m_Vertices.end();++it) 
      glVertex3f(it->x, it->y, 0);
    glEnd();

    // Draw the drag vertices
    if(m_DragVertices.size())
      {
      glBegin(GL_LINE_STRIP);
      for(it = m_DragVertices.begin(); it!=m_DragVertices.end();++it) 
        glVertex3f(it->x, it->y, 0);
      glEnd();
      }

    glPopAttrib();

    // Draw stippled line from last point to end point
    if(m_DragVertices.size() + m_Vertices.size() > 2 && aeClose.Visible) 
      {
      glPushAttrib(GL_LINE_BIT | GL_COLOR_BUFFER_BIT);
      SNAPAppearanceSettings::ApplyUIElementLineSettings(aeClose);

      glBegin(GL_LINES);
      glColor3dv(aeClose.NormalColor.data_block());
      if(m_DragVertices.size())
        glVertex3f(m_DragVertices.back().x, m_DragVertices.back().y, 0);
      else
        glVertex3f(m_Vertices.back().x, m_Vertices.back().y, 0);
      glVertex3f(m_Vertices.front().x, m_Vertices.front().y, 0);
      glEnd();
      glPopAttrib();
      }
  }
    
  // draw the vertices
  glBegin(GL_POINTS);
  glPushAttrib(GL_COLOR_BUFFER_BIT);
  for(it = m_Vertices.begin(); it!=m_Vertices.end();++it) 
  {
    if(it->control)
      {
      if (it->selected) 
        glColor3dv(aeEdit.ActiveColor.data_block());
      else if (m_State == DRAWING_STATE)
        glColor3dv(aeDraw.NormalColor.data_block());
      else
        glColor3dv(aeEdit.NormalColor.data_block());

      glVertex3f(it->x,it->y,0.0f);
      }
  }

  // Draw the last dragging vertex point
  if(m_DragVertices.size())
    {
    Vertex last = m_DragVertices.back();
    glColor3dv(aeEdit.ActiveColor.data_block());
    glVertex3f(last.x, last.y, 0.0f);
    }

  glEnd();
  glPopAttrib();

  // draw edit or pick box
  if (m_DraggingPickBox) 
  {
    glPushAttrib(GL_LINE_BIT | GL_COLOR_BUFFER_BIT);
    
    glLineWidth(1);
    glColor3dv(aeEdit.ActiveColor.data_block()); 
    glBegin(GL_LINE_LOOP);
    glVertex3f(m_SelectionBox[0],m_SelectionBox[2],0.0);
    glVertex3f(m_SelectionBox[1],m_SelectionBox[2],0.0);
    glVertex3f(m_SelectionBox[1],m_SelectionBox[3],0.0);
    glVertex3f(m_SelectionBox[0],m_SelectionBox[3],0.0);
    glEnd();    

    glPopAttrib();
  }
  else if (m_SelectedVertices) 
  {
    glPushAttrib(GL_LINE_BIT | GL_COLOR_BUFFER_BIT);
    
    glLineWidth(1);
    glColor3dv(aeEdit.ActiveColor.data_block()); 

    float border_x = (float) 4.0 * pixel_x;
    float border_y = (float) 4.0 * pixel_y;
    glLineWidth(1);
    glColor3fv(m_EditModeSelectedColor);
    glBegin(GL_LINE_LOOP);
    glVertex3f(m_EditBox[0] - border_x,m_EditBox[2] - border_y,0.0);
    glVertex3f(m_EditBox[1] + border_x,m_EditBox[2] - border_y,0.0);
    glVertex3f(m_EditBox[1] + border_x,m_EditBox[3] + border_y,0.0);
    glVertex3f(m_EditBox[0] - border_x,m_EditBox[3] + border_y,0.0);
    glEnd();

    glPopAttrib();
  }

  glPopAttrib();
}

/**
 * Handle()
 *
 * purpose:
 * handle events from the window that contains polygon drawing object:
 * if internal m_State is DRAWING_STATE
 *   left-click creates subsequent vertices of the polygon, 
 *   right-click closes polygon and puts polygon in EDITING_STATE
 * if internal m_State is EDITING_STATE
 *   shift-left-click on vertex adds vertex to selection
 *   shift-left-click off vertex begins dragging a rubber-band box
 *
 *   shift-right-click performs same actions but de-selects vertices
 *
 *   left-click in selection box begins dragging of selected vertices
 *   left-click outside selection box cancels selection, begins a 
 *   dragging of a rubber-band box
 *   
 *   left-release after dragging adds vertices inside rubber-band box
 *   to selection
 * 
 *   pressing the 'insert' key calls Insert()
 *   pressing the 'delete' key calls Delete()
 * 
 * parameters:
 * event   - an Fl event number
 * x       - the x of the point clicked in the space of the polygon
 * y       - the y of the point clicked in the space of the polygon
 * pixel_x - see Draw()
 * pixel_y - see Draw()
 *
 * pre: 
 * window that calls this has the drawing lock
 * 
 * post:
 * if event is used, 1 is returned, else 0
 */
int
PolygonDrawing
::Handle(int event, int button, float x, float y, 
         float pixel_x, float pixel_y)
{
  VertexIterator it, itNext;

  switch (m_State) {
  case INACTIVE_STATE:
    if ((event == FL_PUSH) && (button == FL_LEFT_MOUSE)) 
      {
      m_State = DRAWING_STATE;
      m_Vertices.push_back( Vertex(x, y, false, true) );
      return 1;
      }
    else if ((event == FL_PUSH) && (button == FL_RIGHT_MOUSE))
      {
      // Show popup menu
      Fl_Menu_Button menu(Fl::event_x_root(), Fl::event_y_root(), 80, 1);
      menu.textsize(12);
      menu.add("paste last polygon", FL_COMMAND + 'v', NULL);

      // Disable some options
      if(m_Cache.size() == 0)
        {
        const_cast<Fl_Menu_Item*>(menu.menu() + 0)->deactivate();
        }

      menu.popup();

      // Branch based on the decision
      if(menu.value() == 0)
        {
        m_Parent->GetParentUI()->OnPastePolygonAction(m_Parent->GetId());
        }

      return 1;
      }
    break;

  case DRAWING_STATE:
    if (event == FL_PUSH) 
      {
      m_DragVertices.clear();
      if (button == FL_LEFT_MOUSE)
        {
        // Left click means to add a vertex to the polygon. However, for
        // compatibility reasons, we must make sure that there are no duplicates
        // in the polygon (otherwise, division by zero occurs).
        if(m_Vertices.size() == 0 || 
          m_Vertices.back().x != x || m_Vertices.back().y != y)
          {
          // Check if the user wants to close the polygon
          if(m_Vertices.size() > 2)
            {
            Vector2d A(m_Vertices.front().x / pixel_x, m_Vertices.front().y / pixel_y);
            Vector2d C(x / pixel_x, y / pixel_y);
            if((A-C).inf_norm() < 4)
              {
              ClosePolygon();
              return 1;
              }
            }
          m_Vertices.push_back( Vertex(x, y, false, true) );
          }
        return 1;
        } 
      else if (button == FL_RIGHT_MOUSE) 
        {
        // Show popup menu
        Fl_Menu_Button menu(Fl::event_x_root(), Fl::event_y_root(), 80, 1);
        menu.textsize(12);
        menu.add("close loop && edit",  FL_Enter, NULL);
        menu.add("_close loop && accept", FL_COMMAND + FL_Enter, NULL);
        menu.add("undo last point", FL_Delete, NULL);
        menu.add("cancel drawing", FL_Escape, NULL);

        // Disable some options
        if(!CanClosePolygon())
          {
          const_cast<Fl_Menu_Item*>(menu.menu() + 0)->deactivate();
          const_cast<Fl_Menu_Item*>(menu.menu() + 1)->deactivate();
          }
        if(!CanDropLastPoint())
          {
          const_cast<Fl_Menu_Item*>(menu.menu() + 2)->deactivate();
          }

        menu.popup();

        // Branch based on the decision
        if(menu.value() == 0)
          {
          ClosePolygon();
          }
        if(menu.value() == 1)
          {
          ClosePolygon();
          m_Parent->GetParentUI()->OnAcceptPolygonAction(m_Parent->GetId());
          }
        else if(menu.value() == 2)
          {
          DropLastPoint();
          }
        else if(menu.value() == 3)
          {
          Reset();
          }
        
        return 1;
        }
      }
    else if (event == FL_DRAG)
      {
      if(m_Vertices.size() == 0)
        {
        m_Vertices.push_back(Vertex(x,y,false,true));
        }
      else 
        {
        if(m_FreehandFittingRate == 0)
          {
          m_Vertices.push_back(Vertex(x,y,false,false));
          }
        else
          {
          Vertex &v = m_Vertices.back();
          double dx = (v.x-x) / pixel_x;
          double dy = (v.y-y) / pixel_y;
          double d = dx*dx+dy*dy;
          if(d >= m_FreehandFittingRate * m_FreehandFittingRate)
            m_Vertices.push_back(Vertex(x,y,false,true));
          }
        }

      return 1;
      }
    else if (event == FL_RELEASE)
      {
      // If some dragging has been done, convert it to polygons
      // if(m_DragVertices.size() > 0)
      //  {
      //  ProcessFreehandCurve();
      //  }
      if(m_Vertices.size() && m_Vertices.back().control == false)
        m_Vertices.back().control = true;
      return 1;
      }
    else if (event == FL_SHORTCUT)
      {
      if(Fl::test_shortcut(FL_COMMAND | FL_Enter))
        {
        if(CanClosePolygon())
          {
          ClosePolygon();
          m_Parent->GetParentUI()->OnAcceptPolygonAction(m_Parent->GetId());
          }
        return 1;
        }
      else if(Fl::test_shortcut(FL_Enter))
        {
        if(CanClosePolygon())
          ClosePolygon();
        return 1;
        }
      else if(Fl::test_shortcut(FL_Delete) || Fl::test_shortcut(FL_BackSpace))
        {
        if(CanDropLastPoint())
          DropLastPoint();
        return 1;
        }
      else if(Fl::test_shortcut(FL_Escape))
        {
        Reset();
        return 1;
        }
      }

    break;

  case EDITING_STATE:
    switch (event) {
    case FL_PUSH:
      m_StartX = x;
      m_StartY = y;

      if (button == FL_LEFT_MOUSE) 
        {

        // if user is pressing shift key, add/toggle m_Vertices, or drag pick box
        if (Fl::event_state(FL_SHIFT)) 
          {
          // check if vertex clicked
          if(CheckClickOnVertex(x,y,pixel_x,pixel_y,4))
            {
            ComputeEditBox();
            return 1;
            }

          // check if clicked near a line segment
          if(CheckClickOnLineSegment(x,y,pixel_x,pixel_y,4))
            {
            ComputeEditBox();
            return 1;
            }

          // otherwise start dragging pick box
          m_DraggingPickBox = true;
          m_SelectionBox[0] = m_SelectionBox[1] = x;
          m_SelectionBox[2] = m_SelectionBox[3] = y;
          return 1;
          }

        // user not holding shift key; if user clicked inside edit box, 
        // edit box will be moved in drag event
        if (m_SelectedVertices &&
          (x >= (m_EditBox[0] - 4.0*pixel_x)) && 
          (x <= (m_EditBox[1] + 4.0*pixel_x)) && 
          (y >= (m_EditBox[2] - 4.0*pixel_y)) && 
          (y <= (m_EditBox[3] + 4.0*pixel_y))) return 1;

        // clicked outside of edit box & shift not held, this means the 
        // current selection will be cleared
        for(it = m_Vertices.begin(); it!=m_Vertices.end(); ++it) 
          it->selected = false;
        m_SelectedVertices = false;

        // Check if clicked on a pixel
        if(CheckClickOnVertex(x,y,pixel_x,pixel_y,4))
          {
          ComputeEditBox();
          return 1;
          }

        // check if clicked near a line segment
        if(CheckClickOnLineSegment(x,y,pixel_x,pixel_y,4))
          {
          ComputeEditBox();
          return 1;
          }

        // didn't click a point - start dragging pick box
        m_DraggingPickBox = true;
        m_SelectionBox[0] = m_SelectionBox[1] = x;
        m_SelectionBox[2] = m_SelectionBox[3] = y;
        return 1;
        }

      // Popup menu business
      else if(button == FL_RIGHT_MOUSE)
        {
        // If nothing is selected, select what's under the cursor
        if(!m_SelectedVertices)
          {
          // Check if clicked on a pixel
          if(CheckClickOnVertex(x,y,pixel_x,pixel_y,4))
            {
            ComputeEditBox();
            m_Parent->GetParentUI()->RedrawWindows();
            }

          // check if clicked near a line segment
          else if(CheckClickOnLineSegment(x,y,pixel_x,pixel_y,4))
            {
            ComputeEditBox();
            m_Parent->GetParentUI()->RedrawWindows();
            }
          }

        // Show popup menu
        Fl_Menu_Button menu(Fl::event_x_root(), Fl::event_y_root(), 80, 1);
        menu.textsize(12);
        menu.add("_accept", FL_Enter, NULL);
        menu.add("delete selected points", FL_Delete, NULL);
        menu.add("_split selected segments", FL_Insert, NULL);
        menu.add("clear", FL_Escape, NULL);

        if(!CanInsertVertices())
          const_cast<Fl_Menu_Item*>(menu.menu() + 2)->deactivate();

        if(!m_SelectedVertices)
          const_cast<Fl_Menu_Item*>(menu.menu() + 1)->deactivate();
        
        menu.popup();

        // Branch based on the decision
        if(menu.value() == 0)
          {
          m_Parent->GetParentUI()->OnAcceptPolygonAction(m_Parent->GetId());
          }
        if(menu.value() == 1)
          {
          m_Parent->GetParentUI()->OnDeletePolygonSelectedAction(m_Parent->GetId());
          }
        else if(menu.value() == 2)
          {
          m_Parent->GetParentUI()->OnInsertIntoPolygonSelectedAction(m_Parent->GetId());
          }
        else if(menu.value() == 3)
          {
          Reset();
          }

        return 1;
        }
      break;

    case FL_DRAG:
      if ((button == FL_LEFT_MOUSE) || (button == FL_RIGHT_MOUSE)) 
        {
        if (m_DraggingPickBox) 
          {
          m_SelectionBox[1] = x;
          m_SelectionBox[3] = y;
          } 
        else 
          {
          if (button == FL_LEFT_MOUSE) 
            {
            m_EditBox[0] += (x - m_StartX);
            m_EditBox[1] += (x - m_StartX);
            m_EditBox[2] += (y - m_StartY);
            m_EditBox[3] += (y - m_StartY);

            // If the selection is bounded by control vertices, we simply shift it
            for(it = m_Vertices.begin(); it!=m_Vertices.end(); ++it) 
              {
              if (it->selected) 
                {
                it->x += (x - m_StartX);
                it->y += (y - m_StartY);
                }
              }

            // If the selection is bounded by freehand vertices, we apply a smooth
            m_StartX = x;
            m_StartY = y;
            }
          }
        return 1;
        }
      break;

    case FL_RELEASE:
      if ((button == FL_LEFT_MOUSE) || (button == FL_RIGHT_MOUSE)) 
        {
        if (m_DraggingPickBox) 
          {
          m_DraggingPickBox = false;

          float temp;
          if (m_SelectionBox[0] > m_SelectionBox[1]) 
            {
            temp = m_SelectionBox[0];
            m_SelectionBox[0] = m_SelectionBox[1];
            m_SelectionBox[1] = temp;
            }
          if (m_SelectionBox[2] > m_SelectionBox[3]) 
            {
            temp = m_SelectionBox[2];
            m_SelectionBox[2] = m_SelectionBox[3];
            m_SelectionBox[3] = temp;
            }

          for(it = m_Vertices.begin(); it!=m_Vertices.end(); ++it) 
            {
            if((it->x >= m_SelectionBox[0]) && (it->x <= m_SelectionBox[1]) 
              && (it->y >= m_SelectionBox[2]) && (it->y <= m_SelectionBox[3]))
              it->selected = (button == 1);
            }
          ComputeEditBox();
          }
        return 1;
        }
      break;

    case FL_SHORTCUT:
      if(Fl::test_shortcut(FL_Enter))
        {
        m_Parent->GetParentUI()->OnAcceptPolygonAction(m_Parent->GetId());
        return 1;
        }
      else if(Fl::test_shortcut(FL_Delete) || Fl::test_shortcut(FL_BackSpace))
        {
        if(m_SelectedVertices)
          m_Parent->GetParentUI()->OnDeletePolygonSelectedAction(m_Parent->GetId());
        return 1;
        }
      else if(Fl::test_shortcut(FL_Insert))
        {
        if(CanInsertVertices())
          m_Parent->GetParentUI()->OnInsertIntoPolygonSelectedAction(m_Parent->GetId());
        return 1;
        }
      else if(Fl::test_shortcut(FL_Escape))
        {
        Reset();
        return 1;
        }

      break;

    default: break;
    }
    break;

  default: cerr << "PolygonDrawing::Handle(): unknown m_State " << m_State << endl;
  }

  return 0;
}

/**
 * Check if a click is within k pixels of a vertex, if so select the vertices
 * of that line segment
 */
bool
PolygonDrawing
::CheckClickOnVertex(
  float x, float y, float pixel_x, float pixel_y, int k)
{
  // check if clicked within 4 pixels of a node (use closest node)
  VertexIterator itmin = m_Vertices.end();
  double distmin = k;
  for(VertexIterator it = m_Vertices.begin(); it!=m_Vertices.end(); ++it)  
    {
    Vector2d A(it->x / pixel_x, it->y / pixel_y); 
    Vector2d C(x / pixel_x, y / pixel_y);
    double dist = (A-C).inf_norm();

    if(distmin > dist)
      {
      distmin = dist;
      itmin = it;
      }
    }

  if(itmin != m_Vertices.end())
    {
    itmin->selected = true;
    return true;
    }
  else return false;
}

/**
 * Check if a click is within k pixels of a line segment, if so select the vertices
 * of that line segment
 */
bool
PolygonDrawing
::CheckClickOnLineSegment(
  float x, float y, float pixel_x, float pixel_y, int k)
{
  // check if clicked near a line segment
  VertexIterator itmin1 = m_Vertices.end(), itmin2 = m_Vertices.end();
  double distmin = k;
  for(VertexIterator it = m_Vertices.begin(); it!=m_Vertices.end(); ++it)  
    {
    VertexIterator itnext = it;
    if(++itnext == m_Vertices.end())
      itnext = m_Vertices.begin();

    Vector2d A(it->x / pixel_x, it->y / pixel_y); 
    Vector2d B(itnext->x / pixel_x, itnext->y / pixel_y); 
    Vector2d C(x / pixel_x, y / pixel_y);

    double ab = (A - B).squared_magnitude();
    if(ab > 0)
      {
      double alpha = - dot_product(A-B, B-C) / ab;
      if(alpha > 0 && alpha < 1)
        {
        double dist = (alpha * A + (1-alpha) * B - C).magnitude();
        if(distmin > dist)
          {
          distmin = dist;
          itmin1 = it;
          itmin2 = itnext;
          }
        }
      }
    }

  if(itmin1 != m_Vertices.end())
    {
    itmin1->selected = true;
    itmin2->selected = true;
    return true;
    }
  else return false;
}

/* Can the polygon be closed? */
bool 
PolygonDrawing
::CanClosePolygon()
{
  return m_Vertices.size() > 2;
}

/* Can last point be dropped? */
bool 
PolygonDrawing
::CanDropLastPoint()
{
  return m_Vertices.size() > 0;
}

/* Can edges be split? */
bool 
PolygonDrawing
::CanInsertVertices()
{
  return GetNumberOfSelectedSegments() > 0;
}



/*
 *$Log: PolygonDrawing.cxx,v $
 *Revision 1.15  2010/10/13 17:01:08  pyushkevich
 *Fixing warnings
 *
 *Revision 1.14  2010/10/09 04:20:08  pyushkevich
 *Added customization to polygon drawing appearance;ability to export/import appearance settings
 *
 *Revision 1.13  2010/06/15 16:27:44  pyushkevich
 *build errors fixed
 *
 *Revision 1.12  2010/05/27 11:16:22  pyushkevich
 *Further improved polygon drawing interface
 *
 *Revision 1.11  2010/05/27 07:29:36  pyushkevich
 *New popup menu for polygon drawing, other improvements to polygon tool
 *
 *Revision 1.10  2009/01/23 20:09:38  pyushkevich
 *FIX: 3D rendering now takes place in Nifti(RAS) world coordinates, rather than the VTK (x spacing + origin) coordinates. As part of this, itk::OrientedImage is now used for 3D images in SNAP. Still have to fix cut plane code in Window3D
 *
 *Revision 1.9  2008/10/24 12:52:08  pyushkevich
 *FIX: Bug on ITK 3.8 with level set being inverted
 *FIX: Bug with NIFTI orientation
 *ENH: Clean up itk extras directory
 *
 *Revision 1.8  2008/01/10 18:00:51  pyushkevich
 *took out test.png from Polygon drawing
 *
 *Revision 1.7  2007/12/30 04:43:03  pyushkevich
 *License/Packaging updates
 *
 *Revision 1.6  2007/12/25 15:46:23  pyushkevich
 *Added undo/redo functionality to itk-snap
 *
 *Revision 1.5  2007/10/01 00:13:15  pyushkevich
 *Polygon Drawing updates
 *
 *Revision 1.4  2007/09/18 18:42:40  pyushkevich
 *Added tablet drawing to polygon mode
 *
 *Revision 1.3  2007/09/04 16:56:13  pyushkevich
 *tablet support 1
 *
 *Revision 1.2  2006/12/06 01:26:07  pyushkevich
 *Preparing for 1.4.1. Seems to be stable in Windows but some bugs might be still there
 *
 *Revision 1.1  2006/12/02 04:22:27  pyushkevich
 *Initial sf checkin
 *
 *Revision 1.1.1.1  2006/09/26 23:56:18  pauly2
 *Import
 *
 *Revision 1.10  2005/12/19 03:43:12  pauly
 *ENH: SNAP enhancements and bug fixes for 1.4 release
 *
 *Revision 1.9  2005/12/08 18:20:46  hjohnson
 *COMP:  Removed compiler warnings from SGI/linux/MacOSX compilers.
 *
 *Revision 1.8  2004/07/22 19:22:50  pauly
 *ENH: Large image support for SNAP. This includes being able to use more screen real estate to display a slice, a fix to the bug with manual segmentation of images larger than the window size, and a thumbnail used when zooming into the image.
 *
 *Revision 1.7  2004/01/27 17:49:47  pauly
 *FIX: MAC OSX Compilation fixes
 *
 *Revision 1.6  2003/10/09 22:45:15  pauly
 *EMH: Improvements in 3D functionality and snake parameter preview
 *
 *Revision 1.5  2003/10/02 14:55:53  pauly
 *ENH: Development during the September code freeze
 *
 *Revision 1.1  2003/09/11 13:51:01  pauly
 *FIX: Enabled loading of images with different orientations
 *ENH: Implemented image save and load operations
 *
 *Revision 1.4  2003/08/28 14:37:09  pauly
 *FIX: Clean 'unused parameter' and 'static keyword' warnings in gcc.
 *FIX: Label editor repaired
 *
 *Revision 1.3  2003/08/27 14:03:23  pauly
 *FIX: Made sure that -Wall option in gcc generates 0 warnings.
 *FIX: Removed 'comment within comment' problem in the cvs log.
 *
 *Revision 1.2  2003/08/27 04:57:47  pauly
 *FIX: A large number of bugs has been fixed for 1.4 release
 *
 *Revision 1.1  2003/07/12 04:46:50  pauly
 *Initial checkin of the SNAP application into the InsightApplications tree.
 *
 *Revision 1.1  2003/07/11 23:28:10  pauly
 **** empty log message ***
 *
 *Revision 1.3  2003/06/08 23:27:56  pauly
 *Changed variable names using combination of ctags, egrep, and perl.
 *
 *Revision 1.2  2003/04/29 14:01:42  pauly
 *Charlotte Trip
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
