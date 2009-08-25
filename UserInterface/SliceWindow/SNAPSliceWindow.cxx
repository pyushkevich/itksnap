/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: SNAPSliceWindow.cxx,v $
  Language:  C++
  Date:      $Date: 2009/08/25 21:38:16 $
  Version:   $Revision: 1.7 $
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
// Borland compiler is very lazy so we need to instantiate the template
//  by hand 
#if defined(__BORLANDC__)
#include <itkCommand.h>
#include <itkSmartPointer.h>
typedef itk::SmartPointer<itk::Command> SNAPSliceWindowDummyCommandSPType;
#include "SNAPBorlandDummyTypes.h"
#endif

#include "SNAPSliceWindow.h"

#include "BubblesInteractionMode.h"
#include "GlobalState.h"
#include "SNAPImageData.h"
#include "SNAPAppearanceSettings.h"
#include "OpenGLSliceTexture.h"
#include "UserInterfaceBase.h"

SNAPSliceWindow
::SNAPSliceWindow(int id, UserInterfaceBase *parentUI, FLTKCanvas *canvas) 
: GenericSliceWindow(id, parentUI, canvas)
{
  // Initialize the texture for displaying the speed image
  m_SpeedTexture = new OpenGLSliceTexture;
  m_SpeedTexture->SetGlComponents(4);
  m_SpeedTexture->SetGlFormat(GL_RGBA);
  m_SpeedTexture->SetGlType(GL_UNSIGNED_BYTE);

  // Initialize the snake image texture
  m_SnakeTexture = new OpenGLSliceTexture;
  m_SnakeTexture->SetGlComponents(4);
  m_SnakeTexture->SetGlFormat(GL_RGBA);
  m_SnakeTexture->SetGlType(GL_UNSIGNED_BYTE);

  // Initialize the overlay texture
  m_OverlayTexture = new OpenGLSliceTexture;
  m_OverlayTexture->SetGlComponents(4);
  m_OverlayTexture->SetGlFormat(GL_RGBA);
  m_OverlayTexture->SetGlType(GL_UNSIGNED_BYTE);

  // Initialize the bubbles interaction mode
  m_BubblesMode = new BubblesInteractionMode(this);

  // Register the interaction modes
  m_BubblesMode->Register();
}

SNAPSliceWindow
::~SNAPSliceWindow()
{
  delete m_SpeedTexture;
}

void 
SNAPSliceWindow
::InitializeSlice(GenericImageData *imageData)
{
  // Call parent's init
  GenericSliceWindow::InitializeSlice(imageData);
  if(!imageData->IsMainLoaded())
	return;

  // Set the pointer to the SNAP data
  m_ImageData = dynamic_cast<SNAPImageData *>(imageData);

  // Test that the cast worked
  assert(m_ImageData);
}

void SNAPSliceWindow
::DrawMainTexture()
{
  // The preprocessing image is shown when the corresponding flag is set and 
  // the display mode is not set to overlay
  if(m_GlobalState->GetShowSpeed() && !m_GlobalState->GetSpeedViewZero())
    {
    // We should have a slice to draw
    assert(m_ImageData->IsSpeedLoaded() && m_ImageSliceIndex >= 0);

    // Get the color to use for background
    Vector3d clrBackground = m_ThumbnailIsDrawing
      ? m_ParentUI->GetAppearanceSettings()->GetUIElement(
          SNAPAppearanceSettings::ZOOM_THUMBNAIL).NormalColor
      : Vector3d(1.0);

    // Make sure the correct image is pointed to
    m_SpeedTexture->SetImage(m_ImageData->GetSpeed()->GetDisplaySlice(m_Id).GetPointer());

    // Paint the grey texture
    m_SpeedTexture->Draw(clrBackground);
    }
  else
    {
    GenericSliceWindow::DrawMainTexture();
    }
}

void SNAPSliceWindow
::DrawSegmentationTexture()
{
  // First check if we are in overlay drawing mode
  if(m_GlobalState->GetSpeedViewZero())
    {
    // We should have a slice to draw
    assert(m_ImageData->IsSpeedLoaded() && m_ImageSliceIndex >= 0);
    
    // Make sure that the right image is there
    m_OverlayTexture->SetImage(m_ImageData->GetSpeed()->GetOverlaySlice(m_Id));

    // Paint the grey texture
    m_OverlayTexture->DrawTransparent(m_GlobalState->GetSegmentationAlpha());
    }

  // Otherwize draw the snake
  else if(m_GlobalState->GetSnakeActive())
    {
    // We should have a slice to draw
    assert(m_ImageData->IsSnakeLoaded() && m_ImageSliceIndex >= 0);

    // Make sure that the right image is there
    m_SnakeTexture->SetImage(m_ImageData->GetSnake()->GetDisplaySlice(m_Id).GetPointer());

    // Paint the grey texture
    m_SnakeTexture->DrawTransparent(m_GlobalState->GetSegmentationAlpha());
    }

  // Otherwize use the parent's method
  else
    {
    // Call the parent's method    
    GenericSliceWindow::DrawSegmentationTexture();
    }
}

void SNAPSliceWindow
::DrawOverlays()
{
  // Call the parent's code
  GenericSliceWindow::DrawOverlays();

  // Draw the bubbles
  if(!m_ThumbnailIsDrawing)
    m_BubblesMode->OnDraw();
}

