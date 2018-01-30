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

#include "OrthogonalSliceCursorNavigationModel.h"
#include "GenericSliceModel.h"
#include "IRISException.h"
#include "IRISApplication.h"
#include "GenericImageData.h"
#include "GlobalUIModel.h"
#include "SliceWindowCoordinator.h"
#include "ImageCoordinateTransform.h"

void OrthogonalSliceCursorNavigationModel::UpdateCursor(Vector2f x)
{
  // Compute the position in slice coordinates
  Vector3f xClick = m_Parent->MapWindowToSlice(x);

  // Compute the new cross-hairs position in image space
  Vector3f xCross = m_Parent->MapSliceToImage(xClick);

  // Round the cross-hairs position down to integer
  Vector3i xCrossInteger = to_int(xCross);

  // Make sure that the cross-hairs position is within bounds by clamping
  // it to image dimensions
  Vector3i xSize = to_int(m_Parent->GetDriver()->
                          GetCurrentImageData()->GetVolumeExtents());
  Vector3ui xCrossClamped = to_unsigned_int(
    xCrossInteger.clamp(Vector3i(0),xSize - Vector3i(1)));

  // Update the crosshairs position in the global state
  m_Parent->GetDriver()->SetCursorPosition(xCrossClamped);
}

void OrthogonalSliceCursorNavigationModel::ProcessKeyNavigation(Vector3i dx)
{
  // Get the displacement in image space
  Vector3f dximg =
      m_Parent->GetDisplayToImageTransform().TransformVector(to_float(dx));
  Vector3i dximgi = to_int(dximg);

  // Update the cursor
  IRISApplication *app = m_Parent->GetDriver();
  Vector3i xSize = to_int(app->GetCurrentImageData()->GetVolumeExtents());
  Vector3i cursor = to_int(app->GetCursorPosition());
  cursor += dximgi;
  cursor = cursor.clamp(Vector3i(0), xSize - 1);
  app->SetCursorPosition(to_unsigned_int(cursor));
}

void OrthogonalSliceCursorNavigationModel::BeginZoom()
{
  m_StartViewZoom = m_Parent->GetViewZoom();
}

void OrthogonalSliceCursorNavigationModel::BeginPan()
{
  m_StartViewPosition = m_Parent->GetViewPosition();
}

void OrthogonalSliceCursorNavigationModel::EndZoom() { }
void OrthogonalSliceCursorNavigationModel::EndPan() { }

void OrthogonalSliceCursorNavigationModel
::ProcessZoomGesture(float scaleFactor)
{
  // Get the slice coordinator
  SliceWindowCoordinator *coordinator =
      m_Parent->GetParentUI()->GetSliceCoordinator();

  // Sometimes the scalefactor is 1, in which case, we restore the zoom
  if(scaleFactor == 1.0f)
    {
    if(m_Parent->GetViewZoom() == m_StartViewZoom)
      return;
    else
      m_Parent->SetViewZoom(m_StartViewZoom);
    }
  else
    {
    // Compute the zoom factor (is this good?)
    float zoom = m_StartViewZoom * scaleFactor;

    // Clamp the zoom factor to reasonable limits
    zoom = coordinator->ClampZoom(m_Parent->GetId(), zoom);

    // Make sure the zoom factor is an integer fraction
    zoom = m_Parent->GetOptimalZoom() *
           ((int)(zoom / m_Parent->GetOptimalZoom() * 100)) / 100.0f;

    // Set the zoom factor using the window coordinator
    m_Parent->SetViewZoom(zoom);
    }

  // TODO: remove this!
  coordinator->OnZoomUpdateInWindow(m_Parent->GetId(), m_Parent->GetViewZoom());
}

void
OrthogonalSliceCursorNavigationModel
::ProcessPanGesture(Vector2f uvOffset)
{
  // Compute the start and end point in slice coordinates
  Vector3f zOffset = m_Parent->MapWindowOffsetToSliceOffset(uvOffset);
  Vector2f xOffset(zOffset[0] * m_Parent->GetSliceSpacing()[0],
                   zOffset[1] * m_Parent->GetSliceSpacing()[1]);

  // Under the left button, the tool changes the view_pos by the
  // distance traversed
  m_Parent->SetViewPosition(m_StartViewPosition - xOffset);
}

void OrthogonalSliceCursorNavigationModel
::ProcessScrollGesture(float scrollAmount)
{
  // Get the cross-hairs position in image space
  Vector3ui xCrossImage = m_Parent->GetDriver()->GetCursorPosition();

  // Map it into slice space
  Vector3f xCrossSlice =
    m_Parent->MapImageToSlice(to_float(xCrossImage) + Vector3f(0.5f));

  // Advance by the scroll amount
  xCrossSlice[2] += scrollAmount;

  // Map back into display space
  xCrossImage = to_unsigned_int(m_Parent->MapSliceToImage(xCrossSlice));

  // Clamp by the image size
  Vector3ui xSize =
      m_Parent->GetDriver()->GetCurrentImageData()->GetVolumeExtents();
  Vector3ui xCrossClamped =
      xCrossImage.clamp(Vector3ui(0,0,0), xSize - Vector3ui(1,1,1));

  // Update the crosshairs position in the global state
  m_Parent->GetDriver()->SetCursorPosition(xCrossClamped);
}

bool OrthogonalSliceCursorNavigationModel::CheckZoomThumbnail(Vector2i xCanvas)
{
  // Check if the event is inside of the thumbnail boundaries
  Vector2i xThumb = m_Parent->GetZoomThumbnailPosition();
  Vector2i sThumb = m_Parent->GetZoomThumbnailSize();
  return (m_Parent->IsThumbnailOn() &&
    xCanvas[0] > xThumb[0] &&
    xCanvas[0] < xThumb[0] + sThumb[0] &&
    xCanvas[1] > xThumb[1] &&
          xCanvas[1] < xThumb[1] + sThumb[1]);
}

void OrthogonalSliceCursorNavigationModel
::ProcessThumbnailPanGesture(Vector2i uvOffset)
{
  // Figure out how this movement translates to space units
  Vector2f xOffset(
    uvOffset[0] / m_Parent->GetThumbnailZoom(),
    uvOffset[1] / m_Parent->GetThumbnailZoom());

  // Add to the position
  m_Parent->SetViewPosition(m_StartViewPosition - xOffset);
}

