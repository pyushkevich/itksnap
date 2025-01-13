/*=========================================================================

Program:   ITK-SNAP
Module:    $RCSfile: Filename.cxx,v $
Language:  C++
Date:      $Date: 2010/10/18 11:25:44 $
Version:   $Revision: 1.12 $
Copyright (c) 2025 Paul A. Yushkevich

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
#ifndef GENERICSLICENEWRENDERER_H
#define GENERICSLICENEWRENDERER_H

#include <GenericSliceModel.h>
#include <ImageWrapper.h>
#include <map>
#include <list>
#include <LayerAssociation.h>
#include "AbstractContextBasedRenderer.h"

class SliceRendererDelegate : public AbstractModel
{
public:
  irisITKAbstractObjectMacro(SliceRendererDelegate, AbstractModel)
  FIRES(ModelUpdateEvent)

  using SubViewport = SliceViewportLayout::SubViewport;

  void SetModel(GenericSliceModel *model);

  /**
   * This method should be overridden by child class to perform drawing on the
   * whole slice window. The render context will have the transform set to
   * use the viewport coordinate
   */
  virtual void RenderOverMainViewport(AbstractRenderContext *context) {};

  /**
   * This method should be overridden by child class to perform drawing on a
   * specific tiled layer. The render context will have the transform set to
   * use the slice coordinate system
   */
  virtual void RenderOverTiledLayer(AbstractRenderContext *context,
                                    ImageWrapperBase         *base_layer,
                                    const SubViewport        &vp) {};
};


class GenericSliceRenderer : public AbstractContextBasedRenderer
{
public:
  irisITKObjectMacro(GenericSliceRenderer, AbstractModel)

  FIRES(ModelUpdateEvent)

  void SetModel(GenericSliceModel *model);

  irisGetMacro(Model, GenericSliceModel *)

  // Viewport object
  typedef SliceViewportLayout::SubViewport ViewportType;

  /**
   * Get a pointer to the viewport that is currently being drawn, or
   * NULL if a viewport is not being drawn
   */
  // const ViewportType *GetDrawingViewport() const;

  // This is the main render method. It uses the context to interact with the
  // underlying paint engine.
  virtual void Render(AbstractRenderContext *context) override;

  typedef std::list<SliceRendererDelegate *> RendererDelegateList;

  // Get a reference to the list of overlays stored in here
  const RendererDelegateList &GetDelegates() const { return m_Delegates; }

  /** Set the array of delegates who perform overlay rendering tasks */
  void SetDelegates(const RendererDelegateList &ovl);

  // A callback for when the model is reinitialized
  // void OnModelReinitialize();

  // Set list of child renderers
  // void SetChildRenderers(std::list<AbstractRenderer *> renderers);

protected:
  // Constants for layer depth ordering
  const double DEPTH_OVERLAY_START = 0.01;
  const double DEPTH_SEGMENTATION_START = 0.02;
  const double DEPTH_STEP = 0.0001;

  GenericSliceRenderer() {};
  virtual ~GenericSliceRenderer() {}

  void OnUpdate() override;

  GenericSliceModel *m_Model = nullptr;

  // Whether rendering to thumbnail or not
  bool m_DrawingZoomThumbnail, m_DrawingLayerThumbnail;

  // Keys to access user data for this renderer from layers
  std::string m_LayerTextureKey, m_LayerZoomThumbnailTextureKey;

  // The index of the viewport that is currently being drawn - for use in child renderers
  int m_DrawingViewportIndex;

  // A list of overlays that the user can configure
  RendererDelegateList m_Delegates;

  using Texture = AbstractRenderContext::Texture;
  struct TextureInfo
  {
    Texture *texture = nullptr;
    unsigned int w = 0, h = 0;
  };
  using TextureInfoKey = std::pair<unsigned int, DisplaySliceIntent>;
  using TextureCache = std::map<TextureInfoKey, TextureInfo>;

  // Internal function to get a texture from layer using local cache if possible
  TextureInfo GetTexture(AbstractRenderContext *context,
                         TextureCache             &texture_cache,
                         ImageWrapperBase         *layer,
                         DisplaySliceIntent       intent);

  // Internal function to render the texture for a layer
  void RenderLayer(AbstractRenderContext *context,
                   TextureCache             &texture_cache,
                   ImageWrapperBase         *layer,
                   bool                      bilinear,
                   double                    thumbnail_zoom,
                   double                    opacity,
                   DisplaySliceIntent        intent);
};


#endif // GENERICSLICENEWRENDERER_H
