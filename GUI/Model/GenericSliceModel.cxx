/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: Filename.cxx,v $
  Language:  C++
  Date:      $Date: 2010/10/18 11:25:44 $
  Version:   $Revision: 1.12 $
  Copyright (c) 2011 Paul A. Yushkevich

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

=========================================================================*/

#include <GenericSliceModel.h>
#include <GlobalUIModel.h>
#include <IRISException.h>
#include <IRISApplication.h>
#include <GenericImageData.h>
#include <SNAPAppearanceSettings.h>
#include <DisplayLayoutModel.h>

#include <itkImage.h>
#include <itkImageRegionIteratorWithIndex.h>

GenericSliceModel::GenericSliceModel()
{
  // Copy parent pointers
  m_ParentUI = NULL;
  m_Driver = NULL;

  // Set the window ID
  m_Id = -1;

  // Initalize the margin
  m_Margin = 2;

  // Initialize the zoom management
  m_ManagedZoom = false;

  // The slice is not yet initialized
  m_SliceInitialized = false;

  // Viewport size reporter is NULL
  m_SizeReporter = NULL;
}

void GenericSliceModel::Initialize(GlobalUIModel *model, int index)
{
  // Copy parent pointers
  m_ParentUI = model;
  m_Driver = model->GetDriver();

  // Set the window ID
  m_Id = index;

  // The slice is not yet initialized
  m_SliceInitialized = false;

  // Listen to events that require action from this object
  Rebroadcast(m_Driver, LayerChangeEvent(), ModelUpdateEvent());

  // Listen to changes in the layout of the slice view into cells. When
  // this change occurs, we have to modify the size of the slice views
  DisplayLayoutModel *dlm = m_ParentUI->GetDisplayLayoutModel();
  Rebroadcast(dlm, DisplayLayoutModel::LayerLayoutChangeEvent(), ModelUpdateEvent());
}

void GenericSliceModel
::SetSizeReporter(ViewportSizeReporter *reporter)
{
  m_SizeReporter = reporter;

  // Rebroadcast the events from the reporter downstream to force an update
  Rebroadcast(m_SizeReporter,
              ViewportSizeReporter::ViewportResizeEvent(),
              ModelUpdateEvent());
}


void GenericSliceModel::OnUpdate()
{
  // Has there been a change in the image dimensions?
  if(m_EventBucket->HasEvent(MainImageDimensionsChangeEvent()))
    {
    // Do a complete initialization
    this->InitializeSlice(m_Driver->GetCurrentImageData());
    }
  // TODO: what is the ValueChangeEvent here???
  else if(m_EventBucket->HasEvent(ViewportSizeReporter::ViewportResizeEvent())
          || m_EventBucket->HasEvent(DisplayLayoutModel::LayerLayoutChangeEvent())
          || m_EventBucket->HasEvent(ValueChangedEvent()))
    {
    if(this->IsSliceInitialized())
      {
      // Check if the zoom should be changed in response to this operation. This
      // is so if the zoom is currently equal to the optimal zoom, and there is
      // no linked zoom
      bool rezoom = (m_ViewZoom == m_OptimalZoom) && !m_ManagedZoom;

      // Just recompute the optimal zoom factor
      this->ComputeOptimalZoom();

      // Keep zoom optimal if before it was optimal
      if(rezoom)
        this->SetViewZoom(m_OptimalZoom);
      }
    }

}

void GenericSliceModel::ComputeOptimalZoom()
{
  // Should be fully initialized
  assert(IsSliceInitialized());

  // Compute slice size in spatial coordinates
  Vector2f worldSize(
    m_SliceSize[0] * m_SliceSpacing[0],
    m_SliceSize[1] * m_SliceSpacing[1]);

  // Set the view position (position of the center of the image?)
  m_OptimalViewPosition = worldSize * 0.5f;

  // Reduce the width and height of the slice by the margin
  Vector2ui size = this->GetSize();
  Vector2i szCanvas =
      Vector2i(size[0], size[1]) - Vector2i(2 * m_Margin);

  // Compute the ratios of window size to slice size
  Vector2f ratios(
    szCanvas(0) / worldSize(0),
    szCanvas(1) / worldSize(1));

  // The zoom factor is the bigger of these ratios, the number of pixels
  // on the screen per millimeter in world space
  m_OptimalZoom = ratios.min_value();
}


GenericSliceModel
::~GenericSliceModel()
{
}



void
GenericSliceModel
::InitializeSlice(GenericImageData *imageData)
{
  // Store the image data pointer
  m_ImageData = imageData;

  // Quit if there is no image loaded
  if (!m_ImageData->IsMainLoaded())
    {
    m_SliceInitialized = false;
    return;
    }

  // Store the transforms between the display and image spaces
  m_ImageToDisplayTransform =
    imageData->GetImageGeometry().GetImageToDisplayTransform(m_Id);
  m_DisplayToImageTransform =
    imageData->GetImageGeometry().GetDisplayToImageTransform(m_Id);
  m_DisplayToAnatomyTransform =
    imageData->GetImageGeometry().GetAnatomyToDisplayTransform(m_Id).Inverse();

  // Get the volume extents & voxel scale factors
  Vector3ui imageSizeInImageSpace = m_ImageData->GetVolumeExtents();
  Vector3f imageScalingInImageSpace = to_float(m_ImageData->GetImageSpacing());

  // Initialize quantities that depend on the image and its transform
  for(unsigned int i = 0; i < 3; i++)
    {
    // Get the direction in image space that corresponds to the i'th
    // direction in slice space
    m_ImageAxes[i] = m_DisplayToImageTransform.GetCoordinateIndexZeroBased(i);

    // Record the size and scaling of the slice
    m_SliceSize[i] = imageSizeInImageSpace[m_ImageAxes[i]];
    m_SliceSpacing[i] = imageScalingInImageSpace[m_ImageAxes[i]]; // TODO: Reverse sign by orientation?
    }

  // We have been initialized
  m_SliceInitialized = true;

  // Notify listeners that image content has changed
  // InvokeEvent(SliceModelImageDimensionsChangeEvent());

  // Compute the optimal zoom for this slice
  ComputeOptimalZoom();

  // setup default view - fit to window
  ResetViewToFit();
}

Vector2i
GenericSliceModel
::GetOptimalCanvasSize()
{
  // Compute slice size in spatial coordinates
  Vector2i optSize(
    (int) ceil(m_SliceSize[0] * m_SliceSpacing[0] * m_ViewZoom + 2 * m_Margin),
    (int) ceil(m_SliceSize[1] * m_SliceSpacing[1] * m_ViewZoom + 2 * m_Margin));

  return optSize;
}

void
GenericSliceModel
::ResetViewToFit()
{
  // Should be fully initialized
  assert(IsSliceInitialized());

  // The zoom factor is the bigger of these ratios, the number of pixels
  // on the screen per millimeter in world space
  SetViewZoom(m_OptimalZoom);
  SetViewPosition(m_OptimalViewPosition);
}

Vector3f
GenericSliceModel
::MapSliceToImage(const Vector3f &xSlice)
{
  assert(IsSliceInitialized());

  // Get corresponding position in display space
  return m_DisplayToImageTransform.TransformPoint(xSlice);
}

/**
 * Map a point in image coordinates to slice coordinates
 */
Vector3f
GenericSliceModel
::MapImageToSlice(const Vector3f &xImage)
{
  assert(IsSliceInitialized());

  // Get corresponding position in display space
  return  m_ImageToDisplayTransform.TransformPoint(xImage);
}

Vector2f
GenericSliceModel
::MapSliceToWindow(const Vector3f &xSlice)
{
  assert(IsSliceInitialized());

  // Adjust the slice coordinates by the scaling amounts
  Vector2f uvScaled(
    xSlice(0) * m_SliceSpacing(0),xSlice(1) * m_SliceSpacing(1));

  // Compute the window coordinates
  Vector2ui size = this->GetSize();
  Vector2f uvWindow =
    m_ViewZoom * (uvScaled - m_ViewPosition) +
      Vector2f(0.5f * size[0],0.5f * size[1]);

  // That's it, the projection matrix is set up in the scaled-slice coordinates
  return uvWindow;
}

Vector3f
GenericSliceModel
::MapWindowToSlice(const Vector2f &uvWindow)
{
  assert(IsSliceInitialized() && m_ViewZoom > 0);

  // Compute the scaled slice coordinates
  Vector2ui size = this->GetSize();
  Vector2f winCenter(0.5f * size[0],0.5f * size[1]);
  Vector2f uvScaled =
    m_ViewPosition + (uvWindow - winCenter) / m_ViewZoom;

  // The window coordinates are already in the scaled-slice units
  Vector3f uvSlice(
    uvScaled(0) / m_SliceSpacing(0),
    uvScaled(1) / m_SliceSpacing(1),
    this->GetCursorPositionInSliceCoordinates()[2]);

  // Return this vector
  return uvSlice;
}

Vector3f
GenericSliceModel
::MapWindowOffsetToSliceOffset(const Vector2f &uvWindowOffset)
{
  assert(IsSliceInitialized() && m_ViewZoom > 0);

  Vector2f uvScaled = uvWindowOffset / m_ViewZoom;

  // The window coordinates are already in the scaled-slice units
  Vector3f uvSlice(
    uvScaled(0) / m_SliceSpacing(0),
    uvScaled(1) / m_SliceSpacing(1),
    0);

  // Return this vector
  return uvSlice;
}

Vector2f
GenericSliceModel
::MapSliceToPhysicalWindow(const Vector3f &xSlice)
{
  assert(IsSliceInitialized());

  // Compute the physical window coordinates
  Vector2f uvPhysical;
  uvPhysical[0] = xSlice[0] * m_SliceSpacing[0];
  uvPhysical[1] = xSlice[1] * m_SliceSpacing[1];

  return uvPhysical;
}

void
GenericSliceModel
::ResetViewPosition()
{
  // Compute slice size in spatial coordinates
  Vector2f worldSize(
    m_SliceSize[0] * m_SliceSpacing[0],
    m_SliceSize[1] * m_SliceSpacing[1]);

  // Set the view position (position of the center of the image?)
  m_ViewPosition = worldSize * 0.5f;

  // Update view
  InvokeEvent(SliceModelGeometryChangeEvent());
}

void
GenericSliceModel
::SetViewPositionRelativeToCursor(Vector2f offset)
{
  // Get the crosshair position
  Vector3ui xCursorInteger = m_Driver->GetCursorPosition();

  // Shift the cursor position by by 0.5 in order to have it appear
  // between voxels
  Vector3f xCursorImage = to_float(xCursorInteger) + Vector3f(0.5f);

  // Get the cursor position on the slice
  Vector3f xCursorSlice = MapImageToSlice(xCursorImage);

  // Subtract from the view position
  Vector2f vp;
  vp[0] = offset[0] + xCursorSlice[0] * m_SliceSpacing[0];
  vp[1] = offset[1] + xCursorSlice[1] * m_SliceSpacing[1];
  SetViewPosition(vp);
}

Vector2f
GenericSliceModel
::GetViewPositionRelativeToCursor()
{
  // Get the crosshair position
  Vector3ui xCursorInteger = m_Driver->GetCursorPosition();

  // Shift the cursor position by by 0.5 in order to have it appear
  // between voxels
  Vector3f xCursorImage = to_float(xCursorInteger) + Vector3f(0.5f);

  // Get the cursor position on the slice
  Vector3f xCursorSlice = MapImageToSlice(xCursorImage);

  // Subtract from the view position
  Vector2f offset;
  offset[0] = m_ViewPosition[0] - xCursorSlice[0] * m_SliceSpacing[0];
  offset[1] = m_ViewPosition[1] - xCursorSlice[1] * m_SliceSpacing[1];

  return offset;
}

void GenericSliceModel::CenterViewOnCursor()
{
  Vector2f offset; offset.fill(0.0f);
  this->SetViewPositionRelativeToCursor(offset);
}

/*
GenericSliceModel *
GenericSliceModel
::GenericSliceModel()
{
  SliceWindowCoordinator *swc = m_ParentUI->GetSliceCoordinator();
  return swc->GetWindow( (m_Id+1) % 3);
}
*/

bool
GenericSliceModel
::IsThumbnailOn()
{
  return m_ParentUI->GetAppearanceSettings()->GetFlagDisplayZoomThumbnail()
      && m_ViewZoom > m_OptimalZoom;
}

Vector3f GenericSliceModel::GetCursorPositionInSliceCoordinates()
{
  Vector3ui cursorImageSpace = m_Driver->GetCursorPosition();
  Vector3f cursorDisplaySpace =
    m_ImageToDisplayTransform.TransformPoint(
      to_float(cursorImageSpace) + Vector3f(0.5f));
  return cursorDisplaySpace;
}

unsigned int GenericSliceModel::GetSliceIndex()
{
  Vector3ui cursorImageSpace = m_Driver->GetCursorPosition();
  return cursorImageSpace[m_ImageAxes[2]];
}


void GenericSliceModel::UpdateSliceIndex(unsigned int newIndex)
{
  Vector3ui cursorImageSpace = m_Driver->GetCursorPosition();
  cursorImageSpace[m_ImageAxes[2]] = newIndex;
  m_Driver->SetCursorPosition(cursorImageSpace);
}

void GenericSliceModel::ComputeThumbnailProperties()
{
  // The thumbnail will occupy a specified fraction of the target canvas
  float xFraction = 0.01f *
    m_ParentUI->GetAppearanceSettings()->GetZoomThumbnailSizeInPercent();

  // But it must not exceed a predefined size in pixels in either dimension
  float xThumbMax =
    m_ParentUI->GetAppearanceSettings()->GetZoomThumbnailMaximumSize();

  // Recompute the fraction based on maximum size restriction
  Vector2ui size = this->GetSize();
  float xNewFraction = xFraction;
  if( size[0] * xNewFraction > xThumbMax )
    xNewFraction = xThumbMax * 1.0f / size[0];
  if( size[1] * xNewFraction > xThumbMax )
    xNewFraction = xThumbMax * 1.0f / size[1];

  // Set the position and size of the thumbnail, in pixels
  m_ThumbnailZoom = xNewFraction * m_OptimalZoom;
  m_ThumbnailPosition.fill(5);
  m_ThumbnailSize[0] =
      (int)(m_SliceSize[0] * m_SliceSpacing[0] * m_ThumbnailZoom);
  m_ThumbnailSize[1] =
      (int)(m_SliceSize[1] * m_SliceSpacing[1] * m_ThumbnailZoom);
}

unsigned int GenericSliceModel::GetNumberOfSlices() const
{
  return m_SliceSize[2];
}

/*
void GenericSliceModel::OnSourceDataUpdate()
{
  this->InitializeSlice(m_Driver->GetCurrentImageData());
}
*/

void GenericSliceModel::SetViewPosition(Vector2f pos)
{
  if(m_ViewPosition != pos)
    {
    m_ViewPosition = pos;
    InvokeEvent(SliceModelGeometryChangeEvent());
    }
}


/*
GenericSliceWindow::EventHandler
::EventHandler(GenericSliceWindow *parent)
: InteractionMode(parent->GetCanvas())
{
  m_Parent = parent;
}

void
GenericSliceWindow::EventHandler
::Register()
{
  m_Driver = m_Parent->m_Driver;
  m_ParentUI = m_Parent->m_ParentUI;
  m_GlobalState = m_Parent->m_GlobalState;
}

#include <itksys/SystemTools.hxx>

int
GenericSliceWindow::OnDragAndDrop(const FLTKEvent &event)
{
  // Check if it is a real file
  if(event.Id == FL_PASTE)
    {
    if(itksys::SystemTools::FileExists(Fl::event_text(), true))
      {
      m_ParentUI->OpenDraggedContent(Fl::event_text(), true);
      return 1;
      }
    return 0;
    }
  else
    return 1;
}
*/


unsigned int
GenericSliceModel
::MergeSliceSegmentation(itk::Image<unsigned char, 2> *drawing)
{
  // Z position of slice
  float zpos = this->GetCursorPositionInSliceCoordinates()[2];
  return m_Driver->UpdateSegmentationWithSliceDrawing(
        drawing, m_DisplayToImageTransform, zpos, "Polygon Drawing");
}

Vector2ui GenericSliceModel::GetSize()
{
  Vector2ui viewport = m_SizeReporter->GetViewportSize();
  DisplayLayoutModel *dlm = m_ParentUI->GetDisplayLayoutModel();
  Vector2ui layout = dlm->GetSliceViewLayerTilingModel()->GetValue();
  unsigned int rows = layout[0], cols = layout[1];
  return Vector2ui(viewport[0] / cols, viewport[1] / rows);
}


