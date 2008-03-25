/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: GenericSliceWindow.h,v $
  Language:  C++
  Date:      $Date: 2008/03/25 19:31:33 $
  Version:   $Revision: 1.6 $
  Copyright (c) 2007 Paul A. Yushkevich
  
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

  -----

  Copyright (c) 2003 Insight Software Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notices for more information.

  -----

  Copyright (c) 2003 Insight Software Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notices for more information. 

=========================================================================*/
#ifndef __GenericSliceWindow_h_
#define __GenericSliceWindow_h_

#if defined(_MSC_VER)
#pragma warning ( disable : 4786 )
#pragma warning ( disable : 4503 )
#endif

#include "FLTKCanvas.h"
#include "SNAPCommonUI.h"
#include "GreyImageWrapper.h"
#include "RGBImageWrapper.h"
#include "LabelImageWrapper.h"
#include "RecursiveInteractionMode.h"

// Forward references to parent classes
class IRISApplication;
class GenericImageData;
class GlobalState;
class UserInterfaceBase;  

// Forward references to interaction modes that work with this window
class CrosshairsInteractionMode;
class ZoomPanInteractionMode;
class ThumbnailInteractionMode;

// Forward reference to Gl texture object
template <class TPixel> class OpenGLSliceTexture;


/**
 * \class GenericSliceWindow
 * \brief A window used to display a 2D slice either in SNAP or in IRIS mode.
 *
 * A generic slice window, that is neither fitted to IRIS nor to SNAP.  The
 * IRIS and SNAP windows are children of this slice window class.  This class
 * provides support for display space to image space transforms, for texture
 * display and management, and for interaction mode plugins.
 *
 * The generic window supports two types of interaction modes: crosshairs mode
 * and zoom/pan mode.  
 */
class GenericSliceWindow : public RecursiveInteractionMode
{
public:

  /**
   * Register the window with its parent UI.  This method assigns an Id to 
   * the window, which is equal to the coordinate direction in display space
   * along which the window displays slices.
   */
  GenericSliceWindow(int id, UserInterfaceBase *parentUI, FLTKCanvas *inWindow);
  virtual ~GenericSliceWindow();

  /** Enter the cross-hairs mode of operation */
  virtual void EnterCrosshairsMode();
  
  /** Enter the zoom/pan mode of operation */
  virtual void EnterZoomPanMode();

  /**
   * Initialize the window's attributes (size, view position, etc.)
   * This method should called when the image dimensions and transforms
   * get updated.
   */
  virtual void InitializeSlice(GenericImageData *imageData);

  /**
   * Reset the view parameters of the window (zoom, view position) to
   * defaults
   */
  virtual void ResetViewToFit();

  /** The FLTK draw method (paints the window) */
  void OnDraw();

  /**
   * Map a point in window coordinates to a point in slice coordinates
   * (Window coordinates are the ones stored in FLTKEvent.xSpace)
   */
  Vector3f MapWindowToSlice(const Vector2f &xWindow); 
  
  /**
   * Map a point in slice coordinates to a point in window coordinates
   * (Window coordinates are the ones stored in FLTKEvent.xSpace)
   */
  Vector2f MapSliceToWindow(const Vector3f &xSlice);
  
  /**
   * Map a point in slice coordinates to a point in the image coordinates
   */
  Vector3f MapSliceToImage(const Vector3f &xSlice);

  /**
   * Map a point in image coordinates to slice coordinates
   */
  Vector3f MapImageToSlice(const Vector3f &xImage);

  /** Return the image axis along which this window shows slices */
  size_t GetSliceDirectionInImageSpace()
    { return m_ImageAxes[2]; }

  /** Set the zoom factor (number of pixels on the screen per millimeter in
   * image space */
  void SetViewZoom(float newZoom);

  /** Get the zoom factor (number of pixels on the screen per millimeter in
   * image space */
  irisGetMacro(ViewZoom,float);

  /** Compute the optimal zoom (best fit) */
  irisGetMacro(OptimalZoom,float);

  /** Set the zoom management flag */
  irisSetMacro(ManagedZoom,bool);

  /** Get the slice spacing in the display space orientation */
  irisGetMacro(SliceSpacing,Vector3f);

  /** Get the slice spacing in the display space orientation */
  irisGetMacro(SliceSize,Vector3i);

  /**
   * A parent class from which all the Fl event handlers associated
   * with this class should be derived
   */
  class EventHandler : public InteractionMode {
  public:
    EventHandler(GenericSliceWindow *parent);
    void Register();

  protected:
    GenericSliceWindow *m_Parent;
    GlobalState *m_GlobalState;
    UserInterfaceBase *m_ParentUI;
    IRISApplication *m_Driver;
  };

  // Allow friendly access by interactors
  friend class EventHandler;
  friend class BubblesInteractionMode;
  friend class CrosshairsInteractionMode;
  friend class ZoomPanInteractionMode;
  friend class RegionInteractionMode;
  friend class PolygonInteractionMode;
  friend class ThumbnailInteractionMode;
  friend class PaintbrushInteractionMode;

protected:

  /** Pointer to the application driver for this UI object */
  IRISApplication *m_Driver;

  /** Pointer to the global state object (shorthand) */
  GlobalState *m_GlobalState;

  /** Pointer to GUI that contains this Window3D object */
  UserInterfaceBase *m_ParentUI;   

  /** The image data object that is displayed in this window */
  GenericImageData *m_ImageData;

  /** Interaction mode used to position the crosshairs */
  CrosshairsInteractionMode *m_CrosshairsMode;

  /** Interaction mode used to zoom and pan the image */
  ZoomPanInteractionMode *m_ZoomPanMode;

  /** Interaction mode used to control the zoom thumbnail. This mode is always
   * on and at the top of the interaction stack */
  ThumbnailInteractionMode *m_ThumbnailMode;

  /** Whether or not we have been registered with the parent UI */
  bool m_IsRegistered;

  /** 
   * Whether or not the sizes have been initialized 
   * (synched with the image data)
   */
  bool m_IsSliceInitialized;

  // Window id, equal to the direction in display space along which the window
  // shows slices
  int m_Id;       

  // Current slice number in image coordinates 
  int m_ImageSliceIndex;

  // The position of the slice on its z-axis, in the display coordinate space
  float m_DisplayAxisPosition;

  // The index of the image space axes corresponding to the u,v,w of the window
  // (computed by applying a transform to the DisplayAxes)
  int m_ImageAxes[3]; 

  // The transform from image coordinates to display coordinates
  ImageCoordinateTransform m_ImageToDisplayTransform;
  
  // The transform from display coordinates to image coordinates
  ImageCoordinateTransform m_DisplayToImageTransform;

  // The transform from display coordinates to patient coordinates
  ImageCoordinateTransform m_DisplayToAnatomyTransform;
  
  // Dimensions of the current slice (the third component is the size of the
  // image in the slice direction)
  Vector3i m_SliceSize;             

  // Pixel dimensions for the slice.  (the thirs component is the pixel width
  // in the slice direction)
  Vector3f m_SliceSpacing;
  
  // Position of visible window in slice space coordinates
  Vector2f m_ViewPosition;            

  // The number of screen pixels per mm of image
  float m_ViewZoom;  

  // The zoom level at which the slice fits snugly into the window
  float m_OptimalZoom;

  // Flag indicating whether the window's zooming is managed externally by
  // the SliceWindowCoordinator
  bool m_ManagedZoom;

  // The default screen margin (area into which we do not paint) at lest in 
  // default zoom
  unsigned int m_Margin;

  // Grey texture object typedefs
  typedef OpenGLSliceTexture<unsigned char> GreyTextureType;
  typedef OpenGLSliceTexture<LabelType> LabelTextureType;

  // Label texture object typedefs
  typedef LabelImageWrapper::DisplayPixelType RGBAType;
  typedef OpenGLSliceTexture<RGBAType> RGBTextureType;
    
  // Grey texture object
  GreyTextureType *m_GreyTexture;

  // RGB texture object
  RGBTextureType *m_RGBTexture, *m_LabelRGBTexture;
  
  // Label texture object
  LabelTextureType *m_LabelColorIndexTexture;
  
  // Check whether the thumbnail should be draw or not
  bool IsThumbnailOn();

  // The position and size of the zoom thumbnail
  Vector2i m_ThumbnailPosition, m_ThumbnailSize;

  // Whether or not the thumbnail is being drawn at the moment
  bool m_ThumbnailIsDrawing;

  // The zoom level in the thumbnail
  double m_ThumbnailZoom;

  // Computes the zoom that gives the best fit for the window
  void ComputeOptimalZoom();
  
  // This method is called in draw() to paint the grey slice
  virtual void DrawGreyTexture();

  // This method is called in draw() to paint the RGB slice
  virtual void DrawRGBTexture();

  // This method is called in draw() to paint the segmentation slice
  virtual void DrawSegmentationTexture();

  // This method is called after the grey and segmentation images have
  // been drawn.  It calls the draw method of each of the interaction modes
  virtual void DrawOverlays();

  /** This method draws the RAI labels at the four sides of the slice */
  void DrawOrientationLabels();

  /** Draw a window that shows where in the image the zoom region is located */
  void DrawThumbnail();

  /** Access the next window in the slice pipeline */
  GenericSliceWindow *GetNextSliceWindow();

  /** Activate a given interaction mode */
  virtual void EnterInteractionMode(InteractionMode *mode);
};

#endif // __GenericSliceWindow_h_
