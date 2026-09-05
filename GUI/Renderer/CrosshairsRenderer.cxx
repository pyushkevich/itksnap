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

#include "CrosshairsRenderer.h"
#include "OrthogonalSliceCursorNavigationModel.h"
#include "GenericSliceModel.h"
#include "SNAPAppearanceSettings.h"
#include "GlobalUIModel.h"
#include "IRISApplication.h"
#include <vtkContext2D.h>
#include <vtkContextScene.h>
#include <vtkObjectFactory.h>
#include <vtkTransform2D.h>

void
CrosshairsRenderer::RenderOverTiledLayer(AbstractRenderContext *context,
                                         ImageWrapperBase      *base_layer,
                                         const SubViewport     &vp)
{
  SNAPAppearanceSettings *as = m_Model->GetParentUI()->GetAppearanceSettings();

  // Get the line color, thickness and dash spacing for the crosshairs
  OpenGLAppearanceElement *elt = vp.isThumbnail
                                   ? as->GetUIElement(SNAPAppearanceSettings::CROSSHAIRS_THUMB)
                                   : as->GetUIElement(SNAPAppearanceSettings::CROSSHAIRS);

  // Exit if the crosshars are not drawn
  if (elt->GetVisible() && !vp.isThumbnail)
  {
    // Draw cursor on this image
    // Get the current cursor position
    Vector3i xCursorInteger = m_Model->GetDriver()->GetCursorPosition();

    // Shift the cursor position by by 0.5 in order to have it appear
    // between voxels
    Vector3d xCursorImage = to_double(xCursorInteger) + Vector3d(0.5);

    // Get the cursor position on the slice
    Vector3d pos = m_Model->MapImageToSlice(xCursorImage);

    // Upper and lower bounds to which the crosshairs are drawn
    Vector3i lower_ref(0);
    Vector3i upper_ref = m_Model->GetReferenceSpaceSize();
    Vector3i lower_fe(m_Model->GetFullExtentRegion().GetIndex());
    Vector3i upper_fe(m_Model->GetFullExtentRegion().GetUpperIndex());

    // Check if the slice is inside of the segmentation box
    bool z_in_range = (pos[2] >= lower_ref[2] && pos[2] <= upper_ref[2]);

    // Reference extent rectangle
    double rx0 = lower_ref[0], ry0 = lower_ref[1];
    double rx1 = upper_ref[0], ry1 = upper_ref[1];
    double rw = rx1 - rx0, rh = ry1 - ry0;

    // Full extent rectangle
    double fx0 = lower_fe[0], fy0 = lower_fe[1];
    double fx1 = upper_fe[0], fy1 = upper_fe[1];

    // Draw the crosshair normally, spanning the full extent
    context->SetPenAppearance(*as->GetUIElement(SNAPAppearanceSettings::CROSSHAIRS));
    context->DrawLine(pos[0], pos[1], fx0, pos[1]);
    context->DrawLine(pos[0], pos[1], fx1, pos[1]);
    context->DrawLine(pos[0], pos[1], pos[0], fy0);
    context->DrawLine(pos[0], pos[1], pos[0], fy1);

    // If the reference space differs from the full extent, outline it
    bool ref_differs_from_fe = (rx0 != fx0 || ry0 != fy0 || rx1 != fx1 || ry1 != fy1);
    if(z_in_range && ref_differs_from_fe)
    {
      context->SetPenAppearance(*as->GetUIElement(SNAPAppearanceSettings::REFERENCE_SPACE_BOUNDS));
      context->DrawRect(rx0, ry0, rw, rh);
    }
  }
}
