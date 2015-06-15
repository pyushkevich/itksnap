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
#include "SliceWindowCoordinator.h"
#include "PaintbrushSettingsModel.h"

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

  // Listen to changes to the segmentation
  Rebroadcast(m_Model->GetDriver(), SegmentationChangeEvent(), AppearanceUpdateEvent());

  // Changes to cell layout also must be rebroadcast
  DisplayLayoutModel *dlm = m_Model->GetParentUI()->GetDisplayLayoutModel();
  Rebroadcast(dlm, DisplayLayoutModel::LayerLayoutChangeEvent(),
              AppearanceUpdateEvent());

  // Listen to changes in appearance
  Rebroadcast(m_Model->GetParentUI()->GetAppearanceSettings(),
              ChildPropertyChangedEvent(), AppearanceUpdateEvent());

  // Listen to overall visibility of overlaps
  Rebroadcast(m_Model->GetParentUI()->GetAppearanceSettings()->GetOverallVisibilityModel(),
              ValueChangedEvent(), AppearanceUpdateEvent());

  // Paintbrush appearance changes
  PaintbrushSettingsModel *psm = m_Model->GetParentUI()->GetPaintbrushSettingsModel();
  Rebroadcast(psm->GetBrushSizeModel(), ValueChangedEvent(), AppearanceUpdateEvent());

  // Which layer is currently selected
  Rebroadcast(m_Model->GetParentUI()->GetDriver()->GetGlobalState()->GetSelectedLayerIdModel(),
              ValueChangedEvent(), AppearanceUpdateEvent());

  Rebroadcast(m_Model->GetHoveredImageLayerIdModel(), ValueChangedEvent(), AppearanceUpdateEvent());
  Rebroadcast(m_Model->GetHoveredImageIsThumbnailModel(), ValueChangedEvent(), AppearanceUpdateEvent());

}

void GenericSliceRenderer::OnUpdate()
{
  // Make sure the model has been updated first
  m_Model->Update();

  // Also make sure to update the model zoom coordinator (this is confusing)
  m_Model->GetParentUI()->GetSliceCoordinator()->Update();

  // Also make sure to update the display layout model
  m_Model->GetParentUI()->GetDisplayLayoutModel()->Update();

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
  // Get the current image data
  GenericImageData *id = m_Model->GetDriver()->GetCurrentImageData();

  // Get the appearance settings pointer since we use it a lot
  SNAPAppearanceSettings *as =
      m_Model->GetParentUI()->GetAppearanceSettings();

  // Get the properties for the background color
  Vector3d clrBack = as->GetUIElement(
      SNAPAppearanceSettings::BACKGROUND_2D)->GetNormalColor();

  // Get the overall viewport
  Vector2ui vp_full = m_Model->GetSizeReporter()->GetViewportSize();

  // Set up lighting attributes
  glPushAttrib(GL_LIGHTING_BIT | GL_DEPTH_BUFFER_BIT |
               GL_PIXEL_MODE_BIT | GL_TEXTURE_BIT );

  glDisable(GL_LIGHTING);

  glClearColor(clrBack[0], clrBack[1], clrBack[2], 1.0);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  // Set up the viewports for individual 'cells' in the display. Each cell constitutes one
  // image with its various overlays.

  // Slice should be initialized before display
  if (m_Model->IsSliceInitialized())
    {
    // Draw each viewport in turn. For now, the number of z-layers is hard-coded at 2
    for(int k = 0; k < m_Model->GetViewportLayout().vpList.size(); k++)
      {
      const SliceViewportLayout::SubViewport &vp = m_Model->GetViewportLayout().vpList[k];

      // Set up the viewport for the current cell
      glViewport(vp.pos[0], vp.pos[1], vp.size[0], vp.size[1]);

      // Set up the projection
      glMatrixMode(GL_PROJECTION);
      glPushMatrix();
      glLoadIdentity();
      irisOrtho2D(0.0, vp.size[0], 0.0, vp.size[1]);

      // Establish the model view matrix
      glMatrixMode(GL_MODELVIEW);
      glPushMatrix();
      glLoadIdentity();
      glPushMatrix();

      // First set of transforms
      glTranslated(0.5 * vp.size[0], 0.5 * vp.size[1], 0.0);

      // Zoom by display zoom. The amount of zoom depends on whether we are in thumbnail
      // mode or in regular mode
      double zoom = m_Model->GetViewZoom();
      if(vp.isThumbnail)
        {
        double scale_x = vp.size[0] * 1.0 / m_Model->GetCanvasSize()[0];
        double scale_y = vp.size[1] * 1.0 / m_Model->GetCanvasSize()[1];
        zoom *= std::max(scale_x, scale_y);
        }

      // Apply the correct scaling
      glScalef(zoom, zoom, 1.0);

      // Panning
      glTranslated(-m_Model->GetViewPosition()[0], -m_Model->GetViewPosition()[1], 0.0);

      // Convert from voxel space to physical units
      glScalef(m_Model->GetSliceSpacing()[0], m_Model->GetSliceSpacing()[1], 1.0);

      // Draw the main layers for this row/column combination
      ImageWrapperBase *layer = id->FindLayer(vp.layer_id, false);
      if(layer && this->DrawImageLayers(layer, !vp.isThumbnail))
        {
        // We don't want to draw segmentation over the speed image and other
        // SNAP-mode layers.
        this->DrawSegmentationTexture();

        // Draw the overlays
        if(as->GetOverallVisibility())
          {
          // Draw all the overlays added to this object
          this->DrawTiledOverlays();
          }

        glPopMatrix();

        // Determine if the current layer is hovered over by the mouse
        bool is_hover = layer->GetUniqueId() == m_Model->GetHoveredImageLayerId();
        bool is_thumb = vp.isThumbnail;
        bool is_selected = layer->GetUniqueId() == m_Model->GetDriver()->GetGlobalState()->GetSelectedLayerId();

        // Draw decoration around layer thumbnail. This is done when the thumbnail is hovered over
        // or currently selected
        if(is_thumb && (is_hover || is_selected))
          {
          // If the layer has positive z, draw a line
          glPushAttrib(GL_LINE_BIT | GL_COLOR_BUFFER_BIT);

          // The element used for highlighting thumbnails
          SmartPtr<OpenGLAppearanceElement> elt = OpenGLAppearanceElement::New();
          elt->SetNormalColor(Vector3d(0.6, 0.54, 0.46));
          elt->SetActiveColor(Vector3d(1.0, 0.9, 0.1));
          elt->SetLineThickness(3.0);
          elt->SetVisible(true);
          elt->SetAlphaBlending(false);

          elt->ApplyLineSettings();

          // Determine the colors
          if(is_selected && is_hover)
            {
            Vector3d clr = elt->GetActiveColor();
            clr += 0.4;
            clr = clr.clamp(Vector3d(0.0), Vector3d(1.0));
            glColor3dv(clr.data_block());
            }
          else if(is_selected)
            {
            glColor3dv(elt->GetActiveColor().data_block());
            }
          else if(is_hover)
            {
            glColor3dv(elt->GetNormalColor().data_block());
            }

          glBegin(GL_LINE_LOOP);
          glVertex2i(0,0);
          glVertex2i(0,vp.size[1]);
          glVertex2i(vp.size[0], vp.size[1]);
          glVertex2i(vp.size[0], 0);
          glEnd();

          glPopAttrib();
          }

        // Draw context menu indicator for the layer being hovered
        /*
         * // NOTE - this is now being done in the Qt code instead
        if(is_hover && is_thumb == m_Model->GetHoveredImageIsThumbnail())
          {
          // Load the texture for the icon
          static GLuint icon_texture_id = -1u;
          static Vector2ui icon_size;
          int vpratio = m_Model->GetSizeReporter()->GetViewportPixelRatio();
          if(icon_texture_id == -1u)
            {
            m_PlatformSupport->LoadTexture("context_gray_12", icon_texture_id, icon_size);
            }

          // Draw the icon in the corner of the view
          glPushAttrib(GL_COLOR_BUFFER_BIT | GL_TEXTURE_BIT);
          glEnable(GL_TEXTURE_2D);
          glEnable(GL_BLEND);
          glBindTexture(GL_TEXTURE_2D, icon_texture_id);
          glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

          glColor3d(1.0, 1.0, 1.0);
          glBegin(GL_QUADS);
          int margin = 4 * vpratio;
          int x0 = vp.size[0] - margin - icon_size[0], x1 = vp.size[0] - margin;
          int y1 = vp.size[1] - margin - icon_size[1], y0 = vp.size[1] - margin;
          glTexCoord2d(0.0, 0.0); glVertex2i(x0,y0);
          glTexCoord2d(0.0, 1.0); glVertex2i(x0,y1);
          glTexCoord2d(1.0, 1.0); glVertex2i(x1,y1);
          glTexCoord2d(1.0, 0.0); glVertex2i(x1,y0);
          glEnd();

          glPopAttrib();
          }
          */


        glPopMatrix();
        glMatrixMode(GL_PROJECTION);
        glPopMatrix();
        }
      }

    // Set the viewport and projection to original dimensions
    glViewport(0, 0, vp_full[0], vp_full[1]);

    // Set up the projection
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    irisOrtho2D(0.0, vp_full[0], 0.0, vp_full[1]);

    // Establish the model view matrix
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    if(as->GetOverallVisibility())
      {
      // Draw the zoom locator
      if(m_Model->IsThumbnailOn())
        this->DrawThumbnail();

      // Draw the global overlays
      this->DrawGlobalOverlays();
      }

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();

    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    }

  // Draw the various decorations
  glPopAttrib();

  // Display!
  glFlush();
}

void
GenericSliceRenderer
::resizeGL(int w, int h, int device_pixel_ratio)
{
  // Set up projection matrix
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  irisOrtho2D(0.0,w,0.0,h);
  glViewport(0,0,w,h);

  // Establish the model view matrix
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
}

bool GenericSliceRenderer::DrawImageLayers(ImageWrapperBase *base_layer, bool drawStickies)
{
  // Get the image data
  GenericImageData *id = m_Model->GetImageData();

  // If drawing the thumbnail, only draw the main layer
  if(m_ThumbnailDrawing)
    {
    DrawTextureForLayer(base_layer, false);
    return true;
    }

  // Is the display partitioned into rows and columns?
  if(!this->IsTiledMode())
    {
    // Draw the base layer without transparency
    DrawTextureForLayer(base_layer, false);

    // Now draw all the sticky layers on top
    if(drawStickies)
      {
        for(LayerIterator it(id); !it.IsAtEnd(); ++it)
        {
        ImageWrapperBase *layer = it.GetLayer();
        if(it.GetRole() != LABEL_ROLE
           && layer->IsDrawable()
           && layer->IsSticky()
           && layer->GetAlpha() > 0)
          {
          DrawTextureForLayer(layer, true);
          }
        }
      }

    return true;
    }
  else
    {
    // Draw the particular layer
    DrawTextureForLayer(base_layer, false);

    // Now draw all the non-sticky layers
    if(drawStickies)
      {
      for(LayerIterator itov(id); !itov.IsAtEnd(); ++itov)
        {
        if(itov.GetRole() != MAIN_ROLE
           && itov.GetLayer()->IsSticky()
           && itov.GetLayer()->IsDrawable()
           && itov.GetLayer()->GetAlpha() > 0)
          {
          DrawTextureForLayer(itov.GetLayer(), true);
          }
        }
      }

    return true;
  }
}

bool GenericSliceRenderer::IsTiledMode() const
{
  DisplayLayoutModel *dlm = m_Model->GetParentUI()->GetDisplayLayoutModel();
  Vector2ui layout = dlm->GetSliceViewLayerTilingModel()->GetValue();
  return layout[0] > 1 || layout[1] > 1;
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
    for(LayerIterator it(id, OVERLAY_ROLE); !it.IsAtEnd(); ++it)
      DrawTextureForLayer(it.GetLayer(), true);
    }
}

void GenericSliceRenderer::DrawTextureForLayer(
    ImageWrapperBase *layer, bool use_transparency)
{
  // Get the appearance settings pointer since we use it a lot
  SNAPAppearanceSettings *as =
      m_Model->GetParentUI()->GetAppearanceSettings();

  // Get the global display settings
  const GlobalDisplaySettings *gds =
      m_Model->GetParentUI()->GetGlobalDisplaySettings();

  // Get the interpolation mode
  GLenum interp =
      gds->GetGreyInterpolationMode() == GlobalDisplaySettings::LINEAR
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
        ? as->GetUIElement(SNAPAppearanceSettings::ZOOM_THUMBNAIL)->GetNormalColor()
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
    texture->DrawTransparent(alpha);
    }
  }

void GenericSliceRenderer::DrawThumbnail()
  {
  // Get the thumbnail appearance properties
  SNAPAppearanceSettings *as = m_Model->GetParentUI()->GetAppearanceSettings();
  const OpenGLAppearanceElement *elt =
      as->GetUIElement(SNAPAppearanceSettings::ZOOM_THUMBNAIL);

  // If thumbnail is not to be drawn, exit
  if(!elt->GetVisible()) return;

  // Tell model to figure out the thumbnail size
  m_Model->ComputeThumbnailProperties();
  Vector2i tPos = m_Model->GetZoomThumbnailPosition();
  double tZoom = m_Model->GetThumbnailZoom();

  // Current display layout
  DisplayLayoutModel *dlm = m_Model->GetParentUI()->GetDisplayLayoutModel();
  Vector2ui layout = dlm->GetSliceViewLayerTilingModel()->GetValue();
  unsigned int rows = layout[0], cols = layout[1];

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
  elt->ApplyLineSettings();

  // Draw the little version of the image in the corner of the window
  double w = m_Model->GetSliceSize()[0];
  double h = m_Model->GetSliceSize()[1];

  // Draw the line around the image
  glColor3dv(elt->GetNormalColor().data_block());
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
  w = m_Model->GetCanvasSize()[0] * 0.5 / m_Model->GetViewZoom();
  h = m_Model->GetCanvasSize()[1] * 0.5 / m_Model->GetViewZoom();

  glColor3dv(elt->GetActiveColor().data_block());
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

    const GlobalDisplaySettings *gds = m_Model->GetParentUI()->GetGlobalDisplaySettings();

    GLint imode =
        (gds->GetGreyInterpolationMode() == GlobalDisplaySettings::LINEAR)
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


