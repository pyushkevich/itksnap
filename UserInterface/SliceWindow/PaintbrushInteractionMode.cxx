/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: PaintbrushInteractionMode.cxx,v $
  Language:  C++
  Date:      $Date: 2009/01/31 09:02:50 $
  Version:   $Revision: 1.14 $
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
#include "PaintbrushInteractionMode.h"

#include "itkRegionOfInterestImageFilter.h"
#include "itkGradientAnisotropicDiffusionImageFilter.h"
#include "itkGradientMagnitudeImageFilter.h"
#include "itkWatershedImageFilter.h"
#include "GlobalState.h"
#include "PolygonDrawing.h"
#include "UserInterfaceBase.h"
#include "IRISException.h"
#include "IRISApplication.h"
#include "IRISImageData.h"
#include "SNAPAppearanceSettings.h"
#include <algorithm>

using namespace std;


class BrushWatershedPipeline
{
public:
  typedef itk::Image<GreyType, 3> GreyImageType;
  typedef itk::Image<LabelType, 3> LabelImageType;
  typedef itk::Image<float, 3> FloatImageType;
  typedef itk::Image<unsigned long, 3> WatershedImageType;
  typedef WatershedImageType::IndexType IndexType;

  BrushWatershedPipeline()
    {
    roi = ROIType::New();
    adf = ADFType::New();
    adf->SetInput(roi->GetOutput());
    adf->SetConductanceParameter(0.5);
    gmf = GMFType::New();
    gmf->SetInput(adf->GetOutput());
    wf = WFType::New();
    wf->SetInput(gmf->GetOutput());
    }

  void PrecomputeWatersheds(
    GreyImageType *grey, 
    LabelImageType *label, 
    itk::ImageRegion<3> region,
    itk::Index<3> vcenter,
    size_t smoothing_iter)
    {
    this->region = region;

    // Get the offset of vcenter in the region
    if(region.IsInside(vcenter))
      for(size_t d = 0; d < 3; d++)
        this->vcenter[d] = vcenter[d] - region.GetIndex()[d];
    else
      for(size_t d = 0; d < 3; d++)
        this->vcenter[d] = region.GetSize()[d] / 2;

    // Create a backup of the label image
    LROIType::Pointer lroi = LROIType::New();
    lroi->SetInput(label);
    lroi->SetRegionOfInterest(region);
    lroi->Update();
    lsrc = lroi->GetOutput();
    lsrc->DisconnectPipeline();

    // Initialize the watershed pipeline
    roi->SetInput(grey);
    roi->SetRegionOfInterest(region);
    adf->SetNumberOfIterations(smoothing_iter);

    // Set the initial level to lowest possible - to get all watersheds
    wf->SetLevel(1.0);
    wf->Update();
    }

  void RecomputeWatersheds(double level)
    {
    // Reupdate the filter with new level
    wf->SetLevel(level);
    wf->Update();
    }

  bool IsPixelInSegmentation(IndexType idx)
    {
    // Get the watershed ID at the center voxel 
    unsigned long wctr = wf->GetOutput()->GetPixel(vcenter);
    unsigned long widx = wf->GetOutput()->GetPixel(idx);
    return wctr == widx;
    }

  bool UpdateLabelImage(
    LabelImageType *ltrg, 
    CoverageModeType mode, 
    LabelType drawing_color,
    LabelType overwrt_color)
    {
    // Get the watershed ID at the center voxel 
    unsigned long wid = wf->GetOutput()->GetPixel(vcenter);

    // Keep track of changed voxels
    bool flagChanged = false;

    // Do the update 
    typedef itk::ImageRegionConstIterator<WatershedImageType> WIter;
    typedef itk::ImageRegionIterator<LabelImageType> LIter;
    WIter wit(wf->GetOutput(), wf->GetOutput()->GetBufferedRegion());
    LIter sit(lsrc, lsrc->GetBufferedRegion());
    LIter tit(ltrg, region);
    for(; !wit.IsAtEnd(); ++sit,++tit,++wit)
      {
      LabelType pxLabel = sit.Get();
      if(wit.Get() == wid)
        {
        // Standard paint mode
        if (mode == PAINT_OVER_ALL || 
          (mode == PAINT_OVER_ONE && pxLabel == overwrt_color) ||
          (mode == PAINT_OVER_COLORS && pxLabel != 0))
          {
          pxLabel = drawing_color;
          }
        }
      if(pxLabel != tit.Get())
        {
        tit.Set(pxLabel);
        flagChanged = true;
        }
      }

    if(flagChanged)
      ltrg->Modified();
    return flagChanged;
    }

private:
  typedef itk::RegionOfInterestImageFilter<GreyImageType, FloatImageType> ROIType;
  typedef itk::RegionOfInterestImageFilter<LabelImageType, LabelImageType> LROIType;
  typedef itk::GradientAnisotropicDiffusionImageFilter<FloatImageType,FloatImageType> ADFType;
  typedef itk::GradientMagnitudeImageFilter<FloatImageType, FloatImageType> GMFType;
  typedef itk::WatershedImageFilter<FloatImageType> WFType;
  
  ROIType::Pointer roi;  
  ADFType::Pointer adf;
  GMFType::Pointer gmf;
  WFType::Pointer wf;

  itk::ImageRegion<3> region;
  LabelImageType::Pointer lsrc;
  itk::Index<3> vcenter;
};




PaintbrushInteractionMode
::PaintbrushInteractionMode(GenericSliceWindow *parent)
: GenericSliceWindow::EventHandler(parent)
{                
  m_MouseInside = false;
  m_Watershed = new BrushWatershedPipeline();
}

PaintbrushInteractionMode
::~PaintbrushInteractionMode()
{
}

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
  if(ps.mode == PAINTBRUSH_ROUND)
    {
    return xTest.squared_magnitude() <= (ps.radius-0.25) * (ps.radius-0.25);
    }
  else
    {
    return xTest.inf_norm() <= ps.radius - 0.25;
    }
  }

void 
PaintbrushInteractionMode::
BuildBrush(const PaintbrushSettings &ps)
{
  // This is a simple 2D marching algorithm. At any given state of the 
  // marching, there is a 'tail' and a 'head' of an arrow. To the right
  // of the arrow is a voxel that's inside the brush and to the left a
  // voxel that's outside. Depending on the two voxels that are 
  // ahead of the arrow to the left and right (in in, in out, out out)
  // at the next step the arrow turns right, continues straight or turns
  // left. This goes on until convergence
  
  // Initialize the marching. This requires constructing the first arrow
  // and marching it to the left until it is between out and in voxels.
  // If the brush has even diameter, the arrow is from (0,0) to (1,0). If
  // the brush has odd diameter (center at voxel center) then the arrow 
  // is from (-0.5, -0.5) to (-0.5, 0.5)
  Vector2d xTail, xHead;
  if(fmod(ps.radius,1.0) == 0)
    { xTail = Vector2d(0.0, 0.0); xHead = Vector2d(0.0, 1.0); }
  else
    { xTail = Vector2d(-0.5, -0.5); xHead = Vector2d(-0.5, 0.5); }

  // Shift the arrow to the left until it is in position
  while(TestInside(Vector2d(xTail(0) - 0.5, xTail(1) + 0.5),ps))
    { xTail(0) -= 1.0; xHead(0) -= 1.0; }

  // Record the starting point, which is the current tail. Once the head
  // returns to the starting point, the loop is done
  Vector2d xStart = xTail;

  // Do the loop
  m_Walk.clear();
  size_t n = 0;
  while((xHead - xStart).squared_magnitude() > 0.01 && (++n) < 10000)
    {
    // Add the current head to the loop
    m_Walk.push_back(xHead);

    // Check the voxels ahead to the right and left
    Vector2d xStep = xHead - xTail;
    Vector2d xLeft(-xStep(1), xStep(0));
    Vector2d xRight(xStep(1), -xStep(0));
    bool il = TestInside(xHead + 0.5 * (xStep + xLeft),ps);
    bool ir = TestInside(xHead + 0.5 * (xStep + xRight),ps);

    // Update the tail
    xTail = xHead;

    // Decide which way to go
    if(il && ir)
      xHead += xLeft;
    else if(!il && ir)
      xHead += xStep;
    else if(!il && !ir)
      xHead += xRight;
    else 
      assert(0);
    }

  // Add the last vertex
  m_Walk.push_back(xStart);
}

/*
void 
PaintbrushInteractionMode::
BuildBrush(const PaintbrushSettings &ps)
{
  Vector2d xFirstVox(0, 0);

  // Shift in case the center of the brush falls at vertex center
  
  // Step left from the center in units of 0.5 until we are outside
  // of the brush
  while(TestInside(xFirstVox, ps))
    xFirstVox(0) -= 1.0;

  // Find the left-most voxel that is inside of the brush
  // Vector2d xFirstVox(0, 0);
  // while(TestInside(xFirstVox, ps))
  //  xFirstVox(0) -= 1.0;

  // Set the starting location
  Vector2d xStart(xFirstVox(0) + 0.5, -0.5);
  Vector2d xWalk(xFirstVox(0) + 0.5, 0.5);
  Vector2d xStep(0, 1);

  if(fmod(ps.radius, 1.0) == 0)
    {
    xStart = Vector2d(xFirstVox(0) + 1.0, 0.0);
    xWalk = Vector2d(xFirstVox(0) + 1.0, 1.0);
    }

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
*/

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

  // Set line properties
  glPushAttrib(GL_LINE_BIT | GL_COLOR_BUFFER_BIT);

  // Apply the line properties
  glColor3dv(elt.NormalColor.data_block());
  SNAPAppearanceSettings::ApplyUIElementLineSettings(elt);

  // Get the brush position
  Vector3f xPos;
  if(fmod(pbs.radius,1.0) == 0)
    xPos = m_Parent->MapImageToSlice(to_float(m_MousePosition));
  else
    xPos = m_Parent->MapImageToSlice(to_float(m_MousePosition) + Vector3f(0.5f));

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

/*
#include "itkImageFileWriter.h"

template<class ImageType> void
DebugDumpImage(ImageType *img, const char *fname)
{
  typedef itk::ImageFileWriter<ImageType> WriterType;
  WriterType::Pointer w = WriterType::New();
  w->SetFileName(fname);
  w->SetInput(img);
  w->Update();
}
*/


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

  // Whether watershed filter is used (adaptive brush)
  bool flagWatershed = (
    pbs.mode == PAINTBRUSH_WATERSHED 
    && event.Button == FL_LEFT_MOUSE 
    && event.Id == FL_PUSH);

  // Define a region of interest
  LabelImageWrapper::ImageType::RegionType xTestRegion;
  for(size_t i = 0; i < 3; i++)
    {
    if(i != imgLabel->GetDisplaySliceImageAxis(m_Parent->m_Id) || pbs.flat == false)
      {
      // For watersheds, the radius must be > 2
      double rad = (flagWatershed && pbs.radius < 1.5) ? 1.5 : pbs.radius;
      xTestRegion.SetIndex(i, (long) (m_MousePosition(i) - rad)); // + 1);
      xTestRegion.SetSize(i, (long) (2 * rad + 1)); // - 1);
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

  // Special code for Watershed brush
  if(flagWatershed)
    {
    // Precompute the watersheds
    m_Watershed->PrecomputeWatersheds(
      m_Driver->GetCurrentImageData()->GetGrey()->GetImage(),
      m_Driver->GetCurrentImageData()->GetSegmentation()->GetImage(),
      xTestRegion, to_itkIndex(m_MousePosition), pbs.watershed.smooth_iterations);

    m_Watershed->RecomputeWatersheds(pbs.watershed.level);
    }

  // Shift vector (different depending on whether the brush has odd/even diameter
  Vector3f offset(0.0);
  if(fmod(pbs.radius,1.0)==0)
    {
    offset.fill(0.5);
    offset(m_Parent->m_ImageAxes[2]) = 0.0;
    }

  // Iterate over the region
  LabelImageWrapper::Iterator it(imgLabel->GetImage(), xTestRegion);
  for(; !it.IsAtEnd(); ++it)
    {
    // Check if we are inside the sphere
    LabelImageWrapper::ImageType::IndexType idx = it.GetIndex();
    Vector3f xDelta = offset + to_float(Vector3l(idx.GetIndex())) - to_float(m_MousePosition);

    Vector3d xDeltaSliceSpace = to_double(
      m_Parent->m_ImageToDisplayTransform.TransformVector(xDelta));

    // Check if the pixel is inside
    if(!TestInside(xDeltaSliceSpace, pbs))
      continue;

    // Check if the pixel is in the watershed
    LabelImageWrapper::ImageType::IndexType idxoff = to_itkIndex(
      Vector3l(idx.GetIndex()) - Vector3l(xTestRegion.GetIndex().GetIndex()));
    if(flagWatershed && !m_Watershed->IsPixelInSegmentation(idxoff))
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
  else
    {
    m_Parent->GetCanvas()->redraw();
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

    // Record the event
    m_LastMouseEvent = event;

    // Eat the event unless cursor chasing is enabled
    return pbs.chase ? 0 : 1;              
    }
  else return 0;
}

void 
PaintbrushInteractionMode
::ComputeMousePosition(const Vector3f &xEvent)
  {   
  // Get the paintbrush properties
  PaintbrushSettings pbs = 
    m_ParentUI->GetDriver()->GetGlobalState()->GetPaintbrushSettings();

  // Find the pixel under the mouse
  Vector3f xClick = m_Parent->MapWindowToSlice(xEvent.extract(2));

  // Compute the new cross-hairs position in image space
  Vector3f xCross = m_Parent->MapSliceToImage(xClick);

  // Round the cross-hairs position down to integer
  Vector3i xCrossInteger;
  if(fmod(pbs.radius, 1.0) == 0.0)
    {
    Vector3f offset(0.5);
    offset(m_Parent->m_ImageAxes[2]) = 0.0;
    xCrossInteger = to_int(xCross + offset);
    }
  else
    xCrossInteger = to_int(xCross);
  
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
  ComputeMousePosition(event.XSpace);

  // Repaint
  m_ParentUI->RedrawWindows();

  return 1;
  }

int 
PaintbrushInteractionMode
::OnMouseEnter(const FLTKEvent &event)
  {
  // Find the pixel under the mouse
  ComputeMousePosition(event.XSpace);

  // Repaint
  m_ParentUI->RedrawWindows();

  // Record the event
  m_LastMouseEvent = event;

  return 0;  
  }

int 
PaintbrushInteractionMode
::OnMouseLeave(const FLTKEvent &event)
  {  
  // Repaint
  m_MouseInside = false;
  m_ParentUI->RedrawWindows();

  // Record the event
  m_LastMouseEvent = event;

  return 0;
  }

int
PaintbrushInteractionMode
::DragReleaseHandler(FLTKEvent const &event, FLTKEvent const &pressEvent, bool drag)
{
  // Get the paintbrush properties
  PaintbrushSettings pbs = 
    m_ParentUI->GetDriver()->GetGlobalState()->GetPaintbrushSettings();

  // The behavior is different for 'fast' regular brushes and adaptive brush. For the 
  // adaptive brush, dragging is disabled.
  if(pbs.mode == PAINTBRUSH_WATERSHED && event.Button == FL_LEFT_MOUSE)
    {
    if(event.Id == FL_RELEASE)
      m_Parent->m_ParentUI->StoreUndoPoint("Drawing with paintbrush");
    
    return pbs.chase ? 0 : 1; 
    }

  else
    {
    // Check if the right button was pressed
    if(event.Button == FL_LEFT_MOUSE || event.Button == FL_RIGHT_MOUSE)
      {
      // See how much we have moved since the last event
      double delta = (event.XCanvas - m_LastMouseEvent.XCanvas).magnitude();
      if(delta > pbs.radius)
        {
        size_t nSteps = (int)ceil(delta / pbs.radius);
        for(size_t i = 0; i < nSteps; i++)
          {
          float t = (1.0 + i) / nSteps;
          Vector3f X = t * m_LastMouseEvent.XSpace + (1.0f - t) * event.XSpace;
          ComputeMousePosition(X);
          ApplyBrush(event);
          }
        }
      else
        {
        // Find the pixel under the mouse
        ComputeMousePosition(event.XSpace);

        // Scan convert the points into the slice
        ApplyBrush(event);    

        // Set an undo point if not in drag mode
        if(!drag)
          m_Parent->m_ParentUI->StoreUndoPoint("Drawing with paintbrush");
        }

      // Record the event
      m_LastMouseEvent = event;

      // Eat the event unless cursor chasing is enabled
      return pbs.chase ? 0 : 1;                        
      }
    }

  return 0;
}

int
PaintbrushInteractionMode
::OnMouseRelease(FLTKEvent const &event, FLTKEvent const &pressEvent)
{
  return DragReleaseHandler(event, pressEvent,false);
}

int
PaintbrushInteractionMode
::OnMouseDrag(FLTKEvent const &event, FLTKEvent const &pressEvent)
{
  return DragReleaseHandler(event, pressEvent,true);
}

int
PaintbrushInteractionMode
::OnKeyDown(FLTKEvent const &event)
{
  // Get the paintbrush properties
  PaintbrushSettings pbs = 
    m_ParentUI->GetDriver()->GetGlobalState()->GetPaintbrushSettings();

  // The behavior is different for 'fast' regular brushes and adaptive brush. For the 
  // adaptive brush, dragging affects parameter settings
  if(pbs.mode == PAINTBRUSH_WATERSHED)  
    {
    double shift = 0.0;
    if(event.Key == ',')
      shift = -1.0;
    else if(event.Key == '.')
      shift = 1.0;
    if(shift != 0.0)
      {
      pbs.watershed.level += shift * 0.05;
      if(pbs.watershed.level < 0.0) pbs.watershed.level = 0.0;
      if(pbs.watershed.level > 1.0) pbs.watershed.level = 1.0;

      
      m_ParentUI->GetDriver()->GetGlobalState()->SetPaintbrushSettings(pbs);
      m_ParentUI->UpdatePaintbrushAttributes();

      return 1;
      }
    }
  return 0;
}

int
PaintbrushInteractionMode
::OnShortcut(FLTKEvent const &event)
{
  return 0;
}


