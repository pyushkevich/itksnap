/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: GenericSliceWindow.cxx,v $
  Language:  C++
  Date:      $Date: 2009/06/09 05:49:45 $
  Version:   $Revision: 1.18 $
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
#include "IRISApplication.h"
#include "IRISImageData.h"
#include "OpenGLSliceTexture.h"
#include "SliceWindowCoordinator.h"
#include "SNAPAppearanceSettings.h"
#include "UserInterfaceBase.h"
#include "ZoomPanInteractionMode.h"
#include "ThumbnailInteractionMode.h"

#include <assert.h>
#include <stdio.h>
#include <cstdlib>
#include <cmath>
#include <iostream>

#include "itkConstantPadImageFilter.h"

using namespace itk;
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
  m_CrosshairsMode = new CrosshairsInteractionMode(this);
  m_ZoomPanMode = new ZoomPanInteractionMode(this);
  m_ThumbnailMode = new ThumbnailInteractionMode(this);

  // The slice is not yet initialized
  m_IsSliceInitialized = false;

  // Initialize the Grey slice texture (not default)
  m_GreyTexture = new GreyTextureType;
  m_GreyTexture->SetGlComponents(4);
  m_GreyTexture->SetGlFormat(GL_RGBA);

  // Initialize the Grey overlay slice texture (not default)
  m_GreyOverlayTexture = new GreyTextureType;
  m_GreyOverlayTexture->SetGlComponents(4);
  m_GreyOverlayTexture->SetGlFormat(GL_RGBA);

  // Initialize the RGB slice texture (not default)
  m_RGBTexture = new RGBTextureType;
  m_RGBTexture->SetGlComponents(4);
  m_RGBTexture->SetGlFormat(GL_RGBA);
  
  // Initialize the Segmentation slice texture (not default)
  m_LabelRGBTexture = new RGBTextureType;
  m_LabelRGBTexture->SetGlComponents(4);
  m_LabelRGBTexture->SetGlFormat(GL_RGBA);

  // Initialize the color index texture on systems that support it (?)
//  m_LabelColorIndexTexture = new LabelTextureType;
//  m_LabelColorIndexTexture->SetGlComponents(1);
//  m_LabelColorIndexTexture->SetGlFormat(GL_COLOR_INDEX);
//  m_LabelColorIndexTexture->SetGlType(GL_UNSIGNED_SHORT);

  // Initalize the margin
  m_Margin = 2;

  // Initialize the zoom management
  m_ManagedZoom = false;

  // No thumbnail
  m_ThumbnailIsDrawing = false;

  // Allow focus grabbing
  m_Canvas->SetGrabFocusOnEntry(true);

  // Register the sub-interaction modes
  m_CrosshairsMode->Register();
  m_ZoomPanMode->Register();
  m_ThumbnailMode->Register();

  // We have been registered
  m_IsRegistered = true;
}    

GenericSliceWindow
::~GenericSliceWindow()
{
  // Delete the interaction modes
  delete m_CrosshairsMode;
  delete m_ZoomPanMode;
  delete m_ThumbnailMode;

  // Delete textures
  delete m_GreyTexture;
  delete m_GreyOverlayTexture;
  delete m_RGBTexture;
  delete m_LabelRGBTexture;
//  delete m_LabelColorIndexTexture;
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
  if (!imageData->IsMainLoaded())
    {
    // If not
    m_IsSliceInitialized = false;
    ClearInteractionStack();
    return;
    }

  // Initialize the grey slice texture
  if (imageData->IsGreyLoaded())
    {
    m_GreyTexture->SetImage(
      m_ImageData->GetGrey()->GetDisplaySlice(m_Id));
    }

  // Initialize the grey overlay slice texture
  if (imageData->IsGreyOverlayLoaded())
    {
    m_GreyOverlayTexture->SetImage(
      m_ImageData->GetGreyOverlay()->GetDisplaySlice(m_Id));
    }

  // Initialize the RGB slice texture
  if (imageData->IsRGBLoaded())
    {
    m_RGBTexture->SetImage(
      m_ImageData->GetRGB()->GetDisplaySlice(m_Id));
    }
  
  // Initialize the segmentation slice texture
  if (imageData->IsSegmentationLoaded())
    {
    m_LabelRGBTexture->SetImage(
      m_ImageData->GetSegmentation()->GetDisplaySlice(m_Id));
    }

//  m_LabelColorIndexTexture->SetImage(
//    m_ImageData->GetSegmentation()->GetSlice(m_Id));

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
    PushInteractionMode(m_CrosshairsMode);
  
  // setup default view - fit to window
  ResetViewToFit();
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
  DrawGreyTexture();
  DrawRGBTexture();
  DrawGreyOverlayTexture();
  DrawSegmentationTexture();

  // Draw the overlays
  DrawOverlays();

  // Draw the zoom locator
  if(IsThumbnailOn())
    DrawThumbnail();

  // Clean up the GL state
  glPopMatrix();
  glPopAttrib();

  // Display!
  glFlush();
}

void 
GenericSliceWindow
::DrawGreyTexture() 
{
  // We should have a slice to return
  assert(m_ImageSliceIndex >= 0);

  // Get the color to use for background
  Vector3d clrBackground = m_ThumbnailIsDrawing
    ? m_ParentUI->GetAppearanceSettings()->GetUIElement(
        SNAPAppearanceSettings::ZOOM_THUMBNAIL).NormalColor
    : Vector3d(1.0);

  // Set the interpolation mode to current default
  m_GreyTexture->SetInterpolation(
    m_ParentUI->GetAppearanceSettings()->GetGreyInterpolationMode()
    == SNAPAppearanceSettings::LINEAR ? GL_LINEAR : GL_NEAREST);
  
  if (m_ImageData->IsGreyLoaded())
    {
    // Paint the grey texture with color as background
    m_GreyTexture->Draw(clrBackground);
    }
}

void 
GenericSliceWindow
::DrawGreyOverlayTexture() 
{
  // We should have a slice to return
  assert(m_ImageSliceIndex >= 0);

  // Get the color to use for background
  Vector3d clrBackground = m_ThumbnailIsDrawing
    ? m_ParentUI->GetAppearanceSettings()->GetUIElement(
        SNAPAppearanceSettings::ZOOM_THUMBNAIL).NormalColor
    : Vector3d(1.0);

  // Set the interpolation mode to current default
  m_GreyOverlayTexture->SetInterpolation(
    m_ParentUI->GetAppearanceSettings()->GetGreyInterpolationMode()
    == SNAPAppearanceSettings::LINEAR ? GL_LINEAR : GL_NEAREST);

  if (m_ImageData->IsGreyOverlayLoaded())
    {
    // Paint the grey texture with color as background
    m_GreyOverlayTexture->DrawTransparent(128);
    }
}

void 
GenericSliceWindow
::DrawRGBTexture() 
{
  // We should have a slice to return
  assert(m_ImageSliceIndex >= 0);

  // Set the interpolation mode to current default
  m_RGBTexture->SetInterpolation(
    m_ParentUI->GetAppearanceSettings()->GetGreyInterpolationMode()
    == SNAPAppearanceSettings::LINEAR ? GL_LINEAR : GL_NEAREST);

  if (m_ImageData->IsRGBLoaded())
    {
    // Update the texture memory with RGB layer
    m_RGBTexture->DrawTransparent(m_GlobalState->GetRGBAlpha());
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

  // Draw the grey scale image (the background will be picked automatically)
  DrawGreyTexture();
  DrawRGBTexture();
 
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
    m_ZoomPanMode->OnDraw();
    }

  // Draw the crosshairs
  m_CrosshairsMode->OnDraw(); 
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
  PushInteractionMode(m_CrosshairsMode);

  // Push the input mode
  if(mode != m_CrosshairsMode)
    PushInteractionMode(mode);

  // Push the thumbnail mode
  PushInteractionMode(m_ThumbnailMode);
}
  
void 
GenericSliceWindow
::EnterCrosshairsMode()
{
  EnterInteractionMode(m_CrosshairsMode);
}

void 
GenericSliceWindow
::EnterZoomPanMode()
{
  EnterInteractionMode(m_ZoomPanMode);
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
::SetViewZoom(float newZoom)
{
  // Update the zoom
  m_ViewZoom = newZoom;  
  // cout << m_Id << " : " << newZoom 

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

