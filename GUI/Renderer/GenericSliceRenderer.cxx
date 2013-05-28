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
#include "DisplayLayoutModel.h"
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
  m_Texture.SetSource(model->GetDriver());
  this->UpdateTextureMap();

  // Record and rebroadcast changes in the model
  Rebroadcast(m_Model, ModelUpdateEvent(), ModelUpdateEvent());

  // Also listen to events on opacity
  Rebroadcast(m_Model->GetParentUI()->GetGlobalState()->GetSegmentationAlphaModel(),
              ValueChangedEvent(), AppearanceUpdateEvent());

  // Listen to changes in the appearance of any of the wrappers
  Rebroadcast(m_Model->GetDriver(), WrapperChangeEvent(), AppearanceUpdateEvent());

  // Changes to cell layout also must be rebroadcast
  DisplayLayoutModel *dlm = m_Model->GetParentUI()->GetDisplayLayoutModel();
  Rebroadcast(dlm, DisplayLayoutModel::LayerLayoutChangeEvent(),
              AppearanceUpdateEvent());
}

void GenericSliceRenderer::OnUpdate()
{
  // Make sure the model has been updated first
  m_Model->Update();

  // Only update the texture map in response to "big" events
  if(m_EventBucket->HasEvent(ModelUpdateEvent(), m_Model))
    {
    this->UpdateTextureMap();
    }
}

void
GenericSliceRenderer
::paintGL()
{
  // Number of divisions
  DisplayLayoutModel *dlm = m_Model->GetParentUI()->GetDisplayLayoutModel();
  Vector2ui layout = dlm->GetSliceViewLayerTilingModel()->GetValue();
  int nrows = (int) layout[0];
  int ncols = (int) layout[1];

  // Get the dimensions of the cells
  unsigned int cell_w = m_Model->GetSize()[0];
  unsigned int cell_h = m_Model->GetSize()[1];

  // Get the appearance settings pointer since we use it a lot
  SNAPAppearanceSettings *as =
      m_Model->GetParentUI()->GetAppearanceSettings();

  // Get the properties for the background color
  Vector3d clrBack = as->GetUIElement(
      SNAPAppearanceSettings::BACKGROUND_2D).NormalColor;

  // Slice should be initialized before display
  if (m_Model->IsSliceInitialized())
    {
    // Set up lighting attributes
    glPushAttrib(GL_LIGHTING_BIT | GL_DEPTH_BUFFER_BIT |
                 GL_PIXEL_MODE_BIT | GL_TEXTURE_BIT );

    glDisable(GL_LIGHTING);

    glClearColor(clrBack[0], clrBack[1], clrBack[2], 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Draw each cell in the nrows by ncols table of images. Usually, there
    // will only be one column, but the new SNAP supports side-by-side drawing
    // of layers for when there are overlays
    for(int irow = 0; irow < nrows; irow++)
      for(int icol = 0; icol < ncols; icol++)
        {
        // Set up the viewport for the current cell
        glViewport(icol * cell_w, (nrows - 1 - irow) * cell_h,
                   cell_w, cell_h);

        // Set up the projection
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        gluOrtho2D(0.0, cell_w, 0.0, cell_h);

        // Establish the model view matrix
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();

        // Scale to account for multiple rows and columns
        // glScaled(1.0 / ncols, 1.0 / nrows, 1.0);

        // Prepare for overlay drawing.  The model view is set up to correspond
        // to pixel coordinates of the slice
        glPushMatrix();

        // First set of transforms
        glTranslated(0.5 * cell_w, 0.5 * cell_h, 0.0);

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

        // Draw the main layers for this row/column combination
        if(this->DrawImageLayers(nrows, ncols, irow, icol))
          {
          this->DrawSegmentationTexture();

          // Draw the overlays
          if(as->GetOverallVisibility())
            {
            // Draw all the overlays added to this object
            this->DrawTiledOverlays();

            // Draw the zoom locator
            if(m_Model->IsThumbnailOn() && irow == (nrows-1) && icol == 0)
              this->DrawThumbnail();
            }
          }

        // Clean up the GL state
        glPopMatrix();
        }

    // Set the viewport and projection to original dimensions
    Vector2ui vp = m_Model->GetSizeReporter()->GetViewportSize();

    glViewport(0, 0, vp[0], vp[1]);

    // Set up the projection
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0.0, vp[0], 0.0, vp[1]);

    // Establish the model view matrix
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    // Draw the global overlays
    this->DrawGlobalOverlays();

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();

    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();

    // Draw the various decorations
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

bool GenericSliceRenderer::DrawImageLayers(int nrows, int ncols, int irow, int icol)
{
  // Get the image data
  GenericImageData *id = m_Model->GetImageData();

  // If drawing the thumbnail, only draw the main layer
  if(m_ThumbnailDrawing)
    {
    DrawTextureForLayer(id->GetMain(), false);
    return true;
    }

  // Is the display partitioned into rows and columns?
  if(nrows == 1 && ncols == 1)
    {
    // Draw all the layers that are visible except segmentation, which is handled
    // separately (last)
    for(LayerIterator it(id); !it.IsAtEnd(); ++it)
      {
      ImageWrapperBase *layer = it.GetLayer();
      if(it.GetRole() == LayerIterator::MAIN_ROLE)
        {
        DrawTextureForLayer(layer, false);
        }
      else if(it.GetRole() != LayerIterator::LABEL_ROLE
              && layer->IsDrawable() && layer->GetAlpha() > 0)
        {
        DrawTextureForLayer(it.GetLayer(), true);
        }
      }

    return true;
    }
  else
    {
    // How many layers to go until we get to the one we want to paint?
    int togo = irow * ncols + icol;

    // Skip all layers until we get to the sticky layer we want to paint
    for(LayerIterator it(id); !it.IsAtEnd(); ++it)
      {
      if(it.GetRole() == LayerIterator::MAIN_ROLE || !it.GetLayer()->IsSticky())
        {
        if(togo > 0)
          togo--;
        else
          {
          // Check if the layer is in drawable condition. If not, draw nothing.
          if(!it.GetLayer()->IsDrawable())
            return false;

          // Draw the particular layer
          DrawTextureForLayer(it.GetLayer(), false);

          // Now draw all the non-sticky layers
          for(LayerIterator itov(id); !itov.IsAtEnd(); ++itov)
            {
            if(itov.GetRole() != LayerIterator::MAIN_ROLE
               && itov.GetLayer()->IsSticky()
               && it.GetLayer()->IsDrawable()
               && itov.GetLayer()->GetAlpha() > 0)
              {
              DrawTextureForLayer(itov.GetLayer(), true);
              }
            }

          return true;
          }
        }
      }

    // Didn't draw anything...
    return false;
    }
}


void GenericSliceRenderer::DrawMainTexture()
{
  // Get the image data
  GenericImageData *id = m_Model->GetImageData();

  // Draw the main texture
  if (id->IsMainLoaded())
    DrawTextureForLayer(id->GetMain(), false);

  // Draw each of the overlays
  if (!m_ThumbnailDrawing)
    {
    for(LayerIterator it(id, LayerIterator::OVERLAY_ROLE); !it.IsAtEnd(); ++it)
      DrawTextureForLayer(it.GetLayer(), true);
    }
}

void GenericSliceRenderer::DrawTextureForLayer(
    ImageWrapperBase *layer, bool use_transparency)
{
  // Get the appearance settings pointer since we use it a lot
  SNAPAppearanceSettings *as =
      m_Model->GetParentUI()->GetAppearanceSettings();

  // Get the interpolation mode
  GLenum interp =
      as->GetGreyInterpolationMode() == SNAPAppearanceSettings::LINEAR
      ? GL_LINEAR : GL_NEAREST;

  // Get the texture
  Texture *tex = m_Texture[layer];

  // Paint the texture with alpha
  if(tex)
    {
    tex->SetInterpolation(interp);
    if(use_transparency)
      {
      tex->DrawTransparent(layer->GetAlpha());
      }
    else
      {
      Vector3d clrBackground = m_ThumbnailDrawing
        ? as->GetUIElement(SNAPAppearanceSettings::ZOOM_THUMBNAIL).NormalColor
        : Vector3d(1.0);
      tex->Draw(clrBackground);
      }
    }
}


void GenericSliceRenderer::DrawSegmentationTexture()
  {
  GenericImageData *id = m_Model->GetImageData();

  if (id->IsSegmentationLoaded())
    {
    Texture *texture = m_Texture[id->GetSegmentation()];
    double alpha = m_Model->GetParentUI()->GetDriver()->GetGlobalState()->GetSegmentationAlpha();
    texture->DrawTransparent((unsigned char)(255 * alpha));
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
  DrawTiledOverlays();

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
    }
}

void GenericSliceRenderer::initializeGL()
{
}

void GenericSliceRenderer::DrawTiledOverlays()
{
  // The renderer will contain a list of overlays that implement the
  // generic interface
  for(RendererDelegateList::iterator it = m_TiledOverlays.begin();
      it != m_TiledOverlays.end(); it++)
    {
    (*it)->paintGL();
    }
}

void GenericSliceRenderer::DrawGlobalOverlays()
{
  // The renderer will contain a list of overlays that implement the
  // generic interface
  for(RendererDelegateList::iterator it = m_GlobalOverlays.begin();
      it != m_GlobalOverlays.end(); it++)
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


