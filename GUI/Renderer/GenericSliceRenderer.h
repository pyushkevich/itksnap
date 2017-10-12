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

#ifndef GENERICSLICERENDERER_H
#define GENERICSLICERENDERER_H

#include <AbstractRenderer.h>
#include <GenericSliceModel.h>
#include <ImageWrapper.h>
#include <OpenGLSliceTexture.h>
#include <SNAPOpenGL.h>
#include <list>
#include <LayerAssociation.h>

class GenericSliceRenderer;

class SliceRendererDelegate : public AbstractRenderer
{
public:
  virtual ~SliceRendererDelegate() {}

  irisGetSetMacro(ParentRenderer, GenericSliceRenderer *)

protected:
  GenericSliceRenderer *m_ParentRenderer;
};

class GenericSliceRenderer : public AbstractRenderer
{
public:

  // texture type
  typedef OpenGLSliceTexture<ImageWrapperBase::DisplayPixelType> Texture;


  irisITKObjectMacro(GenericSliceRenderer, AbstractModel)

  FIRES(ModelUpdateEvent)

  void SetModel(GenericSliceModel *model);

  void initializeGL() ITK_OVERRIDE;
  virtual void resizeGL(int w, int h, int device_pixel_ratio) ITK_OVERRIDE;
  virtual void paintGL() ITK_OVERRIDE;

  irisGetMacro(Model, GenericSliceModel *)

  /** This flag is on while the zoom thumbnail is being painted */
  irisIsMacro(DrawingZoomThumbnail)

  /** This flag is on while the layer thumbnail is being painted */
  irisIsMacro(DrawingLayerThumbnail)

  // Viewport object
  typedef SliceViewportLayout::SubViewport ViewportType;

  /**
   * Get a pointer to the viewport that is currently being drawn, or
   * NULL if a viewport is not being drawn
   */
  const ViewportType *GetDrawingViewport() const;


  typedef std::list<SliceRendererDelegate *> RendererDelegateList;

  // Get a reference to the list of overlays stored in here
  const RendererDelegateList &GetGlobalOverlays() const
    { return m_GlobalOverlays; }

  RendererDelegateList &GetGlobalOverlays()
    { return m_GlobalOverlays; }

  // Get a reference to the list of overlays stored in here
  const RendererDelegateList &GetTiledOverlays() const
    { return m_TiledOverlays; }

  RendererDelegateList &GetTiledOverlays()
    { return m_TiledOverlays; }

  // A callback for when the model is reinitialized
  // void OnModelReinitialize();

  // Get (creating if necessary) and configure the texture for a given layer
  Texture *GetTextureForLayer(ImageWrapperBase *iw);

  // Set list of child renderers
  void SetChildRenderers(std::list<AbstractRenderer *> renderers);

protected:

  GenericSliceRenderer();
  virtual ~GenericSliceRenderer() {}

  void OnUpdate() ITK_OVERRIDE;

  void DrawSegmentationTexture();
  void DrawOverlayTexture();
  void DrawThumbnail();
  void DrawTiledOverlays();
  void DrawGlobalOverlays();


  // Draw the image and overlays either on top of each other or separately
  // in individual cells. Returns true if a layer was drawn, false if not,
  // i.e., the cell is outside of the range of available layers
  bool DrawImageLayers(
      ImageWrapperBase *base_layer,
      const ViewportType &vp);

  // This method can be used by the renderer delegates to draw a texture
  void DrawTextureForLayer(ImageWrapperBase *layer, const ViewportType &vp, bool use_transparency);

  bool IsTiledMode() const;

  GenericSliceModel *m_Model;

  // Whether rendering to thumbnail or not
  bool m_DrawingZoomThumbnail, m_DrawingLayerThumbnail;

  // The index of the viewport that is currently being drawn - for use in child renderers
  int m_DrawingViewportIndex;

  // A list of overlays that the user can configure
  RendererDelegateList m_TiledOverlays, m_GlobalOverlays;

  // List of child renderers
  std::list<AbstractRenderer *>m_ChildRenderers;

  Vector3d ComputeGridPosition(const Vector3d &disp_pix, const itk::Index<2> &slice_index, ImageWrapperBase *vecimg);
};



#endif // GENERICSLICERENDERER_H
