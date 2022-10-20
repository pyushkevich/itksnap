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
#include <itkImageLinearConstIteratorWithIndex.h>


#include <vtkTexture.h>
#include <vtkTexturedActor2D.h>
#include <vtkRenderer.h>
#include <vtkActor2D.h>
#include <vtkActor.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkTextActor.h>
#include <vtkRenderWindow.h>
#include <vtkRendererCollection.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkPointData.h>
#include <vtkFloatArray.h>
#include <vtkQuad.h>
#include <vtkProperty.h>
#include <vtkCamera.h>
#include <vtkProperty2D.h>
#include <vtkMapper2D.h>
#include <vtkPolyDataMapper2D.h>
#include <itkRGBAPixel.h>
#include "SNAPExportITKToVTK.h"
#include "TexturedRectangleAssembly.h"
#include "GenericSliceContextItem.h"

#include <vtkContextItem.h>
#include <vtkContext2D.h>
#include <vtkContextDevice2D.h>
#include <vtkContextActor.h>
#include <vtkContextScene.h>
#include <vtkPen.h>
#include <vtkBrush.h>
#include <vtkTransform2D.h>
#include <vtkContextTransform.h>



/**
 * @brief Context item that draws a box around the layer thumbnail
 */
class LayerThumbnailDecorator : public GenericSliceContextItem
{
public:
  vtkTypeMacro(LayerThumbnailDecorator, GenericSliceContextItem)

  static LayerThumbnailDecorator *New();

  irisSetMacro(Layer, ImageWrapperBase *);
  irisGetMacro(Layer, ImageWrapperBase *);

  virtual bool Paint(vtkContext2D *painter) override
  {
    float w = this->GetScene()->GetSceneWidth();
    float h = this->GetScene()->GetSceneHeight();

    // Determine if the current layer is hovered over by the mouse
    bool is_hover = m_Layer->GetUniqueId() == m_Model->GetHoveredImageLayerId();
    bool is_selected = m_Layer->GetUniqueId() == m_Model->GetDriver()->GetGlobalState()->GetSelectedLayerId();
    if(!is_hover && !is_selected)
      return true;

    if(is_selected && is_hover)
      painter->GetPen()->SetColorF(1.0, 1.0, 0.5);
    else if(is_selected)
      painter->GetPen()->SetColorF(1.0, 0.9, 0.1);
    else if(is_hover)
      painter->GetPen()->SetColorF(0.6, 0.54, 0.46);

    painter->GetPen()->SetOpacityF(0.75);
    painter->GetPen()->SetLineType(vtkPen::SOLID_LINE);
    painter->GetPen()->SetWidth(3 * this->GetVPPR());

    // Use drawpoly method because DrawRect includes a fill
    this->DrawRectNoFill(painter, 0, 0, w, h);
    return true;
  }

private:
  ImageWrapperBase * m_Layer;
};

vtkStandardNewMacro(LayerThumbnailDecorator);

/**
 * @brief Context item that draws zoom thumbnail outline and viewport
 */
class ZoomThumbnailDecorator : public GenericSliceContextItem
{
public:
  vtkTypeMacro(ZoomThumbnailDecorator, GenericSliceContextItem)

  static ZoomThumbnailDecorator *New();

  virtual bool Paint(vtkContext2D *painter) override
  {
    // Thumbnail must be on
    if(!m_Model->IsThumbnailOn())
      return false;

    // Get the thumbnail appearance properties
    SNAPAppearanceSettings *as = m_Model->GetParentUI()->GetAppearanceSettings();

    const OpenGLAppearanceElement *eltThumb =
        as->GetUIElement(SNAPAppearanceSettings::ZOOM_THUMBNAIL);

    const OpenGLAppearanceElement *eltViewport =
        as->GetUIElement(SNAPAppearanceSettings::ZOOM_VIEWPORT);

    // Update the thumbnail info (TODO: this is redundant!)
    m_Model->ComputeThumbnailProperties();
    auto xy = m_Model->GetZoomThumbnailPosition();
    auto wh = m_Model->GetZoomThumbnailSize();

    // Draw the box around the thumbnail
    if(eltThumb->GetVisible())
      {
      ApplyAppearanceSettingsToPen(painter, eltThumb);
      DrawRectNoFill(painter, xy[0], xy[1], xy[0] + wh[0], xy[1] + wh[1]);
      }

    if(eltViewport->GetVisible())
      {
      // Radius of viewport in image voxel units
      double wv = m_Model->GetCanvasSize()[0] * 0.5 / m_Model->GetViewZoom();
      double hv = m_Model->GetCanvasSize()[1] * 0.5 / m_Model->GetViewZoom();

      // Radius of viewport in display pixel units
      double wp = wv * m_Model->GetThumbnailZoom();
      double hp = hv * m_Model->GetThumbnailZoom();

      // Center of viewport in image voxel units
      double xv = m_Model->GetViewPosition()[0];
      double yv = m_Model->GetViewPosition()[1];

      // Center of viewport in display pixel units
      double xp = xv * m_Model->GetThumbnailZoom() + xy[0];
      double yp = yv * m_Model->GetThumbnailZoom() + xy[1];

      ApplyAppearanceSettingsToPen(painter, eltViewport);
      DrawRectNoFill(painter, xp - wp, yp - hp, xp + wp, yp + hp);
      }

    // Draw the box around the viewport
    return true;
  }

};

vtkStandardNewMacro(ZoomThumbnailDecorator);



GenericSliceRenderer
::GenericSliceRenderer()
{
  this->m_DrawingZoomThumbnail = false;
  this->m_DrawingLayerThumbnail = false;
  this->m_DrawingViewportIndex = -1;


  m_BackgroundRenderer = vtkSmartPointer<vtkRenderer>::New();
  m_BackgroundRenderer->SetLayer(0);

  m_OverlayRenderer = vtkSmartPointer<vtkRenderer>::New();
  m_OverlayRenderer->SetLayer(2);
  m_OverlayRenderer->GetActiveCamera()->ParallelProjectionOn();
  m_OverlayRenderer->GetActiveCamera()->SetParallelScale(1.0);

  m_ZoomThumbnail = vtkSmartPointer<TexturedRectangleAssembly2D>::New();
  m_ZoomThumbnail->GetActor()->GetProperty()->SetColor(1, 1, 0);
  m_OverlayRenderer->AddActor2D(m_ZoomThumbnail->GetActor());

  m_OverlaySceneActor = vtkSmartPointer<vtkContextActor>::New();
  m_OverlayRenderer->AddActor2D(m_OverlaySceneActor);
}

void
GenericSliceRenderer::SetModel(GenericSliceModel *model)
{
  this->m_Model = model;

  // Set the keys to access user data
  static const char * key_lta[] = {
    "LayerTextureAssembly_0",
    "LayerTextureAssembly_1",
    "LayerTextureAssembly_2"
  };
  m_KeyLayerTextureAssembly = key_lta[model->GetId()];

  static const char * key_bla[] = {
    "BaseLayerAssembly_0",
    "BaseLayerAssembly_1",
    "BaseLayerAssembly_2"
  };
  m_KeyBaseLayerAssembly = key_bla[model->GetId()];

  // Record and rebroadcast changes in the model
  Rebroadcast(m_Model, ModelUpdateEvent(), ModelUpdateEvent());

  // Respond to changes in image dimension - these require big updates
  Rebroadcast(m_Model->GetDriver(), LayerChangeEvent(), ModelUpdateEvent());

  // Also listen to events on opacity
  Rebroadcast(m_Model->GetParentUI()->GetGlobalState()->GetSegmentationAlphaModel(),
              ValueChangedEvent(), AppearanceUpdateEvent());

  // Listen to changes in the appearance of any of the wrappers
  Rebroadcast(m_Model->GetDriver(), WrapperChangeEvent(), ModelUpdateEvent());

  // Listen to changes to the segmentation
  Rebroadcast(m_Model->GetDriver(), SegmentationChangeEvent(), ModelUpdateEvent());

  // Changes to cell layout also must be rebroadcast
  DisplayLayoutModel *dlm = m_Model->GetParentUI()->GetDisplayLayoutModel();
  Rebroadcast(dlm, DisplayLayoutModel::LayerLayoutChangeEvent(),
              ModelUpdateEvent());

  // Listen to changes in appearance
  Rebroadcast(m_Model->GetParentUI()->GetAppearanceSettings(),
              ChildPropertyChangedEvent(), ModelUpdateEvent());

  Rebroadcast(m_Model->GetParentUI()->GetAppearanceSettings()->GetOverallVisibilityModel(),
              ValueChangedEvent(), ModelUpdateEvent());

  // Listen to overall visibility of overlaps
  Rebroadcast(m_Model->GetParentUI()->GetAppearanceSettings()->GetOverallVisibilityModel(),
              ValueChangedEvent(), ModelUpdateEvent());

  // Paintbrush appearance changes
  PaintbrushSettingsModel *psm = m_Model->GetParentUI()->GetPaintbrushSettingsModel();
  Rebroadcast(psm->GetBrushSizeModel(), ValueChangedEvent(), ModelUpdateEvent());

  // Which layer is currently selected
  Rebroadcast(m_Model->GetDriver()->GetGlobalState()->GetSelectedLayerIdModel(),
              ValueChangedEvent(), ModelUpdateEvent());

  Rebroadcast(m_Model->GetDriver()->GetGlobalState()->GetSelectedSegmentationLayerIdModel(),
              ValueChangedEvent(), ModelUpdateEvent());

  Rebroadcast(m_Model->GetHoveredImageLayerIdModel(), ValueChangedEvent(), ModelUpdateEvent());
  Rebroadcast(m_Model->GetHoveredImageIsThumbnailModel(), ValueChangedEvent(), ModelUpdateEvent());

	Rebroadcast(m_Model->GetParentUI()->GetGlobalDisplaySettings()->GetGreyInterpolationModeModel(),
							ValueChangedEvent(), ModelUpdateEvent());
}

void GenericSliceRenderer::UpdateSceneAppearanceSettings()
{
  // Get the appearance settings pointer since we use it a lot
  SNAPAppearanceSettings *as =
      m_Model->GetParentUI()->GetAppearanceSettings();

  // Get the properties for the background color
  Vector3d clrBack = as->GetUIElement(
      SNAPAppearanceSettings::BACKGROUND_2D)->GetColor();

  // Assign to the background renderer
  m_BackgroundRenderer->SetBackground(clrBack.data_block());
}

void GenericSliceRenderer::OnUpdate()
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
}

void GenericSliceRenderer::SetRenderWindow(vtkRenderWindow *rwin)
{
  m_RenderWindow = rwin;
  m_RenderWindow->SetNumberOfLayers(3);
}



void GenericSliceRenderer::UpdateLayerAssemblies()
{
  // Synchronize the layer assemblies with available renderers
  GenericImageData *id = m_Model->GetDriver()->GetCurrentImageData();

  // For each layer either copy a reference to an existing assembly
  // or create a new assembly
  for(LayerIterator it = id->GetLayers(); !it.IsAtEnd(); ++it)
    {
    if(!GetLayerTextureAssembly(it.GetLayer()))
      {
      SmartPtr<LayerTextureAssembly> lta = LayerTextureAssembly::New();
      it.GetLayer()->SetUserData(m_KeyLayerTextureAssembly, lta);

      // Get the pointer to the display slice
      auto *ds = it.GetLayer()->GetDisplaySlice(m_Model->GetId()).GetPointer();

      // Configure the texture pipeline
      SmartPtr<LayerTextureAssembly::VTKExporter> exporter = LayerTextureAssembly::VTKExporter::New();
      exporter->SetInput(ds);

      lta->m_Exporter = exporter.GetPointer();
      lta->m_Importer = vtkSmartPointer<vtkImageImport>::New();
      ConnectITKExporterToVTKImporter(exporter.GetPointer(), lta->m_Importer);

      lta->m_Texture = vtkSmartPointer<vtkTexture>::New();
      lta->m_Texture->SetInputConnection(lta->m_Importer->GetOutputPort());

      // Get the corners of the slice
      auto sc = m_Model->GetSliceCorners();
      auto c0 = sc.first, c1 = sc.second;

      // Create a polydata for the image
      lta->m_ImageRect = vtkSmartPointer<TexturedRectangleAssembly>::New();
      lta->m_ImageRect->SetCorners(c0[0], c0[1], c1[0], c1[1]);
      lta->m_ImageRect->GetActor()->SetTexture(lta->m_Texture);
      }
    }

  // Now iterate over the base layers only and set up the base layer assemblies, which
  // consist of the base layer, sticky overlays, and segmentation layer
  for(LayerIterator it = id->GetLayers(); !it.IsAtEnd(); ++it)
    {
    // If this is a base layer (something drawn on its own), it gets a pair of renderers
    if(it.GetRole() == MAIN_ROLE || it.GetRole() == OVERLAY_ROLE || it.GetRole() == SNAP_ROLE)
      {
      if(!GetBaseLayerAssembly(it.GetLayer()))
        {
        SmartPtr<BaseLayerAssembly> bla = BaseLayerAssembly::New();
        it.GetLayer()->SetUserData(m_KeyBaseLayerAssembly, bla);

        // Create the renderers
        bla->m_Renderer = vtkSmartPointer<vtkRenderer>::New();
        bla->m_Renderer->SetLayer(1);
        bla->m_ThumbRenderer = vtkSmartPointer<vtkRenderer>::New();
        bla->m_ThumbRenderer->SetLayer(1);

        // Set camera properties
        bla->m_Renderer->GetActiveCamera()->ParallelProjectionOn();
        bla->m_ThumbRenderer->GetActiveCamera()->ParallelProjectionOn();

        // Create the context scene actor and transform item
        bla->m_OverlayContextActor = vtkSmartPointer<vtkContextActor>::New();
        bla->m_OverlayContextTransform = vtkSmartPointer<vtkContextTransform>::New();
        bla->m_OverlayContextActor->GetScene()->AddItem(bla->m_OverlayContextTransform);

        // Configure the context scene
        this->UpdateTiledOverlayContextSceneItems(it.GetLayer());
        this->UpdateTiledOverlayContextSceneTransform(it.GetLayer());

        // Create highlight around the thumbnail.
        vtkNew<LayerThumbnailDecorator> thumb_border;
        thumb_border->SetModel(m_Model);
        thumb_border->SetLayer(it.GetLayer());

        bla->m_ThumbnailDecoratorActor = vtkSmartPointer<vtkContextActor>::New();
        bla->m_ThumbnailDecoratorActor->GetScene()->AddItem(thumb_border);
        }
      }
    else
      {
      it.GetLayer()->SetUserData(m_KeyBaseLayerAssembly, nullptr);
      }
    }

  // Update the appearance settings
  this->UpdateSceneAppearanceSettings();

  // Assign the main image texture to the zoom thumbnail
  m_ZoomThumbnail->GetActor()->SetTexture(
        id->IsMainLoaded() ? GetLayerTextureAssembly(id->GetMain())->m_Texture : nullptr);
}

GenericSliceRenderer::LayerTextureAssembly *
GenericSliceRenderer::GetLayerTextureAssembly(ImageWrapperBase *wrapper)
{
  auto *userdata = wrapper->GetUserData(m_KeyLayerTextureAssembly);
  return dynamic_cast<LayerTextureAssembly *>(userdata);
}

GenericSliceRenderer::BaseLayerAssembly *
GenericSliceRenderer::GetBaseLayerAssembly(ImageWrapperBase *wrapper)
{
  auto *userdata = wrapper->GetUserData(m_KeyBaseLayerAssembly);
  return dynamic_cast<BaseLayerAssembly *>(userdata);
}

void GenericSliceRenderer::SetDepth(vtkActor *actor, double z)
{
  double *p = actor->GetPosition();
  actor->SetPosition(p[0], p[1], z);
}

void GenericSliceRenderer::UpdateLayerDepth()
{
  double depth_ovl = DEPTH_OVERLAY_START;
  double depth_seg = DEPTH_SEGMENTATION_START;

  for(LayerIterator it = m_Model->GetImageData()->GetLayers(); !it.IsAtEnd(); ++it)
    {
    auto *la = GetLayerTextureAssembly(it.GetLayer());
    double depth = 0.0;

    if(it.GetRole() == LABEL_ROLE)
      {
      // All segmentation layers get assigned the same depth because we are
      // currently not supporting rendering of multiple segmentation layers
      // at the same time with transparency
      depth = depth_seg;
      }
    else if(it.GetLayer()->IsSticky())
      {
      // Overlays are placed at increasing values of z
      depth = depth_ovl;
      depth_ovl += DEPTH_STEP;
      }

    SetDepth(la->m_ImageRect->GetActor(), depth);
    }
}

void GenericSliceRenderer::UpdateZoomPanThumbnail()
{
  if(m_Model->GetDriver()->IsMainImageLoaded())
    {
    // Update the geometry of the zoom thumbnail
    m_Model->ComputeThumbnailProperties();
    auto pos = m_Model->GetZoomThumbnailPosition();
    auto size = m_Model->GetZoomThumbnailSize();

    m_ZoomThumbnail->SetCorners(pos[0], pos[1], pos[0]+size[0], pos[1]+size[1]);
    m_ZoomThumbnail->GetActor()->SetVisibility(
          m_Model->IsThumbnailOn() &&
          m_Model->GetParentUI()->GetAppearanceSettings()->GetOverallVisibility());
    }
  else
    {
    m_ZoomThumbnail->GetActor()->SetVisibility(false);
    }
}

void GenericSliceRenderer
::UpdateTiledOverlayContextSceneTransform(ImageWrapperBase *wrapper)
{
}


void GenericSliceRenderer
::UpdateTiledOverlayContextSceneItems(ImageWrapperBase *wrapper)
{
  // Get the scene actor
  auto top_item = GetBaseLayerAssembly(wrapper)->m_OverlayContextTransform;

  // Clear the scene
  top_item->ClearItems();

  // Iterate over the delegates
  for(auto it : m_Delegates)
    {
    it->AddContextItemsToTiledOverlay(top_item, wrapper);
    }
}

void GenericSliceRenderer::UpdateRendererLayout()
{
  if(!m_RenderWindow)
    return;

  // Update the depths of the layers
  this->UpdateLayerDepth();

  // Here we need to keep track of the selected segmentation layer, other segmentation
  // layers should not be rendered
  unsigned int ssid = m_Model->GetDriver()->GetGlobalState()->GetSelectedSegmentationLayerId();

  // Create a sorted structure of layers that are rendered on top of the base
  std::map<double, vtkActor *> depth_map;
  for(LayerIterator it = m_Model->GetImageData()->GetLayers(); !it.IsAtEnd(); ++it)
    {
    // Don't display segmentation layer if it is not the selected one
    if(it.GetRole() == LABEL_ROLE && it.GetLayer()->GetUniqueId() != ssid)
      continue;

    auto *lta = GetLayerTextureAssembly(it.GetLayer());
    if(lta)
      {
      auto *actor = lta->m_ImageRect->GetActor();
      double z = actor->GetPosition()[2];
      if(z > 0.0)
        depth_map[z] = actor;
      }
    }

  // Get the viewport layout
  const SliceViewportLayout &vpl = m_Model->GetViewportLayout();

  // Remove all the renderers from the current window
  m_RenderWindow->GetRenderers()->RemoveAllItems();
  m_RenderWindow->AddRenderer(m_BackgroundRenderer);

  // Get the dimensions of the render window
  Vector2ui szWin = m_Model->GetSizeReporter()->GetViewportSize();

  // Draw each viewport in turn. For now, the number of z-layers is hard-coded at 2
  for(unsigned int k = 0; k < vpl.vpList.size(); k++)
    {
    // Get the current viewport
    const SliceViewportLayout::SubViewport &vp = m_Model->GetViewportLayout().vpList[k];

    // Get the wrapper in this viewport
    auto *layer = m_Model->GetImageData()->FindLayer(vp.layer_id, false);
    if(!layer)
      continue;

    // Get the assembly for this layer
    BaseLayerAssembly *bla = GetBaseLayerAssembly(layer);

    // Get the renderer that is referenced by this viewport
    vtkRenderer *renderer = vp.isThumbnail ? bla->m_ThumbRenderer : bla->m_Renderer;

    // Create a VTK renderer for this viewport
    Vector2d rel_pos[2];
    for(unsigned int d = 0; d < 2; d++)
      {
      rel_pos[0][d] = vp.pos[d] * 1.0 / szWin[d];
      rel_pos[1][d] = rel_pos[0][d] + vp.size[d] * 1.0 / szWin[d];
      }

    // Set the renderer viewport
    renderer->SetViewport(rel_pos[0][0], rel_pos[0][1], rel_pos[1][0], rel_pos[1][1]);

    // Set up the actors shown in this renderer
    renderer->RemoveAllViewProps();

    // Add the base layer actor
    renderer->AddActor(GetLayerTextureAssembly(layer)->m_ImageRect->GetActor());

    // Some stuff only gets added to the main renderer
    if(vp.isThumbnail)
      {
      // Add the thumbnail highlight
      renderer->AddActor(bla->m_ThumbnailDecoratorActor);
      }
    else
      {
      // Add the overlay layer actors
      for(auto it : depth_map)
        renderer->AddActor(it.second);

      // Add the tiled overlay scene actor
      renderer->AddActor(bla->m_OverlayContextActor);
      }

    // Add the renderer to the window
    m_RenderWindow->AddRenderer(renderer);
    }

  // Add the overlay renderer
  m_RenderWindow->AddRenderer(m_OverlayRenderer);
}

void GenericSliceRenderer::UpdateRendererCameras()
{
	// Preference settings change can trigger this update
	// Add this check to prevent segfault when main image is not loaded
	if (!m_Model->GetDriver()->IsMainImageLoaded())
		return;

  // Get the transform parameters
  auto v_zoom = m_Model->GetViewZoom();
  auto v_pos = m_Model->GetViewPosition();
  auto spacing = m_Model->GetSliceSpacing();

  // Get the dimensions of a non-thumbnail viewport
  Vector2ui vp_pos, vp_size;
  m_Model->GetNonThumbnailViewport(vp_pos, vp_size);

  for(LayerIterator it = m_Model->GetImageData()->GetLayers(); !it.IsAtEnd(); ++it)
    {
    auto *bla = GetBaseLayerAssembly(it.GetLayer());
    if(bla)
      {
      // Update the camera for the main renderer
      auto ren = bla->m_Renderer;
      auto rs = ren->GetSize();
      ren->GetActiveCamera()->SetFocalPoint(rs[0] * 0.5, rs[1] * 0.5, 0.0);
      ren->GetActiveCamera()->SetPosition(rs[0] * 0.5, rs[1] * 0.5, 2.0);
      ren->GetActiveCamera()->SetViewUp(0.0, 1.0, 0.0);
      ren->GetActiveCamera()->SetParallelScale(0.5 * rs[1]);

      // Update the camera for the thumbnail renderer
      auto tren = bla->m_ThumbRenderer;
      tren->GetActiveCamera()->SetFocalPoint(vp_size[0] * 0.5, vp_size[1] * 0.5, 0.0);
      tren->GetActiveCamera()->SetPosition(vp_size[0] * 0.5, vp_size[1] * 0.5, 2.0);
      tren->GetActiveCamera()->SetViewUp(0.0, 1.0, 0.0);
      tren->GetActiveCamera()->SetParallelScale(0.5 * vp_size[1]);

      // Also update the transform for the overlay scene
      auto *tform = bla->m_OverlayContextTransform->GetTransform();

      tform->Identity();
      tform->Translate(ren->GetSize()[0] * 0.5, ren->GetSize()[1] * 0.5);
      tform->Scale(v_zoom, v_zoom);
      tform->Translate(-v_pos[0], -v_pos[1]);
      tform->Scale(spacing[0], spacing[1]);
      }

    auto *lta = GetLayerTextureAssembly(it.GetLayer());
    if(lta)
      {
      auto sz = m_Model->GetViewportLayout().vpList.front().size;
      if(it.GetLayer()->IsSlicingOrthogonal())
        {
        // Map the corners of the slice into the viewport coordinates
        auto sc = m_Model->GetSliceCornersInWindowCoordinates();
        lta->m_ImageRect->SetCorners(sc.first[0], sc.first[1], sc.second[0], sc.second[1]);
        }
      else
        {
        // Nonorthogonal slicing means we render into the whole viewport
        lta->m_ImageRect->SetCorners(0, 0, sz[0], sz[1]);
        }
      }
    }
}

void GenericSliceRenderer::UpdateLayerApperances()
{
  // Iterate over the layers
  for(LayerIterator it = m_Model->GetImageData()->GetLayers(); !it.IsAtEnd(); ++it)
    {
    // Does this layer use transparency?
    double alpha = 1.0;
    if(it.GetRole() == LABEL_ROLE)
      alpha = m_Model->GetDriver()->GetGlobalState()->GetSegmentationAlpha();
    else if(it.GetLayer()->IsSticky())
      alpha = it.GetLayer()->GetAlpha();

    auto *lta = GetLayerTextureAssembly(it.GetLayer());
    if(lta)
			{
			// Configure the texture interpolation
			const GlobalDisplaySettings *gds = m_Model->GetParentUI()->GetGlobalDisplaySettings();
			if (gds->GetGreyInterpolationMode() == GlobalDisplaySettings::LINEAR)
				lta->m_Texture->InterpolateOn();
			else
				lta->m_Texture->InterpolateOff();

			// Set the alpha for the actor
			lta->m_ImageRect->GetActor()->GetProperty()->SetOpacity(alpha);
			}
    }
}

const GenericSliceRenderer::ViewportType *
GenericSliceRenderer
::GetDrawingViewport() const
{
  if(m_DrawingViewportIndex < 0)
    return NULL;
  else
    return &m_Model->GetViewportLayout().vpList[m_DrawingViewportIndex];

}

void GenericSliceRenderer::SetDelegates(const RendererDelegateList &ovl)
{
  m_Delegates = ovl;

  // Assign the tiled delegates to their corresponding tiles
  auto *id = m_Model->GetDriver()->GetCurrentImageData();
  for(LayerIterator it = id->GetLayers(); !it.IsAtEnd(); ++it)
    {
    if(auto *bla = GetBaseLayerAssembly(it.GetLayer()))
      {
      this->UpdateTiledOverlayContextSceneItems(it.GetLayer());
      }
    }


  // Assign global delegates
  m_OverlaySceneActor->GetScene()->ClearItems();

  // Add zoom decorator
  vtkNew<ZoomThumbnailDecorator> ztdec;
  ztdec->SetModel(m_Model);
  m_OverlaySceneActor->GetScene()->AddItem(ztdec);

  // Add all others
  for(auto del : m_Delegates)
    {
    del->AddContextItemsToGlobalOverlayScene(m_OverlaySceneActor->GetScene());
    }
}

bool GenericSliceRenderer::IsTiledMode() const
{
  DisplayLayoutModel *dlm = m_Model->GetParentUI()->GetDisplayLayoutModel();
  Vector2ui layout = dlm->GetSliceViewLayerTilingModel()->GetValue();
  return layout[0] > 1 || layout[1] > 1;
}

