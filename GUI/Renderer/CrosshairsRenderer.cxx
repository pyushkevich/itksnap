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
                                            ImageWrapperBase         *base_layer,
                                            const SubViewport        &vp)
{
  SNAPAppearanceSettings *as = m_Model->GetParentUI()->GetAppearanceSettings();

  // Get the line color, thickness and dash spacing for the crosshairs
  OpenGLAppearanceElement *elt = vp.isThumbnail
                                   ? as->GetUIElement(SNAPAppearanceSettings::CROSSHAIRS_THUMB)
                                   : as->GetUIElement(SNAPAppearanceSettings::CROSSHAIRS);

  // Exit if the crosshars are not drawn
  if(elt->GetVisible() && !vp.isThumbnail)
  {
    // Draw cursor on this image
    // Get the current cursor position
    Vector3ui xCursorInteger = m_Model->GetDriver()->GetCursorPosition();

    // Shift the cursor position by by 0.5 in order to have it appear
    // between voxels
    Vector3d xCursorImage = to_double(xCursorInteger) + Vector3d(0.5);

    // Get the cursor position on the slice
    Vector3d pos = m_Model->MapImageToSlice(xCursorImage);

    // Upper and lober bounds to which the crosshairs are drawn
    Vector2i lower(0);
    Vector2i upper = m_Model->GetSliceSize().extract(2);

    // Apply the color
    context->SetPenAppearance(*as->GetUIElement(SNAPAppearanceSettings::CROSSHAIRS));

    // Draw the four cross-hair pieces
    context->DrawLine(pos[0], pos[1], lower[0], pos[1]);
    context->DrawLine(pos[0], pos[1], upper[0], pos[1]);
    context->DrawLine(pos[0], pos[1], pos[0], lower[1]);
    context->DrawLine(pos[0], pos[1], pos[0], upper[1]);
  }
}
