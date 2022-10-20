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

#include <AbstractVTKRenderer.h>
#include <GenericSliceModel.h>
#include <ImageWrapper.h>
#include <list>
#include <map>
#include <LayerAssociation.h>

class vtkTexture;
class vtkImageImport;
class vtkActor;
class vtkActor2D;
class vtkPolyData;
class vtkPolyDataMapper;
class vtkTexturedActor;
class GenericSliceRenderer;
class vtkContextActor;
class vtkContextScene;
class vtkContextTransform;
class vtkAbstractContextItem;

namespace itk {
template <typename TInputImage> class VTKImageExport;
}

class TexturedRectangleAssembly;
class TexturedRectangleAssembly2D;

/**
 * @brief The parent class for overlays that are placed on top of the image
 * layers, i.e., crosshairs, etc.
 */

// TODO: make child of AbstractModel and rename to Generic... for consistency
class SliceRendererDelegate : public AbstractRenderer
{
public:
  virtual ~SliceRendererDelegate() {}

  irisGetSetMacro(ParentRenderer, GenericSliceRenderer *)

  /**
   * This method should be overridden by child class to add any context items
   * that should be displayed on top of the tiled images to the ContextScene.
   *
   * The ContextScene will have the transform set to use the slice coordinate
   * system
   */
  virtual void AddContextItemsToTiledOverlay(
      vtkAbstractContextItem *parent, ImageWrapperBase *base_layer) {};

  /**
   * This method should be overridden by child class to add any context items
   * that should be displayed as global overlays to the ContextScene.
   *
   * The ContextScene will have the transform set to use the viewport coordinate
   * system
   */
  virtual void AddContextItemsToGlobalOverlayScene(vtkContextScene *) {}


protected:
  GenericSliceRenderer *m_ParentRenderer;
};

class GenericSliceRenderer : public AbstractVTKRenderer
{
public:

  irisITKObjectMacro(GenericSliceRenderer, AbstractModel)

  FIRES(ModelUpdateEvent)

  void SetModel(GenericSliceModel *model);

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
  const RendererDelegateList &GetDelegates() const
    { return m_Delegates; }

  void SetDelegates(const RendererDelegateList &ovl);

  // A callback for when the model is reinitialized
  // void OnModelReinitialize();

  // Set list of child renderers
  void SetChildRenderers(std::list<AbstractRenderer *> renderers);

protected:

  // Constants for layer depth ordering
  const double DEPTH_OVERLAY_START = 0.01;
  const double DEPTH_SEGMENTATION_START = 0.02;
  const double DEPTH_STEP = 0.0001;

  GenericSliceRenderer();
  virtual ~GenericSliceRenderer() {}

  void OnUpdate() override;

  virtual void SetRenderWindow(vtkRenderWindow *rwin) override;

  /**
   * A data structure describing VTK objects (renderers, actors, etc) that
   * are associated with a base image layer.
   */
  class BaseLayerAssembly : public AbstractModel
  {
  public:
    irisITKObjectMacro(GenericSliceRenderer::BaseLayerAssembly, AbstractModel)

    // The main and thumbnail renderers for this tile
    vtkSmartPointer<vtkRenderer> m_Renderer, m_ThumbRenderer;

    // The context scene actor used to display overlays on top of images.
    vtkSmartPointer<vtkContextActor> m_OverlayContextActor;

    // The top-level context item whose transform is set so that the
    // drawing operations can use the slice coordinate system
    vtkSmartPointer<vtkContextTransform> m_OverlayContextTransform;

    // Rectangle highlighting the thumbnail
    vtkSmartPointer<vtkContextActor> m_ThumbnailDecoratorActor;

  protected:
    BaseLayerAssembly() {}
    virtual ~BaseLayerAssembly() {}
  };

  /**
   * A data structure describing VTK objects associated with every image layer
   * including textures
   */
  class LayerTextureAssembly : public AbstractModel
  {
  public:
    irisITKObjectMacro(GenericSliceRenderer::LayerTextureAssembly, AbstractModel)

    typedef itk::VTKImageExport<ImageWrapperBase::DisplaySliceType> VTKExporter;

    // Exporter from ITK to VTK
    SmartPtr<itk::Object> m_Exporter;

    // Importer from ITK to VTK
    vtkSmartPointer<vtkImageImport> m_Importer;

    // Texture algorithm
    vtkSmartPointer<vtkTexture> m_Texture;

    // Actor used to draw the layer
    vtkSmartPointer<TexturedRectangleAssembly> m_ImageRect;

  protected:
    LayerTextureAssembly() {}
    virtual ~LayerTextureAssembly() {}
  };

  // Get a layer assembly for a layer
  LayerTextureAssembly *GetLayerTextureAssembly(ImageWrapperBase *wrapper);

  // Get a layer assembly for a layer
  BaseLayerAssembly *GetBaseLayerAssembly(ImageWrapperBase *wrapper);

  // A renderer placed on top of the tiles
  vtkSmartPointer<vtkRenderer> m_BackgroundRenderer, m_OverlayRenderer;

  // An actor holding the zoom thumbnail
  vtkSmartPointer<TexturedRectangleAssembly2D> m_ZoomThumbnail;

  // An actor holding the global overlays including zoom thumb
  vtkSmartPointer<vtkContextActor> m_OverlaySceneActor;

  // Update the renderers in response to a change in number of layers
  void UpdateLayerAssemblies();

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

  GenericSliceModel *m_Model;

  // Whether rendering to thumbnail or not
  bool m_DrawingZoomThumbnail, m_DrawingLayerThumbnail;

  // Keys to access user data for this renderer from layers
  std::string m_KeyLayerTextureAssembly, m_KeyBaseLayerAssembly;

  // The index of the viewport that is currently being drawn - for use in child renderers
  int m_DrawingViewportIndex;

  // A list of overlays that the user can configure
  RendererDelegateList m_Delegates;

  void UpdateSceneAppearanceSettings();
  void SetDepth(vtkActor *actor, double z);
};



#endif // GENERICSLICERENDERER_H
