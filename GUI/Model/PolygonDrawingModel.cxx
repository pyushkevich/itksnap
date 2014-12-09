#include "PolygonDrawingModel.h"
#include "PolygonScanConvert.h"
#include "SNAPOpenGL.h"
#include <iostream>
#include <cstdlib>
#include <algorithm>
#include <set>
#include <vnl/vnl_random.h>

#include "GenericSliceModel.h"
#include "itkImage.h"
#include "itkPointSet.h"
#include "itkBSplineScatteredDataPointSetToImageFilter.h"
#include "IRISApplication.h"

#include <SNAPUIFlag.h>
#include <SNAPUIFlag.txx>

// Enable this model to be used with the flag engine
template class SNAPUIFlag<PolygonDrawingModel, PolygonDrawingUIState>;

using namespace std;


PolygonDrawingModel
::PolygonDrawingModel()
{
  m_CachedPolygon = false;
  m_State = INACTIVE_STATE;
  m_SelectedVertices = false;
  m_DraggingPickBox = false;
  m_StartX = 0; m_StartY = 0;
  m_PolygonSlice = PolygonSliceType::New();
  m_HoverOverFirstVertex = false;

  m_FreehandFittingRateModel = NewRangedConcreteProperty(8.0, 0.0, 100.0, 1.0);

}

PolygonDrawingModel
::~PolygonDrawingModel()
{

}

Vector2f
PolygonDrawingModel::GetPixelSize()
{
  Vector3f x =
    m_Parent->MapWindowToSlice(Vector2f(1.0f)) -
    m_Parent->MapWindowToSlice(Vector2f(0.0f));

  return Vector2f(x[0],x[1]);
}

bool
PolygonDrawingModel
::CheckNearFirstVertex(float x, float y, float pixel_x, float pixel_y)
{
  if(m_Vertices.size() > 2)
    {
    Vector2d A(m_Vertices.front().x / pixel_x,
               m_Vertices.front().y / pixel_y);
    Vector2d C(x / pixel_x, y / pixel_y);
    if((A-C).inf_norm() < 4)
      return true;
    }
  return false;
}

bool
PolygonDrawingModel
::ProcessPushEvent(float x, float y,
                   bool shift_state)
{
  bool handled = false;
  Vector2f pxsize = GetPixelSize();
  float pixel_x = pxsize(0), pixel_y = pxsize(1);

  if(m_State == INACTIVE_STATE)
    {
    SetState(DRAWING_STATE);
    m_Vertices.push_back( Vertex(x, y, false, true) );

    handled = true;
    }

  else if(m_State == DRAWING_STATE)
    {
    // Restart Dragging
    m_DragVertices.clear();

    // The hover state is false
    m_HoverOverFirstVertex = false;

    // Left click means to add a vertex to the polygon. However, for
    // compatibility reasons, we must make sure that there are no duplicates
    // in the polygon (otherwise, division by zero occurs).
    if(m_Vertices.size() == 0 ||
       m_Vertices.back().x != x || m_Vertices.back().y != y)
      {
      // Check if the user wants to close the polygon
      if(CheckNearFirstVertex(x, y, pixel_x, pixel_y))
        ClosePolygon();
      else
        m_Vertices.push_back( Vertex(x, y, false, true) );
      }

    handled = true;
    }

  else if(m_State == EDITING_STATE)
    {
    m_StartX = x;
    m_StartY = y;

    if(!shift_state && m_SelectedVertices &&
       (x >= (m_EditBox[0] - 4.0*pixel_x)) &&
       (x <= (m_EditBox[1] + 4.0*pixel_x)) &&
       (y >= (m_EditBox[2] - 4.0*pixel_y)) &&
       (y <= (m_EditBox[3] + 4.0*pixel_y)))
      {
      // user not holding shift key; if user clicked inside edit box,
      // edit box will be moved in drag event
      }
    else
      {
      if(!shift_state)
        {
        // clicked outside of edit box & shift not held, this means the
        // current selection will be cleared
        for(VertexIterator it = m_Vertices.begin(); it!=m_Vertices.end(); ++it)
          it->selected = false;
        m_SelectedVertices = false;
        }

      // check if vertex clicked
      if(CheckClickOnVertex(x,y,pixel_x,pixel_y,4))
        ComputeEditBox();

      // check if clicked near a line segment
      else if(CheckClickOnLineSegment(x,y,pixel_x,pixel_y,4))
        ComputeEditBox();

      // otherwise start dragging pick box
      else
        {
        m_DraggingPickBox = true;
        m_SelectionBox[0] = m_SelectionBox[1] = x;
        m_SelectionBox[2] = m_SelectionBox[3] = y;
        }
      }

    handled = true;
    }

  if(handled)
    InvokeEvent(StateMachineChangeEvent());

  return handled;
}

bool
PolygonDrawingModel
::ProcessMouseMoveEvent(float x, float y)
{
  if(m_State == DRAWING_STATE)
    {
    // Check if we are hovering over the starting vertex
    Vector2f pxsize = GetPixelSize();
    float pixel_x = pxsize(0), pixel_y = pxsize(1);
    bool hover = CheckNearFirstVertex(x, y, pixel_x, pixel_y);

    if(hover != m_HoverOverFirstVertex)
      {
      m_HoverOverFirstVertex = hover;
      return true;
      }
    }

  return false;
}

bool
PolygonDrawingModel
::ProcessDragEvent(float x, float y)
{
  bool handled = false;
  if(m_State == DRAWING_STATE)
    {
    if(m_Vertices.size() == 0)
      {
      m_Vertices.push_back(Vertex(x,y,false,true));
      }
    else
      {
      // Check/set the hover state
      ProcessMouseMoveEvent(x, y);

      // Check if a point should be added here
      if(this->GetFreehandFittingRate() == 0)
        {
        m_Vertices.push_back(Vertex(x,y,false,false));
        }
      else
        {
        Vector2f pxsize = GetPixelSize();
        Vertex &v = m_Vertices.back();
        double dx = (v.x-x) / pxsize[0];
        double dy = (v.y-y) / pxsize[1];
        double d = dx*dx+dy*dy;
        if(d >= this->GetFreehandFittingRate() * this->GetFreehandFittingRate())
          m_Vertices.push_back(Vertex(x,y,false,true));
        }
      }
    handled = true;
    }

  else if(m_State == EDITING_STATE)
    {
    if (m_DraggingPickBox)
      {
      m_SelectionBox[1] = x;
      m_SelectionBox[3] = y;
      }
    else
      {
      m_EditBox[0] += (x - m_StartX);
      m_EditBox[1] += (x - m_StartX);
      m_EditBox[2] += (y - m_StartY);
      m_EditBox[3] += (y - m_StartY);

      // If the selection is bounded by control vertices, we simply shift it
      for(VertexIterator it = m_Vertices.begin(); it!=m_Vertices.end(); ++it)
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

    handled = true;
    }

  if(handled)
    InvokeEvent(StateMachineChangeEvent());
  return handled;
}

bool
PolygonDrawingModel
::ProcessReleaseEvent(float x, float y)
{
  bool handled = false;
  Vector2f pxsize = GetPixelSize();
  float pixel_x = pxsize(0), pixel_y = pxsize(1);

  if(m_State == DRAWING_STATE)
    {
    // Check if we've closed the loop
    if(CheckNearFirstVertex(x, y, pixel_x, pixel_y))
      {
      ClosePolygon();
      }

    // Make sure the last point is a control point
    if(m_Vertices.size() && m_Vertices.back().control == false)
      m_Vertices.back().control = true;

    handled = true;
    }

  else if(m_State == EDITING_STATE)
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

      for(VertexIterator it = m_Vertices.begin(); it!=m_Vertices.end(); ++it)
        {
        if((it->x >= m_SelectionBox[0]) && (it->x <= m_SelectionBox[1])
           && (it->y >= m_SelectionBox[2]) && (it->y <= m_SelectionBox[3]))
          it->selected = 1;
        }
      ComputeEditBox();
      }
    handled = true;
    }

  if(handled)
    InvokeEvent(StateMachineChangeEvent());
  return handled;

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
PolygonDrawingModel
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

void
PolygonDrawingModel
::DropLastPoint()
{
  if(m_State == DRAWING_STATE)
    {
    if(m_Vertices.size())
      m_Vertices.pop_back();
    InvokeEvent(StateMachineChangeEvent());
    }
}

void
PolygonDrawingModel
::ClosePolygon()
{
  if(m_State == DRAWING_STATE)
    {
    SetState(EDITING_STATE);
    m_SelectedVertices = true;

    for(VertexIterator it = m_Vertices.begin(); it!=m_Vertices.end(); ++it)
      it->selected = false;

    ComputeEditBox();
    InvokeEvent(StateMachineChangeEvent());
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
PolygonDrawingModel
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
    SetState(INACTIVE_STATE);
    m_SelectedVertices = false;
    }

  ComputeEditBox();
  InvokeEvent(StateMachineChangeEvent());
}

void
PolygonDrawingModel
::Reset()
{
  SetState(INACTIVE_STATE);
  m_Vertices.clear();
  ComputeEditBox();
  InvokeEvent(StateMachineChangeEvent());
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
PolygonDrawingModel
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
  InvokeEvent(StateMachineChangeEvent());
}

int
PolygonDrawingModel
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
PolygonDrawingModel
::ProcessFreehandCurve()
{
  // Special case: no fitting
  if(this->GetFreehandFittingRate() == 0.0)
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
  size_t nctl = (size_t)ceil(len / this->GetFreehandFittingRate());
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

bool PolygonVertexTest(const PolygonVertex &v1, const PolygonVertex &v2)
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
PolygonDrawingModel
::AcceptPolygon(std::vector<IRISWarning> &warnings)
{
  assert(m_State == EDITING_STATE);

  // Allocate the polygon to match current image size. This will only
  // allocate new memory if the slice size changed
  itk::Size<2> sz =
    {{ m_Parent->GetSliceSize()[0], m_Parent->GetSliceSize()[1] }};
  m_PolygonSlice->SetRegions(sz);
  m_PolygonSlice->Allocate();

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
    m_Vertices.begin(), m_Vertices.size(), m_PolygonSlice);

  // Apply the segmentation to the main segmentation
  int nUpdates = m_Parent->MergeSliceSegmentation(m_PolygonSlice);
  if(nUpdates == 0)
    {
    warnings.push_back(
          IRISWarning("Warning: No voxels updated."
                      "No voxels in the segmentation image were changed as the "
                      "result of accepting this polygon. Check that the foreground "
                      "and background labels are set correctly."));
    }

  // Copy polygon into polygon m_Cache
  m_CachedPolygon = true;
  m_Cache = m_Vertices;

  // Reset the vertex array for next time
  m_Vertices.clear();
  m_SelectedVertices = false;

  // Set the state
  SetState(INACTIVE_STATE);
  InvokeEvent(StateMachineChangeEvent());
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
PolygonDrawingModel
::PastePolygon(void)
{
  // Copy the cache into the vertices
  m_Vertices = m_Cache;

  // Select everything
  for(VertexIterator it = m_Vertices.begin(); it!=m_Vertices.end();++it)
    it->selected = false;

  // Set the state
  m_SelectedVertices = false;
  SetState(EDITING_STATE);

  // Compute the edit box
  ComputeEditBox();
  InvokeEvent(StateMachineChangeEvent());
}


/**
 * Check if a click is within k pixels of a vertex, if so select the vertices
 * of that line segment
 */
bool
PolygonDrawingModel
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
PolygonDrawingModel
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
PolygonDrawingModel
::CanClosePolygon()
{
  return m_Vertices.size() > 2;
}

/* Can last point be dropped? */
bool
PolygonDrawingModel
::CanDropLastPoint()
{
  return m_Vertices.size() > 0;
}

/* Can edges be split? */
bool
PolygonDrawingModel
::CanInsertVertices()
{
  return GetNumberOfSelectedSegments() > 0;
}

void PolygonDrawingModel::SetState(PolygonDrawingModel::PolygonState state)
{
  if(m_State != state)
    {
    m_State = state;
    InvokeEvent(StateMachineChangeEvent());
    }
}

bool PolygonDrawingModel::CheckState(PolygonDrawingUIState state)
{
  switch(state)
    {
    case UIF_HAVE_VERTEX_SELECTION:
      return this->GetSelectedVertices();
    case UIF_HAVE_EDGE_SELECTION:
      return this->CanInsertVertices();
    case UIF_INACTIVE:
      return m_State == INACTIVE_STATE;
    case UIF_DRAWING:
      return m_State == DRAWING_STATE;
    case UIF_EDITING:
      return m_State == EDITING_STATE;
    case UIF_HAVEPOLYGON:
      return m_Vertices.size() > 0;
    case UIF_HAVECACHED:
      return m_CachedPolygon;
    }

  return false;
}




