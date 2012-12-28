/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: IRISApplication.h,v $
  Language:  C++
  Date:      $Date: 2009/08/26 01:10:20 $
  Version:   $Revision: 1.18 $
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

=========================================================================*/

#ifndef __IRISApplication_h_
#define __IRISApplication_h_

#include "ImageCoordinateTransform.h"
#include "itkImageRegion.h"
#include "itkExceptionObject.h"
#include "GlobalState.h"
#include "ColorLabelTable.h"
#include "itkCommand.h"
#include "SystemInterface.h"
#include "UndoDataManager.h"
#include "SNAPEvents.h"

// #include "itkImage.h"

// Forward reference to the classes pointed at
class GenericImageData;
class IRISException;
class IRISImageData;
class SNAPImageData;
class MeshExportSettings;
class GuidedNativeImageIO;
class ThresholdSettings;
class EdgePreprocessingSettings;
class AbstractSlicePreviewFilterWrapper;

template <class TFilterConfigTraits> class SlicePreviewFilterWrapper;
class SmoothBinaryThresholdFilterConfigTraits;
class EdgePreprocessingFilterConfigTraits;
template <typename TIn, typename TOut> class SmoothBinaryThresholdImageFilter;
template <typename TIn, typename TOut> class EdgePreprocessingImageFilter;


namespace itk {
  template <class TPixel, unsigned int VDimension> class Image;
}

/**
 * \class IRISApplication
 * \brief This class encapsulates the highest level login of SNAP and IRIS.
 *
 * TODO: Organize the interaction between this class, IRISImageData and SNAPImageData
 * in a more intuitive way.  
 *
 * 'RAI' codes used by this class:
 * The code is a string that describes the transform from image space to patient 
 * coordinate system as three letters from RLAPIS.  For instance, PSR
 * means that the image origin is at the posterior-superior-right corner
 * of the image coordinate system and that the x axis maps to A-P axis, 
 * y to I-S and x to R-L.
 *
 * \sa IRISImageData
 * \sa SNAPImageData
 */
class IRISApplication : public itk::Object
{
public:

  irisITKObjectMacro(IRISApplication, itk::Object)

  // Typedefs
  typedef itk::ImageRegion<3> RegionType;
  typedef itk::Size<3> SizeType;
  typedef itk::Image<GreyType,3> GreyImageType;
  typedef itk::Image<RGBType,3> RGBImageType;
  typedef itk::Image<LabelType,3> LabelImageType;
  typedef itk::Image<short ,3> SpeedImageType;
  typedef itk::Command CommandType;
  typedef UndoDataManager<LabelType> UndoManagerType;

  // A drawing performed on a slice
  typedef itk::Image<unsigned char, 2> SliceBinaryImageType;

  // Bubble array
  typedef std::vector<Bubble> BubbleArray;

  // The main image can be of these types
  enum MainImageType { MAIN_SCALAR, MAIN_RGB, MAIN_ANY };

  // Declare events fired by this object
  FIRES(CursorUpdateEvent)
  FIRES(MainImageDimensionsChangeEvent)
  FIRES(MainImagePoseChangeEvent)
  FIRES(LayerChangeEvent)
  FIRES(SegmentationChangeEvent)
  FIRES(SpeedImageChangedEvent)
  FIRES(DisplayToAnatomyCoordinateMappingChangeEvent)


  /**
   * Get image data related to IRIS operations
   */
  irisGetMacro(IRISImageData,IRISImageData *);

  /**
   * Get image data related to SNAP operations
   */
  irisGetMacro(SNAPImageData,SNAPImageData *);

  /**
   * Get the image data currently used
   */
  irisGetMacro(CurrentImageData,GenericImageData *);

  /**
   * Enter the IRIS mode
   */    
  void SetCurrentImageDataToIRIS();

  /**
   * Enter the SNAP mode
   */
  void SetCurrentImageDataToSNAP();

  /**
    Whether we are currently in active contour mode or not
    */
  bool IsSnakeModeActive() const;

  /** 
   * Set a new main image for IRIS. This method is called to load either grey or
   * RGB image data into IRISImageData. The parameter is the GuidedNativeImageIO,
   * which holds an image in native format. The second parameter specified whether
   * to force RGB or grey image, or to determine image type based on the data.
   */
  MainImageType UpdateIRISMainImage(
    GuidedNativeImageIO *nativeIO, MainImageType force_type);

  /**
   * Add an overlay image into IRIS. This method is called to load either grey or
   * RGB image data into IRISImageData. The parameter is the GuidedNativeImageIO,
   * which holds an image in native format. The second parameter specified whether
   * to force RGB or grey image, or to determine image type based on the data.
   */
  MainImageType AddIRISOverlayImage(
    GuidedNativeImageIO *nativeIO, MainImageType force_type);

  /**
   * Set a new grey image for the IRIS Image data.  This method is called when the
   * grey image is loaded.  The prerequisite to this method is that the SNAP data
   * not be active (CurrentImageData == IRISImageData).
   */
  void UnloadOverlays();
  void UnloadOverlayLast();
  void UnloadMainImage();

  /** 
   * Update the IRIS image data with an external segmentation image (e.g., 
   * loaded from a file).
   */
  void UpdateIRISSegmentationImage(GuidedNativeImageIO *io);

  /** 
   * Clear the IRIS segmentation image
   */
  void ClearIRISSegmentationImage();

  /** 
   * Update the SNAP image data with an external speed image (e.g., 
   * loaded from a file).
   */
  void UpdateSNAPSpeedImage(SpeedImageType *newSpeedImage, SnakeType snakeMode);
  
  /**
   * Initialize SNAP Image data using region of interest extents, and a new
   * voxel size.
   */
  void InitializeSNAPImageData(const SNAPSegmentationROISettings &roi,
                               CommandType *progressCommand = NULL);

  /**
    Enter given preprocessing mode. This activates the pipeline that can be
    used to provide automatic on-the-fly preview of the preprocessing result
    as the user moves the cursor or changes preprocessing parameters. When
    preprocessing is done, or before switching to a new preprocessing mode,
    call this method with PREPROCESS_NONE to disconnect the pipeline.
    */
  void EnterPreprocessingMode(PreprocessingMode mode);

  /**
    Uses the current preprocessing mode to compute the entire extents of the
    speed image. This also sets the SpeedValid flag in GlobalState to true
    */
  void ApplyCurrentPreprocessingModeToSpeedVolume(itk::Command *progress = 0);

  /**
    Get the current preprocessing mode
    */
  PreprocessingMode GetPreprocessingMode() const;

  /**
    Get a pointer to the object that handles the preview pipeline for the
    current preprocessing mode. This object can be used to toggle preview
    and to execute the preprocessing filter. Returns NULL if the mode is
    PREPROCESS_NONE
    */
  AbstractSlicePreviewFilterWrapper *GetPreprocessingFilterPreviewer(
      PreprocessingMode mode);

  /**
    Get a reference to the bubble array
    */
  BubbleArray &GetBubbleArray();

  /**
    Initialize the SNAP active contour evolution with the bubbles.
    */
  bool InitializeActiveContourPipeline();

  /**
   * Update IRIS image data with the segmentation contained in the SNAP image
   * data.
   */
  void UpdateIRISWithSnapImageData(CommandType *progressCommand = NULL);

  /**
   * Get the segmentation label data
   */
  irisGetMacro(ColorLabelTable, ColorLabelTable *);

  /** Release the SNAP Image data */
  void ReleaseSNAPImageData();
  
  /** Update the display-anatomy mapping as an RAI code */
  void SetDisplayToAnatomyRAI(const char *rai0,const char *rai1,const char *rai2);

  /** Does the current image have oblique orientation? */
  bool IsImageOrientationOblique();

  /** Get the current image to anatomy RAI code */
  std::string GetImageToAnatomyRAI();

  /** Get the current display to anatomy RAI code */
  std::string GetDisplayToAnatomyRAI(unsigned int slice);

  /** Get the image axis for a given anatomical direction */
  size_t GetImageDirectionForAnatomicalDirection(
    AnatomicalDirection iAnat);

  /** Get the display window corresponding to an anatomical direction */
  size_t GetDisplayWindowForAnatomicalDirection(
    AnatomicalDirection iAnat);

  /**
   * Get the global state object
   */
  irisGetMacro(GlobalState, GlobalState *);

  /**
   * Get the system interface
   */
  irisGetMacro(SystemInterface, SystemInterface *);

  /**
   * Set the current cursor position.  This will cause all the active image
   * wrappers to update their current slice numbers
   */
  void SetCursorPosition(const Vector3ui cursor);

  /**
   * Get the cursor position
   */
  Vector3ui GetCursorPosition() const;

  /**
   * Export the current slice of the image into a file
   */
  void ExportSlice(AnatomicalDirection iSliceAnatomy, const char *file);

  /** Export voxel statistis to a file */
  void ExportSegmentationStatistics(const char *file);

  /**
   * Export the 3D mesh to a file, using settings passed in the
   * MeshExportSettings structure.
   */
  void ExportSegmentationMesh(const MeshExportSettings &sets, itk::Command *cmd);

  /** 
   * This method is used to selectively override labels in a target 
   * segmentation image with the current drawing color.  It uses the 
   * current coverage mode to determine whether to override the pixel 
   * or to keep it */
  LabelType DrawOverLabel(LabelType iTarget);

  /**
   * Really simple replacement of one label with another. Returns the 
   * number of voxels changed.
   */
  size_t ReplaceLabel(LabelType drawing, LabelType drawover);

  /**
    Number of voxels of a given label in the segmentation.
    */
  size_t GetNumberOfVoxelsWithLabel(LabelType label);

  /*
   * Cut the segmentation using a plane and relabed the segmentation
   * on the side of that plane
   */
  void RelabelSegmentationWithCutPlane(
    const Vector3d &normal, double intercept);

  /**
   * Compute the intersection of the segmentation with a ray
   */
  int GetRayIntersectionWithSegmentation(const Vector3d &point, 
                     const Vector3d &ray, 
                     Vector3i &hit) const;

  /**
   * Load the main image from file. You can either specify that the main
   * image is of a given type (grey vs. rgb) or you can let the program 
   * decide dynamically, based on the number of components in the file
   */
  MainImageType LoadMainImage(const char *filename, MainImageType force_type);

  MainImageType LoadOverlayImage(const char *filename, MainImageType force_type);

  /**
   * Check if there is an image currently loaded in SNAP.
   */
  bool IsMainImageLoaded() const;

  /**
   * This is the most high-level method to load a segmentation image. The
   * segmentation image can only be loaded after the grey image has been 
   * loaded and it must have the same dimensions
   * 
   * This function is deprecated and replaced by the more robust version
   * in the ImageIOWizardLogic class!
   *
   */
  void LoadLabelImageFile(const char *filename);

  /**
    Load label descriptions from file
    */
  void LoadLabelDescriptions(const char *filename);

  /**
   * Store the current state as an undo point, allowing the user to revert
   * to this state at a later point. The state in this context is just the
   * segmentation image in IRIS.
   */
  void StoreUndoPoint(const char *text);

  /** 
   * Clear all the undo points, e.g., after an operation that can not be
   * undone
   */
  void ClearUndoPoints();

  /** Check whether undo is possible */
  bool IsUndoPossible();

  /** Check whether undo is possible */
  bool IsRedoPossible();

  /** Undo (revert to last stored undo point) */
  void Undo();

  /** Redo (undo the undo) */
  void Redo();

  irisGetMacro(UndoManager, const UndoManagerType &);

  /** 
   * Reorient the main image (and all overlays) 
   */
  void ReorientImage(vnl_matrix_fixed<double, 3, 3> inDirection);

  /**
    Apply a binary drawing performed on an orthogonal slice to the
    main segmentation.
    */
  unsigned int UpdateSegmentationWithSliceDrawing(
      SliceBinaryImageType *drawing,
      const ImageCoordinateTransform &xfmSliceToImage,
      double zSlice,
      const std::string &undoTitle);

  /** Get the pointer to the settings used for threshold-based preprocessing */
  irisGetMacro(ThresholdSettings, ThresholdSettings *)

  /** Get the pointer to the settings used for edge-based preprocessing */
  irisGetMacro(EdgePreprocessingSettings, EdgePreprocessingSettings *)

  /** Set the current snake mode. This method should be called instead of the
      method in GlobalState because when the snake mode is set, some changes
      are made to the image data (having to do with setting up the preview
      filters if they are available, and cleaning speed data) */
  void SetSnakeMode(SnakeType mode);

  /** Get the current snake mode (active contours must be active) */
  SnakeType GetSnakeMode() const;

protected:

  IRISApplication();
  virtual ~IRISApplication();

  // Image data objects
  GenericImageData *m_CurrentImageData;
  IRISImageData *m_IRISImageData;
  SNAPImageData *m_SNAPImageData;

  // Color label data
  SmartPtr<ColorLabelTable> m_ColorLabelTable;

  // Global state object
  // TODO: Incorporate GlobalState into IRISApplication more nicely
  GlobalState *m_GlobalState;

  // SystemInterface used to get things from the system
  SystemInterface *m_SystemInterface;

  /** RAI between anatomy space and image space */
  std::string m_DisplayToAnatomyRAI[3];

  // Undo data manager. Perhaps this should really be in IRISImageData, but
  // there is a lot of stuff here that is ambiguous in this way. The manager
  // stores 'deltas', i.e., differences between states of the segmentation
  // image. These deltas are compressed, allowing us to store a bunch of 
  // undo steps with little cost in performance or memory
  UndoManagerType m_UndoManager;

  // Settings for the speed preprocessing. Still not sure this is the best
  // place to put this stuff!
  SmartPtr<ThresholdSettings> m_ThresholdSettings;
  SmartPtr<EdgePreprocessingSettings> m_EdgePreprocessingSettings;

  // The threshold-based preview wrapper type
  typedef SlicePreviewFilterWrapper<SmoothBinaryThresholdFilterConfigTraits>
                                                   ThresholdPreviewWrapperType;

  // The edge-based preview wrapper type
  typedef SlicePreviewFilterWrapper<EdgePreprocessingFilterConfigTraits>
                                           EdgePreprocessingPreviewWrapperType;

  // The threshold-based wrapper
  SmartPtr<ThresholdPreviewWrapperType> m_ThresholdPreviewWrapper;

  // The edge-based wrapper
  SmartPtr<EdgePreprocessingPreviewWrapperType> m_EdgePreviewWrapper;

  // The currently hooked up preprocessing filter preview wrapper
  PreprocessingMode m_PreprocessingMode;

  // Array of bubbles
  BubbleArray m_BubbleArray;

};

#endif // __IRISApplication_h_
