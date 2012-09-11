/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: GenericSliceWindow.cxx,v $
  Language:  C++
  Date:      $Date: 2010/10/13 16:59:25 $
  Version:   $Revision: 1.36 $
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
#include "GenericSliceWindow.h"

#include "CrosshairsInteractionMode.h"
#include "GlobalState.h"
#include "IRISException.h"
#include "IRISApplication.h"
#include "IRISImageData.h"
#include "OpenGLSliceTexture.h"
#include "SliceWindowCoordinator.h"
#include "SNAPAppearanceSettings.h"
#include "UserInterfaceBase.h"
#include "ThumbnailInteractionMode.h"
#include "PopupButtonInteractionMode.h"

#include <assert.h>
#include <stdio.h>
#include <cstdlib>
#include <cmath>
#include <iostream>

#include "itkConstantPadImageFilter.h"

using namespace std;

GenericSliceWindow
::GenericSliceWindow(int index, UserInterfaceBase *ui, FLTKCanvas *canvas)
: RecursiveInteractionMode(canvas)
{
  // Copy parent pointers
  m_ParentUI = ui;
  m_Driver = m_ParentUI->GetDriver();
  m_GlobalState = m_Driver->GetGlobalState();    

  // Set the window ID
  m_Id = index;

  // Initialize the interaction modes
  m_NavigationMode = new CrosshairsInteractionMode(this);
  m_ThumbnailMode = new ThumbnailInteractionMode(this);
  m_PopupButtonMode = new PopupButtonInteractionMode(this);

  // The slice is not yet initialized
  m_IsSliceInitialized = false;

  // Initialize the Main image slice texture
  m_MainTexture = new OpenGLSliceTexture(4, GL_RGBA);

  // Initialize the Segmentation slice texture
  m_LabelRGBTexture = new OpenGLSliceTexture(4, GL_RGBA);

  // Initalize the margin
  m_Margin = 2;

  // Initialize the zoom management
  m_ManagedZoom = false;

  // No thumbnail
  m_ThumbnailIsDrawing = false;

  // Allow focus grabbing
  m_Canvas->SetGrabFocusOnEntry(true);

  // Register the sub-interaction modes
  m_NavigationMode->Register();
  m_ThumbnailMode->Register();
  m_PopupButtonMode->Register();

  // We have been registered
  m_IsRegistered = true;
}    

GenericSliceWindow
::~GenericSliceWindow()
{
  // Delete the interaction modes
  delete m_NavigationMode;
  delete m_ThumbnailMode;
  delete m_PopupButtonMode;

  // Delete textures
  delete m_MainTexture;
  delete m_LabelRGBTexture;
}



void 
GenericSliceWindow
::InitializeSlice(GenericImageData *imageData)
{
  // Register should have been called already
  assert(m_IsRegistered);

  // Store the image data pointer
  m_ImageData = imageData;

  // The main image should be loaded
  if (imageData->IsMainLoaded())
    {
    // Initialize the Main image slice texture
    m_MainTexture->SetImage(
      m_ImageData->GetMain()->GetDisplaySlice(m_Id).GetPointer());
    }
  else
    {
    // If not
    m_IsSliceInitialized = false;
    ClearInteractionStack();
    return;
    }

  // Initialize the segmentation slice texture
  if (imageData->IsSegmentationLoaded())
    {
    m_LabelRGBTexture->SetImage(
      m_ImageData->GetSegmentation()->GetDisplaySlice(m_Id).GetPointer());
    }

  // Initialize overlay slice
  InitializeOverlaySlice(imageData);

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
  for(unsigned int i = 0;i < 3;i++) 
    {    
    // Get the direction in image space that corresponds to the i'th
    // direction in slice space
    m_ImageAxes[i] = m_DisplayToImageTransform.GetCoordinateIndexZeroBased(i);

    // Record the size and scaling of the slice
    m_SliceSize[i] = imageSizeInImageSpace[m_ImageAxes[i]];
    m_SliceSpacing[i] = imageScalingInImageSpace[m_ImageAxes[i]]; // TODO: Reverse sign by orientation?
    }

  // No information about the current slice available yet
  m_ImageSliceIndex = -1;
  m_DisplayAxisPosition = 0.0f;

  // We have been initialized
  m_IsSliceInitialized = true;

  // If the is no current interaction mode, enter the crosshairs mode
  if(GetInteractionModeCount() == 0)
    PushInteractionMode(m_NavigationMode);

  // setup default view - fit to window
  ResetViewToFit();
}

void 
GenericSliceWindow
::InitializeOverlaySlice(GenericImageData *imageData)
{
  // Register should have been called already
  assert(m_IsRegistered);

  // The main image should be loaded
  if (!imageData->IsMainLoaded())
    {
    // If not
    m_IsSliceInitialized = false;
    ClearInteractionStack();
    return;
    }

  // Store the image data pointer
  m_ImageData = imageData;

  // Clear the overlay texture list
  while (m_OverlayTextureList.size() > 0)
    {
    delete m_OverlayTextureList.front();
    m_OverlayTextureList.pop_front();
    }

  // Initialize the overlay slice texture
  if (imageData->IsOverlayLoaded())
    {
    std::list<ImageWrapperBase *>::iterator it = m_ImageData->GetOverlays()->begin();
    while (it != m_ImageData->GetOverlays()->end())
      {
      OpenGLSliceTexture *texture = new OpenGLSliceTexture(4, GL_RGBA);
      texture->SetImage((*it)->GetDisplaySlice(m_Id).GetPointer());
	    m_OverlayTextureList.push_back(texture);
      it++;
    	 }
    }

}

void
GenericSliceWindow
::ComputeOptimalZoom()
{
  // Should be fully initialized
  assert(m_IsRegistered && m_IsSliceInitialized);

  // Compute slice size in spatial coordinates
  Vector2f worldSize(
    m_SliceSize[0] * m_SliceSpacing[0],
    m_SliceSize[1] * m_SliceSpacing[1]);

  // Set the view position (position of the center of the image?)
  m_ViewPosition = worldSize * 0.5f;

  // Reduce the width and height of the slice by the margin
  Vector2i szCanvas = 
    Vector2i(m_Canvas->w(),m_Canvas->h()) - Vector2i(2 * m_Margin);
  
  // Compute the ratios of window size to slice size
  Vector2f ratios(
    szCanvas(0) / worldSize(0),
    szCanvas(1) / worldSize(1));

  // The zoom factor is the bigger of these ratios, the number of pixels 
  // on the screen per millimeter in world space
  m_OptimalZoom = ratios.min_value();
}

Vector2i 
GenericSliceWindow
::GetOptimalCanvasSize()
{
  // Compute slice size in spatial coordinates
  Vector2i optSize(
    (int) ceil(m_SliceSize[0] * m_SliceSpacing[0] * m_ViewZoom + 2 * m_Margin),
    (int) ceil(m_SliceSize[1] * m_SliceSpacing[1] * m_ViewZoom + 2 * m_Margin));

  return optSize;
}

void
GenericSliceWindow
::ResetViewToFit()
{
  // Should be fully initialized
  assert(m_IsRegistered && m_IsSliceInitialized);

  // Compute slice size in spatial coordinates
  ComputeOptimalZoom();

  // The zoom factor is the bigger of these ratios, the number of pixels 
  // on the screen per millimeter in world space
  m_ViewZoom = m_OptimalZoom;

  // Cause a redraw of the window
  m_Canvas->redraw();
}

Vector3f 
GenericSliceWindow
::MapSliceToImage(const Vector3f &xSlice) 
{
  assert(m_IsSliceInitialized);

  // Get corresponding position in display space
  return m_DisplayToImageTransform.TransformPoint(xSlice);
}

/**
 * Map a point in image coordinates to slice coordinates
 */
Vector3f 
GenericSliceWindow
::MapImageToSlice(const Vector3f &xImage) 
{
  assert(m_IsSliceInitialized);

  // Get corresponding position in display space
  return  m_ImageToDisplayTransform.TransformPoint(xImage);
}

Vector2f 
GenericSliceWindow
::MapSliceToWindow(const Vector3f &xSlice)
{
  assert(m_IsSliceInitialized);

  // Adjust the slice coordinates by the scaling amounts
  Vector2f uvScaled(
    xSlice(0) * m_SliceSpacing(0),xSlice(1) * m_SliceSpacing(1));

  // Compute the window coordinates
  Vector2f uvWindow = 
    m_ViewZoom * (uvScaled - m_ViewPosition) + 
    Vector2f(0.5f * m_Canvas->w(),0.5f * m_Canvas->h());
  
  // That's it, the projection matrix is set up in the scaled-slice coordinates
  return uvWindow;
}

Vector3f 
GenericSliceWindow
::MapWindowToSlice(const Vector2f &uvWindow)
{
  assert(m_IsSliceInitialized && m_ViewZoom > 0);

  // Compute the scaled slice coordinates
  Vector2f winCenter(0.5f * m_Canvas->w(),0.5f * m_Canvas->h());
  Vector2f uvScaled = 
    m_ViewPosition + (uvWindow - winCenter) / m_ViewZoom;
  
  // The window coordinates are already in the scaled-slice units
  Vector3f uvSlice(
    uvScaled(0) / m_SliceSpacing(0),
    uvScaled(1) / m_SliceSpacing(1),
    m_DisplayAxisPosition);

  // Return this vector
  return uvSlice;
}

Vector2f 
GenericSliceWindow
::MapSliceToPhysicalWindow(const Vector3f &xSlice)
{
  assert(m_IsSliceInitialized);

  // Compute the physical window coordinates
  Vector2f uvPhysical;
  uvPhysical[0] = xSlice[0] * m_SliceSpacing[0];
  uvPhysical[1] = xSlice[1] * m_SliceSpacing[1];

  return uvPhysical;
}

void
GenericSliceWindow
::OnDraw()
{
  // Set up the projection if necessary
  if(!m_Canvas->valid()) 
  {
    // The window will have coordinates (0,0) to (w,h), i.e. the same as the window
    // coordinates.
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0.0,m_Canvas->w(),0.0,m_Canvas->h());
    glViewport(0,0,m_Canvas->w(),m_Canvas->h());

    // Establish the model view matrix
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // Compute the optimal zoom
    if(m_IsRegistered && m_IsSliceInitialized)
      {
      // If the zoom is set to fit, maintain the fit, otherwise, maintain the 
      // optimal zoom level
      if(!m_ManagedZoom && m_ViewZoom == m_OptimalZoom)
        {
        ComputeOptimalZoom();
        m_ViewZoom = m_OptimalZoom;
        }
      else
        {
        ComputeOptimalZoom();
        }
      }
  }

  // Get the properties for the background color
  Vector3d clrBack = 
    m_ParentUI->GetAppearanceSettings()->GetUIElement(
      SNAPAppearanceSettings::BACKGROUND_2D).NormalColor;

  // Clear the display, using a blue shade when under focus
  glClearColor(clrBack[0],clrBack[1],clrBack[2],1.0);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);    

  // Slice should be initialized before display
  if (!m_IsSliceInitialized) 
    return;

  // Compute the position of the cross-hairs in display space
  Vector3ui cursorImageSpace = m_Driver->GetCursorPosition();
  Vector3f cursorDisplaySpace = 
    m_ImageToDisplayTransform.TransformPoint(
      to_float(cursorImageSpace) + Vector3f(0.5f));

  // Get the current slice number
  m_ImageSliceIndex = cursorImageSpace[m_ImageAxes[2]];
  m_DisplayAxisPosition = cursorDisplaySpace[2];

  // Set up lighting attributes
  glPushAttrib(GL_LIGHTING_BIT | GL_DEPTH_BUFFER_BIT | 
               GL_PIXEL_MODE_BIT | GL_TEXTURE_BIT );  
  
  glDisable(GL_LIGHTING);

  // glDisable(GL_DEPTH);

  // Prepare for overlay drawing.  The model view is set up to correspond
  // to pixel coordinates of the slice
  glPushMatrix();
  glTranslated(0.5 * m_Canvas->w(),0.5 * m_Canvas->h(),0.0);
  glScalef(m_ViewZoom,m_ViewZoom,1.0);
  glTranslated(-m_ViewPosition(0),-m_ViewPosition(1),0.0);
  glScalef(m_SliceSpacing[0],m_SliceSpacing[1],1.0);
  
  // Make the grey and segmentation image textures up-to-date
  DrawMainTexture();
  DrawOverlayTexture();
  DrawSegmentationTexture();

  // Draw the overlays
  if(m_ParentUI->GetAppearanceSettings()->GetOverallVisibility())
    {
    DrawOverlays();

    // Draw the zoom locator
    if(IsThumbnailOn())
      DrawThumbnail();
    }


  // Clean up the GL state
  glPopMatrix();
  glPopAttrib();

  // Draw the little popup button
  // m_PopupButtonMode->OnDraw();

  // Display!
  glFlush();
}

void 
GenericSliceWindow
::DrawMainTexture() 
{
  // We should have a slice to return
  assert(m_ImageSliceIndex >= 0);

  if (m_ImageData->IsMainLoaded())
    {
    // Get the color to use for background
    Vector3d clrBackground = m_ThumbnailIsDrawing
      ? m_ParentUI->GetAppearanceSettings()->GetUIElement(
          SNAPAppearanceSettings::ZOOM_THUMBNAIL).NormalColor
      : Vector3d(1.0);

    // Set the interpolation mode to current default
    m_MainTexture->SetInterpolation(
      m_ParentUI->GetAppearanceSettings()->GetGreyInterpolationMode()
      == SNAPAppearanceSettings::LINEAR ? GL_LINEAR : GL_NEAREST);

    // Paint the grey texture with color as background
    m_MainTexture->Draw(clrBackground);
    }
}

void 
GenericSliceWindow
::DrawOverlayTexture() 
{
  // We should have a slice to return
  assert(m_ImageSliceIndex >= 0);

  // Get the color to use for background
  Vector3d clrBackground = m_ThumbnailIsDrawing
    ? m_ParentUI->GetAppearanceSettings()->GetUIElement(
        SNAPAppearanceSettings::ZOOM_THUMBNAIL).NormalColor
    : Vector3d(1.0);

  OverlayTextureIterator textureIt = m_OverlayTextureList.begin();
  std::list<ImageWrapperBase *>::iterator wrapperIt = m_ImageData->GetOverlays()->begin();
  while (textureIt != m_OverlayTextureList.end())
    {
    // Set the interpolation mode to current default
    OpenGLSliceTexture *texture = *textureIt;
    ImageWrapperBase *wrapper = *wrapperIt;
    texture->SetInterpolation(
      m_ParentUI->GetAppearanceSettings()->GetGreyInterpolationMode()
      == SNAPAppearanceSettings::LINEAR ? GL_LINEAR : GL_NEAREST);
    texture->DrawTransparent(wrapper->GetAlpha());
    textureIt++;
    wrapperIt++;
    }
}

void 
GenericSliceWindow
::DrawSegmentationTexture() 
{
  // We should have a slice to return
  assert(m_ImageSliceIndex >= 0);

  if (m_ImageData->IsSegmentationLoaded())
    {
    m_LabelRGBTexture->DrawTransparent(m_GlobalState->GetSegmentationAlpha());
    }
}

void
GenericSliceWindow
::DrawThumbnail()
{
  // Get the thumbnail appearance properties
  const SNAPAppearanceSettings::Element &elt = 
    m_ParentUI->GetAppearanceSettings()->GetUIElement(
      SNAPAppearanceSettings::ZOOM_THUMBNAIL);

  // If thumbnail is not to be drawn, exit
  if(!elt.Visible) return;

  // Indicate the fact that we are currently drawing in thumbnail mode
  m_ThumbnailIsDrawing = true;  
  
  // The dimensions of the canvas on which we are working, in pixels
  Vector2i xCanvas( m_Canvas->w(), m_Canvas->h() );

  // The thumbnail will occupy a specified fraction of the target canvas
  float xFraction = 0.01f * 
    m_ParentUI->GetAppearanceSettings()->GetZoomThumbnailSizeInPercent();

  // But it must not exceed a predefined size in pixels in either dimension
  float xThumbMax = 
    m_ParentUI->GetAppearanceSettings()->GetZoomThumbnailMaximumSize();

  // Recompute the fraction based on maximum size restriction
  float xNewFraction = xFraction;
  if( xCanvas[0] * xNewFraction > xThumbMax )
    xNewFraction = xThumbMax * 1.0f / xCanvas[0];
  if( xCanvas[1] * xNewFraction > xThumbMax )
    xNewFraction = xThumbMax * 1.0f / xCanvas[1];

  // Draw the little version of the image in the corner of the window
  double w = m_SliceSize[0];
  double h = m_SliceSize[1];

  // Set the position and size of the thumbnail, in pixels
  m_ThumbnailZoom = xNewFraction * m_OptimalZoom;
  m_ThumbnailPosition.fill(5);
  m_ThumbnailSize[0] = (int)(m_SliceSize[0] * m_SliceSpacing[0] * m_ThumbnailZoom);
  m_ThumbnailSize[1] = (int)(m_SliceSize[1] * m_SliceSpacing[1] * m_ThumbnailZoom);
  
  glPushMatrix();
  glLoadIdentity();
  glTranslated((double) m_ThumbnailPosition[0], (double) m_ThumbnailPosition[1], 0.0);
  glScaled(m_ThumbnailZoom, m_ThumbnailZoom, 1.0);

  glPushMatrix();
  glScalef(m_SliceSpacing[0],m_SliceSpacing[1],1.0);
  // glTranslated(w * 0.1111, h * 0.1111, 0.0);

  // Draw the Main image (the background will be picked automatically)
  DrawMainTexture();
 
  // Draw the crosshairs and stuff
  DrawOverlays();

  // Apply the line settings
  SNAPAppearanceSettings::ApplyUIElementLineSettings(elt);

  // Draw the line around the image
  glColor3dv(elt.NormalColor.data_block());
  glBegin(GL_LINE_LOOP);
  glVertex2d(0,0);
  glVertex2d(0,h);
  glVertex2d(w,h);
  glVertex2d(w,0);
  glEnd();

  // Draw a box representing the current zoom level
  glPopMatrix();
  glTranslated(m_ViewPosition[0],m_ViewPosition[1],0.0);
  w = m_Canvas->w() * 0.5 / m_ViewZoom;
  h = m_Canvas->h() * 0.5 / m_ViewZoom;

  glColor3dv(elt.ActiveColor.data_block());
  glBegin(GL_LINE_LOOP);
  glVertex2d(-w,-h);
  glVertex2d(-w, h);
  glVertex2d( w, h);
  glVertex2d( w,-h);
  glEnd();

  glPopMatrix();

  // Indicate the fact that we are not drawing in thumbnail mode
  m_ThumbnailIsDrawing = false;  
}

void 
GenericSliceWindow
::DrawOverlays() 
{
  if(!m_ThumbnailIsDrawing) 
    {
    // Display the letters (RAI)
    DrawOrientationLabels();

    // Display the rulers
    DrawRulers();

    // Draw the zoom mode (does't really draw, repaints a UI widget)
    m_NavigationMode->OnDraw();
    }

  m_NavigationMode->OnDraw();
}

void 
GenericSliceWindow
::DrawOrientationLabels()
{
  // The letter labels
  static const char *letters[3][2] = {{"R","L"},{"A","P"},{"I","S"}};
  const char *labels[2][2];

  // Get the properties for the labels
  const SNAPAppearanceSettings::Element &elt = 
    m_ParentUI->GetAppearanceSettings()->GetUIElement(
      SNAPAppearanceSettings::MARKERS);

  // Leave if the labels are disabled
  if(!elt.Visible) return;

  // Repeat for X and Y directions
  for(unsigned int i=0;i<2;i++) 
    {
    // Which axis are we on in anatomy space?
    unsigned int anatomyAxis = 
      m_DisplayToAnatomyTransform.GetCoordinateIndexZeroBased(i);

    // Which direction is the axis facing (returns -1 or 1)
    unsigned int anatomyAxisDirection = 
      m_DisplayToAnatomyTransform.GetCoordinateOrientation(i);

    // Map the direction onto 0 or 1
    unsigned int letterIndex = (1 + anatomyAxisDirection) >> 1;

    // Compute the two labels for this axis
    labels[i][0] = letters[anatomyAxis][1-letterIndex];
    labels[i][1] = letters[anatomyAxis][letterIndex];
    }

  glPushAttrib(GL_COLOR_BUFFER_BIT | GL_CURRENT_BIT | GL_DEPTH_BUFFER_BIT);
  glPushMatrix();
  glLoadIdentity();

  if(elt.AlphaBlending)
    {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }

  glColor4d( elt.NormalColor[0], elt.NormalColor[1], elt.NormalColor[2], 1.0 );

  gl_font(FL_COURIER_BOLD, elt.FontSize);
  int offset = 4 + elt.FontSize * 2;
  int margin = elt.FontSize / 3;
  int w = m_Canvas->w(), h = m_Canvas->h();

  gl_draw(labels[0][0],margin,0,offset,h,FL_ALIGN_LEFT);
  gl_draw(labels[0][1],w - (offset+margin),0,offset,h,FL_ALIGN_RIGHT);
  gl_draw(labels[1][0],0,0,w,offset,FL_ALIGN_BOTTOM);
  gl_draw(labels[1][1],0,h - (offset+1),w,offset,FL_ALIGN_TOP);


  glPopMatrix();
  glPopAttrib();
}

void
GenericSliceWindow
::DrawRulers()
{
  // Get the properties for the labels
  const SNAPAppearanceSettings::Element &elt = 
    m_ParentUI->GetAppearanceSettings()->GetUIElement(
      SNAPAppearanceSettings::RULER);

  // Leave if the labels are disabled
  if(!elt.Visible) return;  

  glPushAttrib(GL_COLOR_BUFFER_BIT | GL_CURRENT_BIT | GL_DEPTH_BUFFER_BIT);
  glPushMatrix();
  glLoadIdentity();
  
  SNAPAppearanceSettings::ApplyUIElementLineSettings(elt);
  glColor4d( elt.NormalColor[0], elt.NormalColor[1], elt.NormalColor[2], 1.0 );
  gl_font(FL_HELVETICA, elt.FontSize);

  // Pick the scale of the ruler 
  int w = m_Canvas->w(), h = m_Canvas->h();

  // The ruler bar should be as large as possible but less than one half
  // of the screen width (not to go over the markers)
  double maxw = 0.5 * w - 20.0;
  maxw = maxw < 5 ? 5 : maxw;

  double scale = 1.0;
  while(m_ViewZoom * scale > maxw) scale /= 10.0;
  while(m_ViewZoom * scale < 0.1 * maxw) scale *= 10.0;

  // Draw a zoom bar
  double bw = scale * m_ViewZoom;
  glBegin(GL_LINES);
  glVertex2d(5,h - 5);
  glVertex2d(5,h - 20);
  glVertex2d(5,h - 10);
  glVertex2d(5 + bw,h - 10);
  glVertex2d(5 + bw,h - 5);
  glVertex2d(5 + bw,h - 20);
  glEnd();

  // Based on the log of the scale, determine the unit
  string unit = "mm";
  if(scale >= 10 && scale < 1000)
    { unit = "cm"; scale /= 10; }
  else if(scale >= 1000)
    { unit = "m"; scale /= 1000; }
  else if(scale < 1 && scale > 0.001)
    { unit = "\xb5m"; scale *= 1000; }
  else if(scale < 0.001)
    { unit = "nm"; scale *= 1000000; }

  ostringstream oss;
  oss << scale << " " << unit;

  // See if we can squeeze the label under the ruler
  if(bw > elt.FontSize * 4)
    gl_draw(oss.str().c_str(), 10, h - 30, (int) bw, 20, FL_ALIGN_TOP);
  else
    gl_draw(oss.str().c_str(), (int) bw+10, h - 20, (int) bw + elt.FontSize * 4+10, 15, FL_ALIGN_LEFT);


  glPopMatrix();
  glPopAttrib();
}

void 
GenericSliceWindow
::EnterInteractionMode(InteractionMode *mode)
{
  // Empty the stack
  ClearInteractionStack();

  // Push the crosshairs mode - last to get events
  if(mode != m_NavigationMode)
    {
    // Navigation mode serves as a fallback, allowing crosshair editing with middle button
    m_NavigationMode->SetInteractionStyle(
      CrosshairsInteractionMode::ANY,
      CrosshairsInteractionMode::NONE, 
      CrosshairsInteractionMode::NONE);

    PushInteractionMode(m_NavigationMode);
    }

  // Push the input mode
  PushInteractionMode(mode);

  // Push the thumbnail mode
  PushInteractionMode(m_ThumbnailMode);

  // Push the popup mode
  // PushInteractionMode(m_PopupButtonMode);
}
  
void 
GenericSliceWindow
::EnterCrosshairsMode()
{
  m_NavigationMode->SetInteractionStyle(
    CrosshairsInteractionMode::LEFT,
    CrosshairsInteractionMode::RIGHT, 
    CrosshairsInteractionMode::MIDDLE);

  EnterInteractionMode(m_NavigationMode);
}

void 
GenericSliceWindow
::EnterZoomPanMode()
{
  m_NavigationMode->SetInteractionStyle(
    CrosshairsInteractionMode::MIDDLE,
    CrosshairsInteractionMode::RIGHT, 
    CrosshairsInteractionMode::LEFT);

  EnterInteractionMode(m_NavigationMode);
}

void
GenericSliceWindow
::ResetViewPosition()
{
  // Compute slice size in spatial coordinates
  Vector2f worldSize(
    m_SliceSize[0] * m_SliceSpacing[0],
    m_SliceSize[1] * m_SliceSpacing[1]);

  // Set the view position (position of the center of the image?)
  m_ViewPosition = worldSize * 0.5f;

  m_Canvas->redraw();
}

void
GenericSliceWindow
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
GenericSliceWindow
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

void 
GenericSliceWindow
::SetViewZoom(float newZoom)
{
  // Update the zoom
  m_ViewZoom = newZoom;  

  // Repaint the window
  m_Canvas->redraw();
}

GenericSliceWindow *
GenericSliceWindow
::GetNextSliceWindow()
{
  SliceWindowCoordinator *swc = m_ParentUI->GetSliceCoordinator();
  return swc->GetWindow( (m_Id+1) % 3);
}

bool
GenericSliceWindow
::IsThumbnailOn()
{ 
  return m_ParentUI->GetAppearanceSettings()->GetFlagDisplayZoomThumbnail() 
    && m_ViewZoom > m_OptimalZoom; 
}

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

