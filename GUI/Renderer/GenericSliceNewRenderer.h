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
#include <list>
#include <map>
#include <LayerAssociation.h>
#include "AbstractNewRenderer.h"

class GenericSliceNewRenderer : public AbstractNewRenderer
{
public:
  irisITKObjectMacro(GenericSliceNewRenderer, AbstractModel)

  FIRES(ModelUpdateEvent)

  void SetModel(GenericSliceModel *model);

  irisGetMacro(Model, GenericSliceModel *)

  /** This flag is on while the zoom thumbnail is being painted */
  // irisIsMacro(DrawingZoomThumbnail)

  /** This flag is on while the layer thumbnail is being painted */
  // irisIsMacro(DrawingLayerThumbnail)

  // Viewport object
  typedef SliceViewportLayout::SubViewport ViewportType;

  /**
   * Get a pointer to the viewport that is currently being drawn, or
   * NULL if a viewport is not being drawn
   */
  // const ViewportType *GetDrawingViewport() const;

  // This is the main render method. It uses the context to interact with the
  // underlying paint engine.
  virtual void Render(AbstractNewRenderContext *context) override;

  // typedef std::list<SliceRendererDelegate *> RendererDelegateList;

  // Get a reference to the list of overlays stored in here
  // const RendererDelegateList &GetDelegates() const { return m_Delegates; }

  // void SetDelegates(const RendererDelegateList &ovl);

  // A callback for when the model is reinitialized
  // void OnModelReinitialize();

  // Set list of child renderers
  // void SetChildRenderers(std::list<AbstractRenderer *> renderers);

protected:
  // Constants for layer depth ordering
  const double DEPTH_OVERLAY_START = 0.01;
  const double DEPTH_SEGMENTATION_START = 0.02;
  const double DEPTH_STEP = 0.0001;

  GenericSliceNewRenderer() {};
  virtual ~GenericSliceNewRenderer() {}

  void OnUpdate() override;

  // Get a layer assembly for a layer
  // BaseLayerAssembly *GetBaseLayerAssembly(ImageWrapperBase *wrapper);

  /*
  // Update the renderer layout in response to a change in tiling
  void UpdateRendererLayout();

  // Update the renderer model/view matrices in response to zooming or panning
  void UpdateRendererCameras();

  // Update the appearance of various props in the scene
  void UpdateLayerApperances();

  // Update the z-position of various layers
  void UpdateLayerDepth();

  // Update the zoom pan thumbnail appearance
  void UpdateZoomPanThumbnail();

  // Update the tiled overlay context scene contents for one of the tiles
  void UpdateTiledOverlayContextSceneItems(ImageWrapperBase *wrapper);

  // Update the tiled overlay context scene transform for one of the tiles
  void UpdateTiledOverlayContextSceneTransform(ImageWrapperBase *wrapper);

  bool IsTiledMode() const;
*/

  GenericSliceModel *m_Model = nullptr;

  // Whether rendering to thumbnail or not
  bool m_DrawingZoomThumbnail, m_DrawingLayerThumbnail;

  // Keys to access user data for this renderer from layers
  std::string m_LayerTextureKey, m_LayerZoomThumbnailTextureKey;

  // The index of the viewport that is currently being drawn - for use in child renderers
  int m_DrawingViewportIndex;

  /*

  // A list of overlays that the user can configure
  RendererDelegateList m_Delegates;

  void UpdateSceneAppearanceSettings();

*/

  using Texture = AbstractNewRenderContext::Texture;
  struct TextureInfo
  {
    Texture *texture = nullptr;
    unsigned int w = 0, h = 0;
  };
  using TextureInfoKey = std::pair<unsigned int, DisplaySliceIntent>;
  using TextureCache = std::map<TextureInfoKey, TextureInfo>;

  // Internal function to get a texture from layer using local cache if possible
  TextureInfo GetTexture(AbstractNewRenderContext *context,
                         TextureCache             &texture_cache,
                         ImageWrapperBase         *layer,
                         DisplaySliceIntent       intent);

  // Internal function to render the texture for a layer
  void RenderLayer(AbstractNewRenderContext *context,
                   TextureCache             &texture_cache,
                   ImageWrapperBase         *layer,
                   bool                      bilinear,
                   double                    thumbnail_zoom,
                   double                    opacity,
                   DisplaySliceIntent        intent);
};


#endif // GENERICSLICENEWRENDERER_H
