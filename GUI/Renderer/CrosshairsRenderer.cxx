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
#include "GenericSliceRenderer.h"
#include "IRISException.h"
#include "IRISApplication.h"

CrosshairsRenderer::CrosshairsRenderer()
{
  m_Model = NULL;
}

void CrosshairsRenderer::paintGL()
{
  assert(m_Model);

  GenericSliceModel *parentModel = this->GetParentRenderer()->GetModel();
  SNAPAppearanceSettings *as =
      parentModel->GetParentUI()->GetAppearanceSettings();

  // Get the line color, thickness and dash spacing for the crosshairs
  OpenGLAppearanceElement *elt =
    this->GetParentRenderer()->IsThumbnailDrawing()
    ? as->GetUIElement(SNAPAppearanceSettings::CROSSHAIRS_THUMB)
    : as->GetUIElement(SNAPAppearanceSettings::CROSSHAIRS);

  // Exit if the crosshars are not drawn
  if(!elt->GetVisible()) return;

  // Get the current cursor position
  Vector3ui xCursorInteger = parentModel->GetDriver()->GetCursorPosition();

  // Shift the cursor position by by 0.5 in order to have it appear
  // between voxels
  Vector3f xCursorImage = to_float(xCursorInteger) + Vector3f(0.5f);

  // Get the cursor position on the slice
  Vector3f xCursorSlice = parentModel->MapImageToSlice(xCursorImage);

  // Upper and lober bounds to which the crosshairs are drawn
  Vector2i lower(0);
  Vector2i upper = parentModel->GetSliceSize().extract(2);

  // Set line properties
  glPushAttrib(GL_LINE_BIT | GL_COLOR_BUFFER_BIT);

  // Apply the line properties; thick line is only applied in zoom thumbnail (?)
  elt->ApplyLineSettings();

  // Apply the color
  glColor3dv(elt->GetNormalColor().data_block());

  // Refit matrix so that the lines are centered on the current pixel
  glPushMatrix();
  glTranslated( xCursorSlice(0), xCursorSlice(1), 0.0 );

  // Paint the cross-hairs
  glBegin(GL_LINES);
  glVertex2f(0, 0); glVertex2f(lower(0) - xCursorSlice(0), 0);
  glVertex2f(0, 0); glVertex2f(upper(0) - xCursorSlice(0), 0);
  glVertex2f(0, 0); glVertex2f(0, lower(1) - xCursorSlice(1));
  glVertex2f(0, 0); glVertex2f(0, upper(1) - xCursorSlice(1));
  glEnd();

  glPopMatrix();
  glPopAttrib();
}
