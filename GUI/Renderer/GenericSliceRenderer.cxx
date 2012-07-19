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

#include "SNAPCommon.h"
#include "GenericSliceRenderer.h"
#include "GenericSliceModel.h"
#include "GlobalUIModel.h"
#include "SNAPAppearanceSettings.h"
#include "GenericImageData.h"
#include "ImageWrapper.h"
#include "IRISApplication.h"
#include "IntensityCurveModel.h"
#include "ColorMapModel.h"
#include "LayerAssociation.txx"

GenericSliceRenderer
::GenericSliceRenderer()
{
  this->m_ThumbnailDrawing = false;
}

void
GenericSliceRenderer::SetModel(GenericSliceModel *model)
{
  this->m_Model = model;

  // Build the texture map
  OpenGLTextureAssociationFactory texFactoryDelegate = { this };
  m_Texture.SetDelegate(texFactoryDelegate);
  m_Texture.SetImageData(model->GetDriver()->GetCurrentImageData());
  this->UpdateTextureMap();

  // Record and rebroadcast changes in the model
  Rebroadcast(m_Model, ModelUpdateEvent(), ModelUpdateEvent());

  // Also listen to events on opacity
  Rebroadcast(m_Model->GetParentUI()->GetGlobalState()->GetSegmentationAlphaModel(),
              ValueChangedEvent(), AppearanceUpdateEvent());

  // Also listen to events on intensity curves
  Rebroadcast(m_Model->GetParentUI()->GetIntensityCurveModel(),
              ModelUpdateEvent(), AppearanceUpdateEvent());

  // Also listen to events on the color map
  Rebroadcast(m_Model->GetParentUI()->GetColorMapModel(),
              ModelUpdateEvent(), AppearanceUpdateEvent());
}

void GenericSliceRenderer::OnUpdate()
{
  m_Model->Update();
  this->UpdateTextureMap();
}

void
GenericSliceRenderer
::paintGL()
{
  // Get the appearance settings pointer since we use it a lot
  SNAPAppearanceSettings *as =
      m_Model->GetParentUI()->GetAppearanceSettings();

  // Get the properties for the background color
  Vector3d clrBack = as->GetUIElement(
      SNAPAppearanceSettings::BACKGROUND_2D).NormalColor;

  // Clear the display, using a blue shade when under focus
  glClearColor(clrBack[0], clrBack[1], clrBack[2], 1.0);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  // Slice should be initialized before display
  if (m_Model->IsSliceInitialized())
    {
    // Set up lighting attributes
    glPushAttrib(GL_LIGHTING_BIT | GL_DEPTH_BUFFER_BIT |
                 GL_PIXEL_MODE_BIT | GL_TEXTURE_BIT );

    glDisable(GL_LIGHTING);

    // Prepare for overlay drawing.  The model view is set up to correspond
    // to pixel coordinates of the slice
    glPushMatrix();

    // First set of transforms
    glTranslated(0.5 * m_Model->GetSize()[0],
                 0.5 * m_Model->GetSize()[1],
                 0.0);

    // Zoom by display zoom
    glScalef(m_Model->GetViewZoom(), m_Model->GetViewZoom(), 1.0);

    // Panning
    glTranslated(-m_Model->GetViewPosition()[0],
                 -m_Model->GetViewPosition()[1],
                 0.0);

    // Convert from voxel space to physical units
    glScalef(m_Model->GetSliceSpacing()[0],
             m_Model->GetSliceSpacing()[1],
             1.0);

    // Make the grey and segmentation image textures up-to-date
    this->DrawMainTexture();
    this->DrawSegmentationTexture();

    // Draw the overlays
    if(as->GetOverallVisibility())
      {
      // Draw all the overlays added to this object
      this->DrawOverlays();

      // Draw the zoom locator
      if(m_Model->IsThumbnailOn())
        this->DrawThumbnail();
      }

    // Clean up the GL state
    glPopMatrix();
    glPopAttrib();
    }

  // Display!
  glFlush();
}

void
GenericSliceRenderer
::resizeGL(int w, int h)
{
  // Set up projection matrix
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluOrtho2D(0.0,w,0.0,h);
  glViewport(0,0,w,h);

  // Establish the model view matrix
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
}

void GenericSliceRenderer::DrawMainTexture()
{
  // Get the appearance settings pointer since we use it a lot
  SNAPAppearanceSettings *as =
      m_Model->GetParentUI()->GetAppearanceSettings();

  // Get the interpolation mode
  GLenum interp =
      as->GetGreyInterpolationMode() == SNAPAppearanceSettings::LINEAR
      ? GL_LINEAR : GL_NEAREST;

  // Get the image data
  GenericImageData *id = m_Model->GetImageData();

  // Draw the main texture
  if (id->IsMainLoaded())
    {
    // Get the color to use for background
    Vector3d clrBackground = m_ThumbnailDrawing
      ? as->GetUIElement(SNAPAppearanceSettings::ZOOM_THUMBNAIL).NormalColor
      : Vector3d(1.0);

    // Get the texture
    Texture *tex = m_Texture[id->GetMain()];

    // Paint the grey texture with color as background
    tex->SetInterpolation(interp);
    tex->Draw(clrBackground);
    }

  // Draw each of the overlays
  if (!m_ThumbnailDrawing)
    {
    for(LayerIterator it(id, LayerIterator::OVERLAY_ROLE); !it.IsAtEnd(); ++it)
      {
      // Get the texture
      Texture *tex = m_Texture[it.GetLayer()];

      // Paint the texture with alpha
      tex->SetInterpolation(interp);
      tex->DrawTransparent(it.GetLayer()->GetAlpha());
      }
    }
}

void GenericSliceRenderer::DrawSegmentationTexture()
  {
  GenericImageData *id = m_Model->GetImageData();

  if (id->IsSegmentationLoaded())
    {
    Texture *texture = m_Texture[id->GetSegmentation()];
    texture->DrawTransparent(
          m_Model->GetParentUI()->GetDriver()->GetGlobalState()->GetSegmentationAlpha());
    }
  }

void GenericSliceRenderer::DrawThumbnail()
  {
  // Get the thumbnail appearance properties
  SNAPAppearanceSettings *as = m_Model->GetParentUI()->GetAppearanceSettings();
  const SNAPAppearanceSettings::Element &elt =
      as->GetUIElement(SNAPAppearanceSettings::ZOOM_THUMBNAIL);

  // If thumbnail is not to be drawn, exit
  if(!elt.Visible) return;

  // Tell model to figure out the thumbnail size
  m_Model->ComputeThumbnailProperties();
  Vector2i tPos = m_Model->GetThumbnailPosition();
  double tZoom = m_Model->GetThumbnailZoom();

  // Indicate the fact that we are currently drawing in thumbnail mode
  m_ThumbnailDrawing = true;

  // Set up the GL matrices
  glPushMatrix();
  glLoadIdentity();
  glTranslated((double) tPos[0], (double) tPos[1], 0.0);
  glScaled(tZoom, tZoom, 1.0);

  glPushMatrix();
  glScalef(m_Model->GetSliceSpacing()[0],m_Model->GetSliceSpacing()[1],1.0);

  // Draw the Main image (the background will be picked automatically)
  DrawMainTexture();

  // Draw the overlays that are shown on the thumbnail
  DrawOverlays();

  // Apply the line settings
  SNAPAppearanceSettings::ApplyUIElementLineSettings(elt);

  // Draw the little version of the image in the corner of the window
  double w = m_Model->GetSliceSize()[0];
  double h = m_Model->GetSliceSize()[1];

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
  glTranslated(m_Model->GetViewPosition()[0],
               m_Model->GetViewPosition()[1],
               0.0);
  w = m_Model->GetSize()[0] * 0.5 / m_Model->GetViewZoom();
  h = m_Model->GetSize()[1] * 0.5 / m_Model->GetViewZoom();

  glColor3dv(elt.ActiveColor.data_block());
  glBegin(GL_LINE_LOOP);
  glVertex2d(-w,-h);
  glVertex2d(-w, h);
  glVertex2d( w, h);
  glVertex2d( w,-h);
  glEnd();

  glPopMatrix();

  // Indicate the fact that we are not drawing in thumbnail mode
  m_ThumbnailDrawing = false;
  }

GenericSliceRenderer::Texture *
GenericSliceRenderer::CreateTexture(ImageWrapperBase *iw)
{
  if(iw->IsInitialized())
    {
    Texture *texture = new Texture(4, GL_RGBA);
    texture->SetImage(iw->GetDisplaySlice(m_Model->GetId()).GetPointer());

    SNAPAppearanceSettings *as = m_Model->GetParentUI()->GetAppearanceSettings();
    GLint imode = as->GetGreyInterpolationMode() == SNAPAppearanceSettings::LINEAR
        ? GL_LINEAR : GL_NEAREST;
    texture->SetInterpolation(imode);
    return texture;
    }
  else return NULL;
}

/*
void GenericSliceRenderer::AssociateTexture(
  ImageWrapperBase *iw, TextureMap &src, TextureMap &trg)
{
  if(iw->IsInitialized())
    {
    TextureMap::iterator it = src.find(iw);
    Texture *texture;

    if (it != src.end())
      {
      texture = it->second;
      itk::ImageBase<2> *b1 = iw->GetDisplaySlice(m_Model->GetId()).GetPointer();
      const itk::ImageBase<2> *b2 = texture->GetImage();
      std::cout << "TEX1 " << b1 << "   TEX2" << b2 << std::endl;
      src.erase(it);
      }
    else
      {
      texture = new Texture(4, GL_RGBA);
      texture->SetImage(iw->GetDisplaySlice(m_Model->GetId()).GetPointer());
      }

    // Set the interpolation approach
    SNAPAppearanceSettings *as = m_Model->GetParentUI()->GetAppearanceSettings();
    GLint imode = as->GetGreyInterpolationMode() == SNAPAppearanceSettings::LINEAR
        ? GL_LINEAR : GL_NEAREST;
    texture->SetInterpolation(imode);

    // Store the texture association
    trg[iw] = texture;
    }
}

*/

void GenericSliceRenderer::UpdateTextureMap()
{
  if(m_Model->IsSliceInitialized())
    {
    m_Texture.Update();
/*
    GenericImageData *id = m_Model->GetImageData();

    // Create a new texture map where the associations go
    TextureMap m;
    AssociateTexture(id->GetMain(), m_Texture, m);
    AssociateTexture(id->GetSegmentation(), m_Texture, m);
    for(GenericImageData::WrapperIterator it = id->GetOverlays()->begin();
        it != id->GetOverlays()->end(); it++)
      {
      AssociateTexture(*it, m_Texture, m);
      }

    // Delete old textures that haven't been removed after association
    for(TextureMap::iterator it = m_Texture.begin(); it != m_Texture.end(); it++)
      delete (it->second);

    // Copy the associations
    m_Texture = m;
    }
    */
    }
}

void GenericSliceRenderer::initializeGL()
{
}

void GenericSliceRenderer::DrawOverlays()
{
  // The renderer will contain a list of overlays that implement the
  // generic interface
  for(RendererDelegateList::iterator it = m_Overlays.begin();
      it != m_Overlays.end(); it++)
    {
    (*it)->paintGL();
    }
}

OpenGLTextureAssociationFactory::Texture *
OpenGLTextureAssociationFactory
::New(ImageWrapperBase *layer)
{
  return m_Renderer->CreateTexture(layer);
}


template class LayerAssociation<GenericSliceRenderer::Texture,
                                ImageWrapperBase,
                                OpenGLTextureAssociationFactory>;
