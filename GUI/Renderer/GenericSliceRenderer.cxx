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
#include "GenericSliceRenderer.h"

#include "SNAPCommon.h"
#include "GenericSliceModel.h"
#include "GlobalUIModel.h"
#include "SNAPAppearanceSettings.h"
#include "GenericImageData.h"
#include "IRISApplication.h"
#include "IntensityCurveModel.h"
#include "DisplayLayoutModel.h"
#include "LayerAssociation.h"
#include "SliceWindowCoordinator.h"
#include "PaintbrushSettingsModel.h"
#include <itkImageLinearConstIteratorWithIndex.h>

itk::ModifiedTimeType
check_pipeline_mtime(itk::DataObject *object)
{
  auto latest_ts = object->GetMTime();
  auto src = object->GetSource();
  if (src)
  {
    if (src->GetMTime() > latest_ts)
      latest_ts = src->GetMTime();

    for (auto input : src->GetInputs())
    {
      if (input)
      {
        auto in_ts = check_pipeline_mtime(input);
        if (in_ts > latest_ts)
          latest_ts = in_ts;
      }
    }
  }
  return latest_ts;
}

void
check_pipeline_mtime_detail(itk::DataObject      *object,
                            itk::ModifiedTimeType thresh = 0,
                            std::string           indent = "")
{
  if (thresh == 0)
  {
    thresh = object->GetMTime();
    std::cout << indent.c_str() << "t(endpoint): " << thresh << std::endl;
  }
  std::string my_indent = std::string("  ") + indent;
  auto        src = object->GetSource();
  if (src)
  {
    if (src->GetMTime() > thresh)
    {
      std::cout << my_indent.c_str() << "t(PipelineObject): " << src->GetMTime() << std::endl;
      src->Print(std::cout, my_indent.size() + 4);
    }

    for (auto input : src->GetInputs())
    {
      if (input)
      {
        if (input->GetMTime() > thresh)
        {
          std::cout << my_indent.c_str() << "t(DataObject): " << input->GetMTime() << std::endl;
          input->Print(std::cout, my_indent.size() + 4);
        }
        check_pipeline_mtime_detail(input, thresh, my_indent);
      }
    }
  }
}


void
GenericSliceRenderer::SetModel(GenericSliceModel *model)
{
  m_Model = model;

  // Set the keys to access user data
  static const char *key_texture[] = { "LayerTexture_0",
                                       "LayerTexture_1",
                                       "LayerTexture_2" };
  static const char *key_zt_texture[] = { "LayerZoomThumbTexture_0",
                                          "LayerZoomThumbTexture_1",
                                          "LayerZoomThumbTexture_2" };
  m_LayerTextureKey = key_texture[m_Model->GetId()];
  m_LayerZoomThumbnailTextureKey = key_zt_texture[m_Model->GetId()];

  // Record and rebroadcast changes in the model
  Rebroadcast(m_Model, ModelUpdateEvent(), ModelUpdateEvent());

  // Respond to changes in image dimension - these require big updates
  Rebroadcast(m_Model->GetDriver(), LayerChangeEvent(), ModelUpdateEvent());

  // Also listen to events on opacity
  Rebroadcast(m_Model->GetParentUI()->GetGlobalState()->GetSegmentationAlphaModel(),
              ValueChangedEvent(),
              AppearanceUpdateEvent());

  // Listen to changes in the appearance of any of the wrappers
  Rebroadcast(m_Model->GetDriver(), WrapperChangeEvent(), ModelUpdateEvent());

  // Listen to changes to the segmentation
  Rebroadcast(m_Model->GetDriver(), SegmentationChangeEvent(), ModelUpdateEvent());

  // Changes to cell layout also must be rebroadcast
  DisplayLayoutModel *dlm = m_Model->GetParentUI()->GetDisplayLayoutModel();
  Rebroadcast(
    dlm, DisplayLayoutModel::LayerLayoutChangeEvent(), ModelUpdateEvent());

  // Listen to changes in appearance
  Rebroadcast(m_Model->GetParentUI()->GetAppearanceSettings(),
              ChildPropertyChangedEvent(),
              ModelUpdateEvent());

  Rebroadcast(
    m_Model->GetParentUI()->GetAppearanceSettings()->GetOverallVisibilityModel(),
    ValueChangedEvent(),
    ModelUpdateEvent());

  // Listen to overall visibility of overlaps
  Rebroadcast(
    m_Model->GetParentUI()->GetAppearanceSettings()->GetOverallVisibilityModel(),
    ValueChangedEvent(),
    ModelUpdateEvent());

  // Paintbrush appearance changes
  PaintbrushSettingsModel *psm =
    m_Model->GetParentUI()->GetPaintbrushSettingsModel();
  Rebroadcast(psm->GetBrushSizeModel(), ValueChangedEvent(), ModelUpdateEvent());

  // Which layer is currently selected
  Rebroadcast(m_Model->GetDriver()->GetGlobalState()->GetSelectedLayerIdModel(),
              ValueChangedEvent(),
              ModelUpdateEvent());

  Rebroadcast(
    m_Model->GetDriver()->GetGlobalState()->GetSelectedSegmentationLayerIdModel(),
    ValueChangedEvent(),
    ModelUpdateEvent());

  Rebroadcast(m_Model->GetHoveredImageLayerIdModel(),
              ValueChangedEvent(),
              ModelUpdateEvent());
  Rebroadcast(m_Model->GetHoveredImageIsThumbnailModel(),
              ValueChangedEvent(),
              ModelUpdateEvent());

  Rebroadcast(
    m_Model->GetParentUI()->GetGlobalDisplaySettings()->GetGreyInterpolationModeModel(),
    ValueChangedEvent(),
    ModelUpdateEvent());
}

#include <chrono>
typedef std::chrono::high_resolution_clock Clock;


GenericSliceRenderer::TextureInfo
GenericSliceRenderer::GetTexture(AbstractRenderContext *context,
                                    TextureCache             &texture_cache,
                                    ImageWrapperBase         *layer,
                                    DisplaySliceIntent        intent)
{
  // Key for the local cache storing textures
  auto cache_key = std::make_pair(layer->GetUniqueId(), intent);

  // Key for retrieving the texture stored in the image layer
  auto layer_key = (intent == DISPLAY_SLICE_MAIN) ? m_LayerTextureKey : m_LayerZoomThumbnailTextureKey;

  // Find cached texture information
  TextureInfo tex;
  auto it = texture_cache.find(cache_key);

  if(it == texture_cache.end())
  {
    if (intent == DISPLAY_SLICE_MAIN ||
        (intent == DISPLAY_SLICE_THUMBNAIL && !layer->IsSlicingOrthogonal()))
    {
      // The texture should be obtained from the ITK-SNAP slicing and display mapping
      // pipeline (main window and zoom thumbnail when in non-orthogonal slicing mode)

      // Get the display slice that we will render as the base layer
      auto rgba = layer->GetDisplaySlice(DisplaySliceIndex(m_Model->GetId(), intent));

      // CODE below can be used to check what caused pipeline update
      // auto pipe_mtime = check_pipeline_mtime(rgba);
      // if(pipe_mtime > rgba->GetMTime())
      //   check_pipeline_mtime_detail(rgba, rgba->GetMTime(), "  ");

      // Update the display slice
      auto ts_before = rgba->GetMTime();
      auto start_time = Clock::now();
      rgba->GetSource()->UpdateLargestPossibleRegion();
      auto end_time = Clock::now();
      auto ts_after = rgba->GetMTime();
      bool updated = ts_after != ts_before;

      /*
      std::cout << "Texture update: " << updated << " duration : "
                << std::chrono::duration_cast<std::chrono::nanoseconds>(end_time -
                                                                        start_time)
                     .count()
                << " ns" << std::endl;
      */

      // Check if a texture is cached in the layer
      Texture::Pointer texptr = dynamic_cast<Texture *>(layer->GetUserData(layer_key));
      if (!texptr || texptr->GetMTime() < rgba->GetMTime())
      {
        // Create the texture from the image
        texptr = context->CreateTexture(rgba);

        // If zoom thumbnail, colorize it
        if(intent == DISPLAY_SLICE_THUMBNAIL)
          texptr = context->ColorizeTexture(texptr, Vector3d(1., 1., 0.));

        // Store the texture in the layer
        layer->SetUserData(layer_key, texptr.GetPointer());
      }

      // Store the texture in local texture cache
      auto rgba_size = rgba->GetBufferedRegion().GetSize();
      tex.texture = texptr;
      tex.w = rgba_size[0];
      tex.h = rgba_size[1];
      texture_cache[cache_key] = tex;
    }
    else
    {
      // Zoom thumbnail in orthogonal slicing mode - the display slice is just the whole
      // 2D slice from the image, so we don't need a separate texture for the zoom thumb
      // except for colorization.

      // Create the derived texture
      Texture::Pointer texptr = dynamic_cast<Texture *>(layer->GetUserData(layer_key));

      // Get the main texture
      TextureInfo main_tex = GetTexture(context, texture_cache, layer, DISPLAY_SLICE_MAIN);

      // Check if the texture needs to be updated
      if (!texptr || texptr->GetMTime() < main_tex.texture->GetMTime())
      {
        texptr = context->ColorizeTexture(main_tex.texture, Vector3d(1., 1., 0.));
        layer->SetUserData(m_LayerZoomThumbnailTextureKey, texptr.GetPointer());
      }

      // Store the texture in local texture cache
      tex.texture = texptr;
      tex.w = main_tex.w;
      tex.h = main_tex.h;
      texture_cache[cache_key] = tex;
    }
  }
  else
  {
    tex = it->second;
  }
  return tex;
}

void
GenericSliceRenderer::RenderLayer(AbstractRenderContext *context,
                                     TextureCache             &texture_cache,
                                     ImageWrapperBase         *layer,
                                     bool                      bilinear,
                                     double                    thumbnail_zoom,
                                     double                    opacity,
                                     DisplaySliceIntent        intent)
{
  // Get the texture for this layer
  TextureInfo tex = GetTexture(context, texture_cache, layer, intent);

  // Get the texture pointer and texture size from the cache entry
  if (layer->IsSlicingOrthogonal())
  {
    // Draw this image using texture interpolation
    context->DrawImage(0, 0, tex.w, tex.h, tex.texture, bilinear, opacity);
  }
  else
  {
    // The image should be rendered directly onto the screen using the identity transform
    context->PushMatrix();
    context->LoadIdentity();
    context->Scale(thumbnail_zoom, thumbnail_zoom);
    context->DrawImage(0, 0, tex.w, tex.h, tex.texture, bilinear, opacity);
    context->PopMatrix();
  }
}

void
GenericSliceRenderer::Render(AbstractRenderContext *context)
{
  using Texture = AbstractRenderContext::Texture;

  // Create a temporary cache to store texture pointers and sizes, this is so that we
  // don't call the ITK update pipeline each time we want to render a texture in this
  // render pass since some textures are rendered more than once.
  TextureCache tcache;

  // Fill the background with current background color
  auto *gs = m_Model->GetDriver()->GetGlobalState();
  auto *gds = m_Model->GetParentUI()->GetGlobalDisplaySettings();
  auto *id = m_Model->GetImageData();
  if(!id)
    return;

  SNAPAppearanceSettings *as = m_Model->GetParentUI()->GetAppearanceSettings();
  Vector3d clrBack = as->GetUIElement(SNAPAppearanceSettings::BACKGROUND_2D)->GetColor();
  context->FillViewport(clrBack);

  // The rest requires a model
  if (!m_Model->GetDriver()->IsMainImageLoaded())
    return;

  // Adjust the view matrix so that we can point in slice coordinates
  auto v_pos = m_Model->GetViewPosition();
  auto spacing = m_Model->GetSliceSpacing();

  // Get the dimensions of a non-thumbnail viewport
  Vector2ui vp_pos, vp_size;
  m_Model->GetNonThumbnailViewport(vp_pos, vp_size);

  // Are we doing linear interpolation?
  bool global_linear_mode = gds->GetGreyInterpolationMode() == GlobalDisplaySettings::LINEAR;
  double vppr = m_Model->GetSizeReporter()->GetViewportPixelRatio();

  // Here we need to keep track of the selected segmentation layer, other segmentation
  // layers should not be rendered

  // Iterate over the base layers in the viewport layout
  const SliceViewportLayout &vpl = m_Model->GetViewportLayout();
  Vector2ui szWin = m_Model->GetSizeReporter()->GetViewportSize();
  for(const auto &vp : vpl.vpList)
  {
    // Get the wrapper in this viewport
    auto *layer = id->FindLayer(vp.layer_id, false);
    if(!layer)
      continue;

    // Zoom scaling for thumbnails
    double view_zoom = m_Model->GetViewZoom();
    double thumbnail_zoom = 1.0;
    if (vp.isThumbnail)
    {
      double scale_x = vp.size[0] * 1.0 / m_Model->GetCanvasSize()[0];
      double scale_y = vp.size[1] * 1.0 / m_Model->GetCanvasSize()[1];
      thumbnail_zoom = std::max(scale_x, scale_y);
    }

    // Push the transform state
    context->PushMatrix();

    // Set the viewport
    // TODO: Currently these Viewports are reported scaled to physical pixel units
    // on high DPI displays. In the future we should change this behavior to use
    // logical pixel units. For now we account for this by hand
    context->SetViewport(vp.pos[0] / vppr, vp.pos[1] / vppr, vp.size[0] / vppr, vp.size[1] / vppr);
    context->SetLogicalWindow(0, 0, vp.size[0], vp.size[1]);

    // Apply transforms
    context->Translate(0.5 * vp.size[0], 0.5 * vp.size[1]);
    context->Scale(view_zoom * thumbnail_zoom, view_zoom * thumbnail_zoom);
    context->Translate(-v_pos[0], -v_pos[1]);
    context->Scale(spacing[0], spacing[1]);

    // Render the base layer in this viewport
    this->RenderLayer(
      context, tcache, layer, global_linear_mode, thumbnail_zoom, 1.0, DISPLAY_SLICE_MAIN);

    // If the base layer is not in thumbnail mode, draw overlays and segmentation
    if(!vp.isThumbnail)
    {
      // Draw the sticky overlays on top of this image
      for(LayerIterator it_ovl(id); !it_ovl.IsAtEnd(); ++it_ovl)
      {
        if(it_ovl.GetRole() != LABEL_ROLE && it_ovl.GetLayer()->IsSticky())
        {
          double opacity = it_ovl.GetLayer()->GetAlpha();
          if(opacity > 0)
            this->RenderLayer(
              context, tcache, it_ovl.GetLayer(), global_linear_mode, 1.0, opacity, DISPLAY_SLICE_MAIN);
        }
      }

      // Draw the current segmentation on top of this iamge
      unsigned int ssid = m_Model->GetDriver()->GetGlobalState()->GetSelectedSegmentationLayerId();
      auto *seg_layer = id->FindLayer(ssid, false, LABEL_ROLE);
      if(seg_layer)
      {
        double opacity = m_Model->GetDriver()->GetGlobalState()->GetSegmentationAlpha();
        if(opacity > 0)
          this->RenderLayer(context, tcache, seg_layer, false, 1.0, opacity, DISPLAY_SLICE_MAIN);
      }
    }

    // Draw decorators around the layer selection thumbnail, if hovered over by the mouse or selected
    bool is_hover = layer->GetUniqueId() == m_Model->GetHoveredImageLayerId();
    bool is_selected = layer->GetUniqueId() == gs->GetSelectedLayerId();
    if(vp.isThumbnail && (is_hover || is_selected))
    {
      // The element used for highlighting thumbnails
      if (is_selected && is_hover)
        context->SetPenAppearance(*as->GetUIElement(
          SNAPAppearanceSettings::LAYER_THUMBNAIL_SELECTED_AND_HOVER));
      else if (is_selected)
        context->SetPenAppearance(
          *as->GetUIElement(SNAPAppearanceSettings::LAYER_THUMBNAIL_SELECTED));
      else if (is_hover)
        context->SetPenAppearance(
          *as->GetUIElement(SNAPAppearanceSettings::LAYER_THUMBNAIL_HOVER));

      context->PushMatrix();
      context->LoadIdentity();
      context->DrawRect(0, 0, vp.size[0], vp.size[1]);
      context->PopMatrix();
    }

    // Render the various overlays (called delegates) for this tile
    for(auto *del : m_Delegates)
      del->RenderOverTiledLayer(context, layer, vp);

    // Restore the matrix
    context->PopMatrix();
  }

  // Set the viewport to the full viewport
  Vector2ui vp_full = m_Model->GetSizeReporter()->GetViewportSize();
  context->SetViewport(0, 0, vp_full[0] / vppr, vp_full[1] / vppr);
  context->SetLogicalWindow(0, 0, vp_full[0], vp_full[1]);

  if(as->GetOverallVisibility())
  {
    // Should we render the zoom thumbnail?
    if(id->IsMainLoaded() && m_Model->IsThumbnailOn())
    {
      // Draw the zoom thumbnail
      auto *eltThumb = as->GetUIElement(SNAPAppearanceSettings::ZOOM_THUMBNAIL);
      auto *eltViewport = as->GetUIElement(SNAPAppearanceSettings::ZOOM_VIEWPORT);
      if(eltThumb->GetVisible())
      {
        // Draw the outline of the zoom thumbnail
        m_Model->ComputeThumbnailProperties();
        auto xy = m_Model->GetZoomThumbnailPosition();
        auto wh = m_Model->GetZoomThumbnailSize();
        double tZoom = m_Model->GetThumbnailZoom();
        double w_main = m_Model->GetSliceSize()[0], h_main = m_Model->GetSliceSize()[1];

        // Set up the zoom thumbnail viewport
        context->PushMatrix();

        // Apply transforms
        ViewportType vp = m_Model->GetViewportLayout().vpList.front();
        context->Translate(xy[0], xy[1]);
        context->Scale(tZoom, tZoom);
        context->PushMatrix();
        context->Scale(spacing[0], spacing[1]);

        // Render the main layer as a thumbnail
        TextureInfo tex = GetTexture(context, tcache, id->GetMain(), DISPLAY_SLICE_THUMBNAIL);

        // Get the texture pointer and texture size from the cache entry
        if (id->GetMain()->IsSlicingOrthogonal())
        {
          // Draw this image using texture interpolation
          context->DrawImage(0, 0, tex.w, tex.h, tex.texture, true, 1.0);
        }
        else
        {
          // The image should be rendered directly onto the screen using the identity transform
          context->PushMatrix();
          context->LoadIdentity();
          context->Translate(xy[0], xy[1]);
          context->DrawImage(0, 0, tex.w, tex.h, tex.texture, true, 1.0);
          context->PopMatrix();
        }

        // Draw the outline of the thumbnail
        context->SetPenAppearance(*eltThumb);
        context->DrawRect(0, 0, w_main, h_main);

        // Undo the scaling by spacing
        context->PopMatrix();

        // Draw the viewport rectangle
        if(eltViewport->GetVisible())
        {
          // Draw a box representing the current zoom level
          context->Translate(v_pos[0], v_pos[1]);
          double w_vp = m_Model->GetCanvasSize()[0] * 0.5 / m_Model->GetViewZoom();
          double h_vp = m_Model->GetCanvasSize()[1] * 0.5 / m_Model->GetViewZoom();
          context->SetPenAppearance(*eltViewport);
          context->DrawRect(-w_vp, -h_vp, 2 * w_vp, 2 * h_vp);
        }

        context->PopMatrix();
      }
    }

    // Render the various overlays (called delegates) for this tile
    for (auto *del : m_Delegates)
      del->RenderOverMainViewport(context);
  }
}

void
GenericSliceRenderer::SetDelegates(const RendererDelegateList &ovl)
{
  m_Delegates = ovl;
}

void
GenericSliceRenderer::OnUpdate()
{
  // Make sure the model has been updated first
  m_Model->Update();

         // Also make sure to update the model zoom coordinator (this is confusing)
  m_Model->GetParentUI()->GetSliceCoordinator()->Update();

         // Also make sure to update the display layout model
  m_Model->GetParentUI()->GetDisplayLayoutModel()->Update();

         // Check what events have occurred
  bool appearance_settings_changed =
    m_EventBucket->HasEvent(ChildPropertyChangedEvent(),
                            m_Model->GetParentUI()->GetAppearanceSettings()) ||
    m_EventBucket->HasEvent(ValueChangedEvent(),
                            m_Model->GetParentUI()->GetAppearanceSettings()->GetOverallVisibilityModel());

  bool segmentation_opacity_changed =
    m_EventBucket->HasEvent(ValueChangedEvent(),
                            m_Model->GetParentUI()->GetGlobalState()->GetSegmentationAlphaModel());

  bool display_setting_changed =
    m_EventBucket->HasEvent(ValueChangedEvent(),
                            m_Model->GetParentUI()->GetGlobalDisplaySettings()->GetGreyInterpolationModeModel());

  bool layers_changed =
    m_EventBucket->HasEvent(LayerChangeEvent());

  bool layer_layout_changed =
    m_EventBucket->HasEvent(DisplayLayoutModel::LayerLayoutChangeEvent());

  bool layer_metadata_changed =
    m_EventBucket->HasEvent(WrapperMetadataChangeEvent());

  bool layer_mapping_changed =
    m_EventBucket->HasEvent(WrapperDisplayMappingChangeEvent());

  bool layer_visibility_changed =
    m_EventBucket->HasEvent(WrapperVisibilityChangeEvent());

  bool zoom_pan_changed =
    m_EventBucket->HasEvent(ModelUpdateEvent(), m_Model);

  // Which layer is currently selected
  bool selected_layer_changed =
    m_EventBucket->HasEvent(ValueChangedEvent(),
                            m_Model->GetDriver()->GetGlobalState()->GetSelectedLayerIdModel());

  bool selected_segmentation_changed =
    m_EventBucket->HasEvent(ValueChangedEvent(),
                            m_Model->GetDriver()->GetGlobalState()->GetSelectedSegmentationLayerIdModel());
  /*
  if(layers_changed)
  {
    this->UpdateLayerAssemblies();
  }

  if(layers_changed || layer_layout_changed || selected_layer_changed || selected_segmentation_changed)
  {
    this->UpdateRendererLayout();
  }

  if(layers_changed || layer_mapping_changed || segmentation_opacity_changed || layer_visibility_changed || display_setting_changed)
  {
    this->UpdateLayerApperances();
  }

  if(appearance_settings_changed)
  {
    this->UpdateSceneAppearanceSettings();
  }

  if(layers_changed || layer_layout_changed || zoom_pan_changed || layer_mapping_changed || layer_visibility_changed || appearance_settings_changed)
  {
    this->UpdateRendererCameras();
    this->UpdateZoomPanThumbnail();
  }
*/
}
