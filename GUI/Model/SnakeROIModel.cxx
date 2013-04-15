#include "SnakeROIModel.h"
#include "GenericSliceModel.h"
#include "IRISApplication.h"
#include "GenericImageData.h"
#include "GlobalState.h"

SnakeROIModel::SnakeROIModel()
{
}


void SnakeROIModel::SetParent(GenericSliceModel *parent)
{
  // Set up the parent
  m_Parent = parent;

  // Listen to changes in the parent's segmentation ROI settings
  Rebroadcast(
        m_Parent->GetDriver()->GetGlobalState()->GetSegmentationROISettingsModel(),
        ValueChangedEvent(), ModelUpdateEvent());

  // Layer change events too?
  Rebroadcast(m_Parent->GetDriver(), LayerChangeEvent(), ModelUpdateEvent());
}

// Return true if the selection state has changed
SnakeROISideSelectionState
SnakeROIModel
::ComputeSelection(Vector2f &uvSlice, Vector3f corners[])
{
  // This code computes the current selection based on the mouse coordinates
  // Flag indicating whether we respond to this event or not
  SnakeROISideSelectionState h;

  // Repeat for vertical and horizontal edges
  for(unsigned int dir = 0; dir < 2; dir++)
    {
    // Variables used to find the closest edge that's within delta
    int iClosest = -1;
    float dToClosest = m_PixelDelta;

    // Search for the closest edge
    for(unsigned int i=0;i<2;i++)
      {
      float d = GetEdgeDistance(dir,i,uvSlice,corners);
      if(d < dToClosest)
        {
        dToClosest = d;
        iClosest = i;
        }
      }

    // Highlight the selected edge
    if(iClosest >= 0)
      {
      h.Highlighted[dir][iClosest] = true;
      }
    }

  // Also test for an inside click (all selected)
  if(!h.AnyHighlighted())
    {
    // Are we inside?
    if(uvSlice[0] > std::min(corners[0][0], corners[1][0]) &&
       uvSlice[0] < std::max(corners[0][0], corners[1][0]) &&
       uvSlice[1] > std::min(corners[0][1], corners[1][1]) &&
       uvSlice[1] < std::max(corners[0][1], corners[1][1]))
      {
      h.Highlighted[0][0]=true;
      h.Highlighted[0][1]=true;
      h.Highlighted[1][0]=true;
      h.Highlighted[1][1]=true;
      }
    }

  return h;
}

bool SnakeROIModel::ProcessPushEvent(float x, float y)
{
  // Convert the event location into slice u,v coordinates
  Vector2f uvSlice(x, y);

  // Record the system's corners at the time of drag start
  GetSystemROICorners(m_CornerDragStart);

  // Compute the selection
  SnakeROISideSelectionState hl = ComputeSelection(uvSlice, m_CornerDragStart);
  if(hl != m_Highlight)
    {
    m_Highlight = hl;
    InvokeEvent(ModelUpdateEvent());
    }

  // If nothing was highlighted, then return and let the next handler process
  // the event
  return m_Highlight.AnyHighlighted();
}

bool SnakeROIModel
::ProcessMoveEvent(float x, float y)
{
  // Convert the event location into slice u,v coordinates
  Vector2f uvSlice(x, y);

  // Record the system's corners at the time of drag start
  GetSystemROICorners(m_CornerDragStart);

  // Compute the selection
  SnakeROISideSelectionState hl = ComputeSelection(uvSlice, m_CornerDragStart);
  if(hl != m_Highlight)
    {
    m_Highlight = hl;
    InvokeEvent(ModelUpdateEvent());
    }

  return true;
}

bool SnakeROIModel::ProcessDragEvent(
    float x, float y, float xStart, float yStart, bool release)
{
  // Only do something if there is a highlight
  if(m_Highlight.AnyHighlighted())
    {
    // Update the corners in response to the dragging
    UpdateCorners(Vector2f(x, y), Vector2f(xStart,yStart));

    // Event has been handled
    return true;
    }

  // Event has not been handled
  return false;
}

// The click detection radius (delta)
const unsigned int SnakeROIModel::m_PixelDelta = 8;


void
SnakeROIModel
::GetEdgeVertices(unsigned int direction,unsigned int index,
                  Vector2f &x0,Vector2f &x1,
                  const Vector3f corner[2])
{
  x0(direction) = corner[0](direction);
  x1(direction) = corner[1](direction);
  x0(1-direction) = x1(1-direction) = corner[index](1-direction);
}

float
SnakeROIModel
::GetEdgeDistance(unsigned int direction,
                  unsigned int index,
                  const Vector2f &x,
                  const Vector3f corner[2])
{
  // Compute the vertices of the edge
  Vector2f x0,x1;
  GetEdgeVertices(direction,index,x0,x1,corner);

  // Compute the squared distance between the vertices
  float l2 = (x1-x0).squared_magnitude();
  float l = sqrt(l2);

  // Compute the projection of x onto x1-x0
  float p = dot_product(x-x0,x1-x0) / sqrt(l2);
  float p2 = p*p;

  // Compute the squared distance to the line of the edge
  float q2 = (x-x0).squared_magnitude() - p2;

  // Compute the total distance
  float d = sqrt(q2 + (p < 0 ? p2 : 0) + (p > l ? (p-l)*(p-l) : 0));

  // Return this distance
  return d;
}

void SnakeROIModel
::GetSystemROICorners(Vector3f corner[2])
{
  // Get the region of interest in image coordinates
  GlobalState *gs = m_Parent->GetDriver()->GetGlobalState();
  GlobalState::RegionType roi = gs->GetSegmentationROI();

  // Get the lower-valued corner
  Vector3l ul(roi.GetIndex().GetIndex());

  // Get the higher valued corner
  Vector3ul sz(roi.GetSize().GetSize());

  // Remap to slice coordinates
  corner[0] = m_Parent->MapImageToSlice(to_float(ul));
  corner[1] = m_Parent->MapImageToSlice(to_float(ul+to_long(sz)));
}

void SnakeROIModel
::UpdateCorners(const Vector2f &uvSliceNow, const Vector2f &uvSlicePress)
{
  // Compute the corners in slice coordinates
  Vector3f corner[2];
  GetSystemROICorners(corner);

  // Get the current bounds and extents of the region of interest
  Vector3f xCornerImage[2] = {
    m_Parent->MapSliceToImage(corner[0]),
    m_Parent->MapSliceToImage(corner[1])
  };

  Vector3f clamp[2][2] =
  {
    {
      Vector3f(0.0f,0.0f,0.0f),
      xCornerImage[1] - Vector3f(1.0f,1.0f,1.0f)
    },
    {
      xCornerImage[0] + Vector3f(1.0f,1.0f,1.0f),
      to_float(m_Parent->GetDriver()->GetCurrentImageData()->GetVolumeExtents())
    }
  };

  // For each highlighted edge, update the coordinates of the affected vertex
  // by clamping to the maximum range
  for (unsigned int dir=0;dir<2;dir++)
    {
    for (unsigned int i=0;i<2;i++)
      {
      if (m_Highlight.Highlighted[dir][i])
        {
        // Horizontal edge affects the y of the vertex and vice versa
        corner[i](1-dir) =
          m_CornerDragStart[i](1-dir) + uvSliceNow(1-dir) - uvSlicePress(1-dir);

        // Map the affected vertex to image space
        Vector3f vImage = m_Parent->MapSliceToImage(corner[i]);

        // Clamp the affected vertex in image space
        Vector3f vImageClamped = vImage.clamp(clamp[i][0],clamp[i][1]);

        // Map the affected vertex back into slice space
        corner[i] = m_Parent->MapImageToSlice(vImageClamped);
        }
      }
    }

  // Update the region of interest in the system
  Vector3i xImageLower = to_int(m_Parent->MapSliceToImage(corner[0]));
  Vector3i xImageUpper = to_int(m_Parent->MapSliceToImage(corner[1]));

  // Create a region based on the corners
  GlobalState::RegionType roiCorner(
    to_itkIndex(xImageLower),to_itkSize(xImageUpper-xImageLower));

  // Get the system's region of interest
  GlobalState *gs = m_Parent->GetDriver()->GetGlobalState();
  GlobalState::RegionType roiSystem = gs->GetSegmentationROI();

  // The slice z-direction index and size in the ROI should retain the system's
  // previous value because we are only manipulating the slice in 2D
  unsigned int idx = m_Parent->GetSliceDirectionInImageSpace();
  roiCorner.SetIndex(idx,roiSystem.GetIndex(idx));
  roiCorner.SetSize(idx,roiSystem.GetSize(idx));

  // Update the system's ROI (TODO: make this fire an event!)
  gs->SetSegmentationROI(roiCorner);
}

void SnakeROIModel::ResetROI()
{
  // Region of interest from the main image
  GlobalState::RegionType roi =
      m_Parent->GetDriver()->GetCurrentImageData()->GetImageRegion();

  // Can't be empty!
  assert(roi.GetNumberOfPixels());

  // Update
  m_Parent->GetDriver()->GetGlobalState()->SetSegmentationROI(roi);
}

void SnakeROIModel::ProcessLeaveEvent()
{
  // Turn off the highlight
  SnakeROISideSelectionState blank;
  if(m_Highlight != blank)
    {
    m_Highlight = blank;
    InvokeEvent(ModelUpdateEvent());
    }
}

void SnakeROIModel::ProcessEnterEvent()
{
  // Nothing to do here!
}


