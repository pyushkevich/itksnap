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

#ifndef GENERICSLICEMODEL_H
#define GENERICSLICEMODEL_H

#include <SNAPCommon.h>
#include <ImageCoordinateTransform.h>
#include <OpenGLSliceTexture.h>
#include <SNAPEvents.h>
#include "AbstractModel.h"
#include "ImageWrapper.h"
#include "UIReporterDelegates.h"
#include "PropertyModel.h"


class GlobalUIModel;
class IRISApplication;
class GenericImageData;
class GenericSliceModel;

// An event fired when the geometry of the slice view changes
itkEventMacro(SliceModelImageDimensionsChangeEvent, IRISEvent)
itkEventMacro(SliceModelGeometryChangeEvent, IRISEvent)


/**
 * A structure describing viewport organization in a SNAP slice view.
 * The viewport can be in a tiled state or in a main/thumbnail state.
 *
 * This class contains a list of viewports with corresponding IDs, as
 * well as the 'primary' viewport, based on which zoom computations are
 * made. The primary viewport is always the first viewport in the list
 */
struct SliceViewportLayout
{
public:
  struct SubViewport {
    // Size and position of the viewport
    Vector2ui pos, size;

    // Index of the associated image layer
    unsigned long layer_id;

    // Whether this is a thumbnail sub-view or a primary view
    bool isThumbnail;
  };

  // List of subviewports
  std::vector<SubViewport> vpList;
};

/**
  \class GenericSliceModel
  \brief Describes the state of the slice panel showing an orthogonal
  projection of a dataset in ITK-SNAP

  This class holds the state of the slice viewer widget. It contains
  information about the slice currently being shown, the mapping of
  coordinates between slice space and image space, and other bits of
  information. Ideally, you should be able to store and retrieve the
  state of this object between sessions.

  This class is meant to be used with arbitrary GUI environments. It is
  unaware of Qt, FLTK, etc.
*/
class GenericSliceModel : public AbstractModel
{
public:

  irisITKObjectMacro(GenericSliceModel, AbstractModel)

  itkEventMacro(ViewportResizeEvent, IRISEvent)

  FIRES(ModelUpdateEvent)
  FIRES(SliceModelGeometryChangeEvent)
  FIRES(ViewportResizeEvent)

  // irisDeclareEventObserver(ReinitializeEvent)

  /**
   * Initializer: takes global UI model and slice ID as input
   */
  void Initialize(GlobalUIModel *model, int index);

  /**
    Set the viewport reporter (and listen to viewport events)
    */
  void SetSizeReporter(ViewportSizeReporter *reporter);

  /**
    Get viewport reporter
    */
  irisGetMacro(SizeReporter, ViewportSizeReporter *)


  /**
    Update if necessary
    */
  virtual void OnUpdate();

  /**
   * Initialize the slice view with image data
   */
  void InitializeSlice(GenericImageData *data);

  /**
   * Reset the view parameters of the window (zoom, view position) to
   * defaults
   */
  virtual void ResetViewToFit();

  /**
   * Map a point in window coordinates to a point in slice coordinates
   * (Window coordinates are the ones stored in FLTKEvent.xSpace)
   */
  Vector3f MapWindowToSlice(const Vector2f &xWindow);

  /**
   * Map an offset in window coordinates to an offset in slice coordinates
   */
  Vector3f MapWindowOffsetToSliceOffset(const Vector2f &xWindowOffset);

  /**
   * Map a point in slice coordinates to a point in window coordinates
   * (Window coordinates are the ones stored in FLTKEvent.xSpace)
   */
  Vector2f MapSliceToWindow(const Vector3f &xSlice);

  /**
   * Map a point in slice coordinates to a point in PHYISCAL window coordinates
   */
  Vector2f MapSliceToPhysicalWindow(const Vector3f &xSlice);

  /**
   * Map a point in PHYISCAL window coordinates to a point in slice coordinates
   */
  Vector3f MapPhysicalWindowToSlice(const Vector2f &uvPhysical);

  /**
   * Map a point in slice coordinates to a point in the image coordinates
   */
  Vector3f MapSliceToImage(const Vector3f &xSlice);

  /**
   * Map a point in image coordinates to slice coordinates
   */
  Vector3f MapImageToSlice(const Vector3f &xImage);

  /**
   * Get the cursor position in slice coordinates, shifted to the center
   * of the voxel
   */
  Vector3f GetCursorPositionInSliceCoordinates();

  /**
   * Get the slice index for this window
   */
  unsigned int GetSliceIndex();

  /**
   * Get the model that handles slice index information
   */
  irisGetMacro(SliceIndexModel, AbstractRangedIntProperty *)

  /**
   * Get the model than handles the selected component (timepoint) in the
   * currently selected image
   */
  irisRangedPropertyAccessMacro(CurrentComponentInSelectedLayer, unsigned int)

  /**
   * Set the index of the slice in the current view. This method will
   * update the cursor in the IRISApplication object
   */
  void UpdateSliceIndex(unsigned int newIndex);

  /**
   * Get the number of slices available in this view
   */
  unsigned int GetNumberOfSlices() const;

  /** Return the image axis along which this window shows slices */
  size_t GetSliceDirectionInImageSpace()
    { return m_ImageAxes[2]; }

  /** Reset the view position to center of the image */
  void ResetViewPosition ();

  /** Return the offset from the center of the viewport to the cursor position
   * in slice units (#voxels * spacing). This is used to synchronize panning
   * across SNAP sessions */
  Vector2f GetViewPositionRelativeToCursor();

  /** Set the offset from the center of the viewport to the cursor position */
  void SetViewPositionRelativeToCursor(Vector2f offset);

  /** Center the view on the image crosshairs */
  void CenterViewOnCursor();

  /** Set the zoom factor (number of pixels on the screen per millimeter in
   * image space */
  irisSetWithEventMacro(ViewZoom, float, SliceModelGeometryChangeEvent)

  /**
   * Zoom in/out by a specified factor. This method will 'stop' at the optimal
   * zoom if it's between the old zoom and the new zoom
   */
  void ZoomInOrOut(float factor);

  /** Get the zoom factor (number of pixels on the screen per millimeter in
   * image space */
  irisGetMacro(ViewZoom,float)

  /** Computes the zoom that gives the best fit for the window */
  void ComputeOptimalZoom();

  /** Compute the optimal zoom (best fit) */
  irisGetMacro(OptimalZoom,float)

  /** Set the zoom management flag */
  irisSetMacro(ManagedZoom,bool)

  /** Set the view position **/
  void SetViewPosition(Vector2f);

  /** Get the view position **/
  irisGetMacro(ViewPosition, Vector2f)

  /** Get the slice spacing in the display space orientation */
  irisGetMacro(SliceSpacing,Vector3f)

  /** Get the slice spacing in the display space orientation */
  irisGetMacro(SliceSize,Vector3i)

  /** The id (slice direction) of this slice model */
  irisGetMacro(Id, int)

  /** Get the physical size of the window (updated from widget via events) */
  Vector2ui GetSize();

  /**
   * Get the size of the canvas on which the slice will be rendered. When the
   * view is in tiled mode, this reports the size of one of the tiles. When the
   * new is in main/thumbnail mode, this reports the size of the main view
   */
  Vector2ui GetCanvasSize();

  /** Has the slice model been initialized with image data? */
  irisIsMacro(SliceInitialized)

  irisGetMacro(ParentUI, GlobalUIModel *)
  irisGetMacro(Driver, IRISApplication *)

  irisGetMacro(ImageData, GenericImageData *)

  irisGetMacro(ZoomThumbnailPosition, Vector2i)
  irisGetMacro(ZoomThumbnailSize, Vector2i)
  irisGetMacro(ThumbnailZoom, float)

  irisGetMacro(ImageToDisplayTransform, const ImageCoordinateTransform &)
  irisGetMacro(DisplayToAnatomyTransform, const ImageCoordinateTransform &)
  irisGetMacro(DisplayToImageTransform, const ImageCoordinateTransform &)

  irisGetMacro(ViewportLayout, const SliceViewportLayout &)

  /**
   * Get the viewport for decoration. This is either the entire viewport,
   * or when the viewport is broken into thumbnail part and main part, the
   * main part
   */
  void GetNonThumbnailViewport(Vector2ui &pos, Vector2ui &size);

  /**
   * Get the image layer for a context menu request, or NULL if requesting a
   * context menu at a position should not be possible. TODO: this is kind of
   * a weird place to house this code.
   */
  ImageWrapperBase *GetThumbnailedLayerAtPosition(int x, int y);

  /**
   * Get the layer that is in context for a position in the window. This can be
   * a thumbnail layer or a regular layer. The third parameter is a boolean flag
   * indicating whether the layer is a thumbnail or not.
   */
  ImageWrapperBase *GetContextLayerAtPosition(int x, int y, bool &outIsThumbnail);

  /** Get the layer in a given tile, when using tiled views */
  ImageWrapperBase *GetLayerForNthTile(int row, int col);

  /** Compute the canvas size needed to display slice at current zoom factor */
  Vector2i GetOptimalCanvasSize();

  /** This method computes the thumbnail properties (size, zoom) */
  void ComputeThumbnailProperties();

  // Check whether the thumbnail should be drawn or not
  bool IsThumbnailOn();

  /** A model representing the ID of the layer over which the mouse is hovering */
  irisSimplePropertyAccessMacro(HoveredImageLayerId, unsigned long)

  /** Whether the hovered image layer id is shown in thumbnail mode */
  irisSimplePropertyAccessMacro(HoveredImageIsThumbnail, bool)

  /** Get the viewport corresponding to the hovered layer */
  const SliceViewportLayout::SubViewport *GetHoveredViewport();

  /**
    Merges a binary segmentation drawn on a slice into the main
    segmentation in SNAP. Returns the number of voxels changed.

    This method might be better placed in IRISApplication
   */
  unsigned int MergeSliceSegmentation(
        itk::Image<unsigned char, 2> *drawing);


protected:

  GenericSliceModel();
  ~GenericSliceModel();

  // Parent (where the global UI state is stored)
  GlobalUIModel *m_ParentUI;

  // Top-level logic object
  IRISApplication *m_Driver;

  // Pointer to the image data
  GenericImageData *m_ImageData;

  // Viewport size reporter (communicates with the UI about viewport size)
  ViewportSizeReporter *m_SizeReporter;

  // Description of how the main viewport is divided into parts
  SliceViewportLayout m_ViewportLayout;

  // Window id, equal to the direction in display space along which the
  // window shows slices
  int m_Id;

  // The index of the image space axes corresponding to the u,v,w of the
  // window (computed by applying a transform to the DisplayAxes)
  int m_ImageAxes[3];

  // The transform from image coordinates to display coordinates
  ImageCoordinateTransform m_ImageToDisplayTransform;

  // The transform from display coordinates to image coordinates
  ImageCoordinateTransform m_DisplayToImageTransform;

  // The transform from display coordinates to patient coordinates
  ImageCoordinateTransform m_DisplayToAnatomyTransform;

  // Dimensions of the current slice (the third component is the size
  // of the image in the slice direction)
  Vector3i m_SliceSize;

  // Pixel dimensions for the slice.  (the third component is the pixel
  // width in the slice direction)
  Vector3f m_SliceSpacing;

  // Position of visible window in slice space coordinates
  Vector2f m_ViewPosition;

  // The view position where the slice wants to be
  Vector2f m_OptimalViewPosition;

  // The number of screen pixels per mm of image
  float m_ViewZoom;

  // The zoom level at which the slice fits snugly into the window
  float m_OptimalZoom;

  // Flag indicating whether the window's zooming is managed externally
  // by the SliceWindowCoordinator
  bool m_ManagedZoom;

  // The default screen margin (area into which we do not paint) at
  // least in default zoom
  unsigned int m_Margin;

  // The position and size of the zoom thumbnail
  Vector2i m_ZoomThumbnailPosition, m_ZoomThumbnailSize;

  // The zoom level in the thumbnail
  double m_ThumbnailZoom;

  // State of the model (whether it's been initialized)
  bool m_SliceInitialized;

  /** Hovered over layer id */
  SmartPtr<ConcreteSimpleULongProperty> m_HoveredImageLayerIdModel;
  SmartPtr<ConcreteSimpleBooleanProperty> m_HoveredImageIsThumbnailModel;

  /** Access the next window in the slice pipeline */
  GenericSliceModel *GetNextSliceWindow();

  SmartPtr<AbstractRangedIntProperty> m_SliceIndexModel;
  bool GetSliceIndexValueAndDomain(int &value, NumericValueRange<int> *domain);
  void SetSlideIndexValue(int value);

  SmartPtr<AbstractRangedUIntProperty> m_CurrentComponentInSelectedLayerModel;
  bool GetCurrentComponentInSelectedLayerValueAndDomain(unsigned int &value, NumericValueRange<unsigned int> *domain);
  void SetCurrentComponentInSelectedLayerValue(unsigned int value);



  /** Update the state of the viewport based on current layout settings */
  void UpdateViewportLayout();
};

#endif // GENERICSLICEMODEL_H
