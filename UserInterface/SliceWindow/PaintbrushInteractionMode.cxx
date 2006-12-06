/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    $RCSfile: PaintbrushInteractionMode.cxx,v $
  Language:  C++
  Date:      $Date: 2006/12/06 01:26:07 $
  Version:   $Revision: 1.2 $
  Copyright (c) 2003 Insight Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notices for more information.
=========================================================================*/
#include "PaintbrushInteractionMode.h"

#include "GlobalState.h"
#include "PolygonDrawing.h"
#include "UserInterfaceBase.h"
#include "IRISApplication.h"
#include "IRISImageData.h"
#include "SNAPAppearanceSettings.h"
#include <algorithm>

using namespace std;


PaintbrushInteractionMode
::PaintbrushInteractionMode(GenericSliceWindow *parent)
: GenericSliceWindow::EventHandler(parent)
{                
  m_MouseInside = false;
}

PaintbrushInteractionMode
::~PaintbrushInteractionMode()
{
}

#include "Fl/fl_ask.H"

bool
PaintbrushInteractionMode::
TestInside(const Vector2d &x, const PaintbrushSettings &ps)
  {
  return this->TestInside(Vector3d(x(0), x(1), 0.0), ps);
  }

bool
PaintbrushInteractionMode::
TestInside(const Vector3d &x, const PaintbrushSettings &ps)
  {
  // Determine how to scale the voxels   
  Vector3d xTest = x;
  if(ps.isotropic)
    {
    double xMinVoxelDim = m_Parent->m_SliceSpacing.min_value();
    xTest(0) *= m_Parent->m_SliceSpacing(0) / xMinVoxelDim;
    xTest(1) *= m_Parent->m_SliceSpacing(1) / xMinVoxelDim;
    xTest(2) *= m_Parent->m_SliceSpacing(2) / xMinVoxelDim;
    }

  // Test inside/outside  
  if(ps.shape == PAINTBRUSH_ROUND)
    {
    return xTest.squared_magnitude() < (ps.radius * ps.radius) - 0.5;
    }
  else if(ps.shape == PAINTBRUSH_RECTANGULAR)
    {
    return xTest.inf_norm() < ps.radius - 0.5;
    }
  else return false;
  }

void 
PaintbrushInteractionMode::
BuildBrush(const PaintbrushSettings &ps)
{

  // Find the left-most voxel that is inside of the brush
  Vector2d xFirstVox(0, 0);
  while(TestInside(xFirstVox, ps))
    xFirstVox(0) -= 1.0;

  // Set the starting location
  Vector2d xStart(xFirstVox(0) + 0.5, -0.5);
  Vector2d xWalk(xFirstVox(0) + 0.5, 0.5);
  Vector2d xStep(0, 1);

  int i = 0;
  m_Walk.clear();
  while((xStart - xWalk).squared_magnitude() > 0.01 && ++i < 10000)
    {
    // Add the current walk step
    m_Walk.push_back(xWalk);

    // Compute the walk direction
    Vector2d xLeft(-xStep(1), xStep(0));
    Vector2d xRight(xStep(1), -xStep(0));
    if(TestInside(xWalk + 0.5 * (xStep + xLeft), ps))
      xStep = xLeft;
    else if(!TestInside(xWalk + 0.5 * (xStep + xRight), ps))
      xStep = xRight;

    // Update the walk
    xWalk = xWalk + xStep;
    }

  // Push the last point on the walk
  m_Walk.push_back(xStart);
}


void
PaintbrushInteractionMode
::OnDraw()
{
  // Leave if mouse outside of slice
  if(!m_MouseInside) return;

  // Draw the outline of the paintbrush
  PaintbrushSettings pbs = 
    m_ParentUI->GetDriver()->GetGlobalState()->GetPaintbrushSettings();

  // Paint all the edges in the paintbrush definition
  const SNAPAppearanceSettings::Element &elt = 
    m_ParentUI->GetAppearanceSettings()->GetUIElement(
    SNAPAppearanceSettings::PAINTBRUSH_OUTLINE);

  // Build the mask edges
  BuildBrush(pbs);

  // Get the cursor position on the slice
  Vector3ui xCursorInteger = m_GlobalState->GetCrosshairsPosition();
  Vector3f xCursorImage = to_float(xCursorInteger) + Vector3f(0.5f);
  Vector3f xCursorSlice = m_Parent->MapImageToSlice(xCursorImage);

  // Set line properties
  glPushAttrib(GL_LINE_BIT | GL_COLOR_BUFFER_BIT);

  // Apply the line properties
  glColor3dv(elt.NormalColor.data_block());
  SNAPAppearanceSettings::ApplyUIElementLineSettings(elt);

  // Get the brush position
  Vector3f xPos = m_Parent->MapImageToSlice(
    to_float(m_MousePosition) + Vector3f(0.5f));

  // Refit matrix so that the lines are centered on the current pixel
  glPushMatrix();
  glTranslated( xPos(0), xPos(1), 0.0 );

  // Draw the lines around the point
  glBegin(GL_LINE_LOOP);
  for(std::list<Vector2d>::iterator it = m_Walk.begin(); it != m_Walk.end(); ++it)
    glVertex2d((*it)(0), (*it)(1));
  glEnd();

  // Pop the matrix
  glPopMatrix();

  // Pop the attributes
  glPopAttrib();
}

void
PaintbrushInteractionMode
::ApplyBrush(FLTKEvent const &event)
{
  // Get the segmentation image
  LabelImageWrapper *imgLabel = m_Driver->GetCurrentImageData()->GetSegmentation();

  // Get the paint properties
  LabelType drawing_color = m_GlobalState->GetDrawingColorLabel();
  LabelType overwrt_color = m_GlobalState->GetOverWriteColorLabel();
  CoverageModeType mode = m_GlobalState->GetCoverageMode();

  // Get the paintbrush properties
  PaintbrushSettings pbs = 
    m_ParentUI->GetDriver()->GetGlobalState()->GetPaintbrushSettings();
  double r2 = pbs.radius * pbs.radius;

  // Define a region of interest
  LabelImageWrapper::ImageType::RegionType xTestRegion;
  for(size_t i = 0; i < 3; i++)
    {
    if(i != imgLabel->GetDisplaySliceImageAxis(m_Parent->m_Id) || pbs.flat == false)
      {
      xTestRegion.SetIndex(i, m_MousePosition(i) - pbs.radius + 1);
      xTestRegion.SetSize(i, 2 * pbs.radius - 1);
      }
    else
      {
      xTestRegion.SetIndex(i, m_MousePosition(i));
      xTestRegion.SetSize(i, 1);
      }
    }

  // Crop the region by the buffered region
  xTestRegion.Crop(imgLabel->GetImage()->GetBufferedRegion());

  // Flag to see if anything was changed
  bool flagUpdate = false;

  // Iterate over the region
  LabelImageWrapper::Iterator it(imgLabel->GetImage(), xTestRegion);
  for(; !it.IsAtEnd(); ++it)
    {
    // Check if we are inside the sphere
    LabelImageWrapper::ImageType::IndexType idx = it.GetIndex();
    Vector3f xDelta = to_float(Vector3l(idx.GetIndex())) - to_float(m_MousePosition);
    Vector3d xDeltaSliceSpace = to_double(
      m_Parent->m_ImageToDisplayTransform.TransformVector(xDelta));

    // Check if the pixel is inside
    if(!TestInside(xDeltaSliceSpace, pbs))
      continue;

    // if(pbs.shape == PAINTBRUSH_ROUND && xDelta.squared_magnitude() >= r2)
    //  continue;

    // Paint the pixel
    LabelType pxLabel = it.Get();

    // Standard paint mode
    if(event.Button == FL_LEFT_MOUSE)
      {
      if (mode == PAINT_OVER_ALL || 
        (mode == PAINT_OVER_ONE && pxLabel == overwrt_color) ||
        (mode == PAINT_OVER_COLORS && pxLabel != 0))
        {
        it.Set(drawing_color);
        if(pxLabel != drawing_color) flagUpdate = true;
        }
      }
    // Background paint mode (clear label over current label)
    else if(event.Button == FL_RIGHT_MOUSE)
      {
      if(drawing_color != 0 && pxLabel == drawing_color) 
        {
        it.Set(0);
        if(pxLabel != 0) flagUpdate = true;
        }
      else if(drawing_color == 0 && mode == PAINT_OVER_ONE)
        {
        it.Set(overwrt_color);
        if(pxLabel != overwrt_color) flagUpdate = true;
        }
      }
    }

  // Image has been updated
  if(flagUpdate)
    {
    imgLabel->GetImage()->Modified();
    m_ParentUI->OnPaintbrushPaint();
    m_ParentUI->RedrawWindows();
    }
}

int
PaintbrushInteractionMode
::OnMousePress(FLTKEvent const &event)
{
  // Get the paintbrush properties
  PaintbrushSettings pbs = 
    m_ParentUI->GetDriver()->GetGlobalState()->GetPaintbrushSettings();

  // Check if the right button was pressed
  if(event.Button == FL_LEFT_MOUSE || event.Button == FL_RIGHT_MOUSE)
    {
    // Scan convert the points into the slice
    ApplyBrush(event);

    // Eat the event unless cursor chasing is enabled
    return pbs.chase ? 0 : 1;              
    }
  else return 0;
}

void 
PaintbrushInteractionMode
::ComputeMousePosition(FLTKEvent const &event)
  {
  // Find the pixel under the mouse
  Vector3f xClick = m_Parent->MapWindowToSlice(event.XSpace.extract(2));

  // Compute the new cross-hairs position in image space
  Vector3f xCross = m_Parent->MapSliceToImage(xClick);

  // Round the cross-hairs position down to integer
  Vector3i xCrossInteger = to_int(xCross);
  
  // Make sure that the cross-hairs position is within bounds by clamping
  // it to image dimensions
  Vector3i xSize = to_int(m_Driver->GetCurrentImageData()->GetVolumeExtents());
  m_MousePosition = to_unsigned_int(
    xCrossInteger.clamp(Vector3i(0),xSize - Vector3i(1)));
  m_MouseInside = true;
  }

int
PaintbrushInteractionMode
::OnMouseMotion(FLTKEvent const &event)
  {
  // Find the pixel under the mouse
  ComputeMousePosition(event);

  // Repaint
  m_ParentUI->RedrawWindows();

  return 1;
  }

int 
PaintbrushInteractionMode
::OnMouseEnter(const FLTKEvent &event)
  {
  // Find the pixel under the mouse
  ComputeMousePosition(event);

  // Repaint
  m_ParentUI->RedrawWindows();

  return 1;  
  }

int 
PaintbrushInteractionMode
::OnMouseLeave(const FLTKEvent &event)
  {  
  // Repaint
  m_MouseInside = false;
  m_ParentUI->RedrawWindows();

  return 1;
  }

int
PaintbrushInteractionMode
::OnMouseRelease(FLTKEvent const &event, FLTKEvent const &pressEvent)
{
  // Get the paintbrush properties
  PaintbrushSettings pbs = 
    m_ParentUI->GetDriver()->GetGlobalState()->GetPaintbrushSettings();

  // Check if the right button was pressed
  if(event.Button == FL_LEFT_MOUSE || event.Button == FL_RIGHT_MOUSE)
    {
    // Find the pixel under the mouse
    ComputeMousePosition(event);

    // Scan convert the points into the slice
    ApplyBrush(event);
    
    // Eat the event unless cursor chasing is enabled
    return pbs.chase ? 0 : 1;                  
    }

  return 0;
}

int
PaintbrushInteractionMode
::OnMouseDrag(FLTKEvent const &event, FLTKEvent const &pressEvent)
{
  // Get the paintbrush properties
  PaintbrushSettings pbs = 
    m_ParentUI->GetDriver()->GetGlobalState()->GetPaintbrushSettings();

  // Check if the right button was pressed
  if(event.Button == FL_LEFT_MOUSE || event.Button == FL_RIGHT_MOUSE)
    {
    // Find the pixel under the mouse
    ComputeMousePosition(event);

    // Scan convert the points into the slice
    ApplyBrush(event);
    

    // Eat the event unless cursor chasing is enabled
    return pbs.chase ? 0 : 1;                        
    }

  return 0;
}

int
PaintbrushInteractionMode
::OnKeyDown(FLTKEvent const &event)
{
  return 0;
}

int
PaintbrushInteractionMode
::OnShortcut(FLTKEvent const &event)
{
  return 0;
}


