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
#include "IRISDisplayGeometry.h"
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
class UnsupervisedClustering;
class RFClassificationEngine;
class ImageWrapperBase;
class MeshManager;
class AbstractLoadImageDelegate;
class AbstractSaveImageDelegate;
class IRISWarningList;
class GaussianMixtureModel;
class RandomForestClassifier;
struct IRISDisplayGeometry;
class LabelUseHistory;
class ImageAnnotationData;

template <class TTraits> class PresetManager;
class ColorMapPresetTraits;
typedef PresetManager<ColorMapPresetTraits> ColorMapPresetManager;

template <class TFilterConfigTraits> class SlicePreviewFilterWrapper;
class SmoothBinaryThresholdFilterConfigTraits;
class EdgePreprocessingFilterConfigTraits;
class GMMPreprocessingFilterConfigTraits;
class RFPreprocessingFilterConfigTraits;
template <typename TIn, typename TOut> class SmoothBinaryThresholdImageFilter;
template <typename TIn, typename TOut> class EdgePreprocessingImageFilter;
template <typename TIn, typename TVIn, typename TOut> class GMMClassifyImageFilter;


namespace itk {
  template <class TPixel, unsigned int VDimension> class Image;
  template <class TPixel, unsigned int VDimension> class VectorImage;
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

  // The internal representation of anatomical images
  typedef itk::VectorImage<GreyType, 3> AnatomyImageType;

  typedef itk::Image<LabelType,3> LabelImageType;
  typedef itk::Image<short ,3> SpeedImageType;
  typedef itk::Command CommandType;
  typedef UndoDataManager<LabelType> UndoManagerType;

  // A drawing performed on a slice
  typedef itk::Image<unsigned char, 2> SliceBinaryImageType;

  // Bubble array
  typedef std::vector<Bubble> BubbleArray;

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
   * Whether there is currently a valid level set function
   */
  bool IsSnakeModeLevelSetActive() const;

  /**
   * Load an image using a delegate object. The delegate specializes the behavior
   * of this class to different layer roles (main image, overlay). The warnings
   * generated in the course of the IO operation are stored in the passed in
   * warning list object;
   */
  void LoadImageViaDelegate(const char *fname,
                            AbstractLoadImageDelegate *del,
                            IRISWarningList &wl);

  /**
   * Load an image for a particular role using the default delegate for this role.
   * This convenience method is currently implemented for MAIN, OVERLAY and LABEL
   * image types. This method loads the associated settings and metadata for the
   * image either from the user's image associations directory (default) or from
   * the provided Registry object.
   */
  void LoadImage(const char *fname, LayerRole role,
                 IRISWarningList &wl, Registry *meta_data_reg = NULL);

  /**
   * Create a delegate for saving an image interactively or non-interactively
   * via a wizard.
   */
  SmartPtr<AbstractSaveImageDelegate> CreateSaveDelegateForLayer(
      ImageWrapperBase *layer, LayerRole role);

  /** 
   * Update the main image in IRIS. The first parameter is the IO object that
   * has the image data loaded, and the second parameter is an optional pointer
   * to a registry from which to read the metadata. If not provided, metadata
   * will be read from the 'image association' files automatically generated
   * as images are closed.
   */
  void UpdateIRISMainImage(GuidedNativeImageIO *nativeIO, Registry *metadata = NULL);

  /**
   * Add an overlay image into IRIS.
   */
  void AddIRISOverlayImage(GuidedNativeImageIO *nativeIO, Registry *metadata = NULL);

  /**
   * Add an overlay image that resides in its native space, but is transformed into
   * the space of the main image using a set of transformation parameters (e.g., rigid
   * transform)
   */
  void AddIRISCoregOverlayImage(GuidedNativeImageIO *io, Registry *metadata);

  /**
   * Add a 'derived' overlay, i.e., an overlay generated using image processing from one
   * of the existing image layers
   */
  void AddDerivedOverlayImage(ImageWrapperBase *overlay);

  /**
   * Remove a specific overlay
   */
  void UnloadOverlay(ImageWrapperBase *ovl);

  /**
   * Remove all overlays
   */
  void UnloadAllOverlays();

  /**
   * Unload the main image layer
   */
  void UnloadMainImage();

  /**
   * Move layers (overlay for now) up and down in the list, changing their
   * display order
   */
  void ChangeOverlayPosition(ImageWrapperBase *overlay, int dir);

  /**
   * Quit the application. This responds to the quit action in the main application.
   * This unloads all layers. Additionally, when the application is in snake mode,
   * it first cancels snake mode
   */
  void Quit();

  /** 
   * Update the IRIS image data with an external segmentation image (e.g., 
   * loaded from a file).
   */
  void UpdateIRISSegmentationImage(GuidedNativeImageIO *io);

  /** 
   * Clear the IRIS segmentation image
   */
  void ResetIRISSegmentationImage();

  /**
   * Clear the SNAP segmentation image (active during pre-segmentation)
   */
  void ResetSNAPSegmentationImage();

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
  irisGetMacro(ColorLabelTable, ColorLabelTable *)

  /**
   * Get the history of recently used color label combos
   */
  irisGetMacro(LabelUseHistory, LabelUseHistory *)

  /** Release the SNAP Image data */
  void ReleaseSNAPImageData();
  
  /** Update the display-anatomy mapping as an RAI code */
  void SetDisplayGeometry(const IRISDisplayGeometry &dispGeom);

  /** Get the current display-anatomy mapping */
  irisGetMacro(DisplayGeometry, const IRISDisplayGeometry &)

  /** Does the current image have oblique orientation? */
  bool IsImageOrientationOblique();

  /** Get the current image to anatomy RAI code */
  std::string GetImageToAnatomyRAI();

  /** Get the image axis for a given anatomical direction */
  int GetImageDirectionForAnatomicalDirection(
    AnatomicalDirection iAnat);

  /** Get the display window corresponding to an anatomical direction */
  int GetDisplayWindowForAnatomicalDirection(AnatomicalDirection iAnat) const;

  /** Get the anatomical direction in the i-th display window */
  AnatomicalDirection GetAnatomicalDirectionForDisplayWindow(int iWin) const;

  /**
   * Get the global state object
   */
  irisGetMacro(GlobalState, GlobalState *);

  /**
   * Get the system interface
   */
  irisGetMacro(SystemInterface, SystemInterface *);

  /**
   * Get the history manager
   */
  irisGetMacro(HistoryManager, HistoryManager *)

  /**
   * Set the current cursor position.  This will cause all the active image
   * wrappers to update their current slice numbers. By default, the method
   * does nothing if the passed in cursor position is the same as the current
   * cursor position. When force is true, the cursor position is set regardless.
   */
  void SetCursorPosition(const Vector3ui cursor, bool force = false);

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
   * Method that signals the beginning of segmentation update operation. This
   * method should be used in conjuction with UpdateSegmentationVoxel method
   * to apply current drawing properties to a set of voxels
   *
   * These methods don't perform error checking - it's the user's responsibility
   * to call them in the correct order. The methods are not currently reentrant
   * (i.e., should not be used by multiple threads).
   *
   * TODO: this is currently only being used for spray paint. But this set of
   * methods should be expanded with a method that works with itk iterators and
   * should be made the main entrypoint for changing segmentations. This will
   * especially be necessary for RLE segmentation management.
   *
   */
  void BeginSegmentationUpdate(std::string undo_name);

  /**
   * Apply the current drawing label to a voxel. Depending on coverage mode
   * and the voxel's current label, the label of the voxel may be changed
   */
  void UpdateSegmentationVoxel(const Vector3ui &pos);

  /**
   * Complete the segmentation update. Returns the actual number of voxels
   * relabeled since BeginSegmentationUpdate();
   */
  int EndSegmentationUpdate();

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
   * Check if there is an image currently loaded in SNAP.
   */
  bool IsMainImageLoaded() const;

  /**
    Load label descriptions from file
    */
  void LoadLabelDescriptions(const char *filename);

  /**
    Save label descriptions to file
    */
  void SaveLabelDescriptions(const char *filename);

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
  // irisGetMacro(ThresholdSettings, ThresholdSettings *)

  /** Get the pointer to the settings used for edge-based preprocessing */
  irisGetMacro(EdgePreprocessingSettings, EdgePreprocessingSettings *)

  /** Get the object used to drive the unsupervised clustering */
  irisGetMacro(ClusteringEngine, UnsupervisedClustering *)

  /** Get the object used to drive the supervised classification */
  irisGetMacro(ClassificationEngine, RFClassificationEngine *)

  /** Set the current snake mode. This method should be called instead of the
      method in GlobalState because when the snake mode is set, some changes
      are made to the image data (having to do with setting up the preview
      filters if they are available, and cleaning speed data) */
  void SetSnakeMode(SnakeType mode);

  /** Get the current snake mode (active contours must be active) */
  SnakeType GetSnakeMode() const;

  /** Get the object used to manage VTK mesh creation */
  irisGetMacro(MeshManager, MeshManager *)

  /** Get the preset manager for color maps */
  irisGetMacro(ColorMapPresetManager, ColorMapPresetManager *)

  // ----------------------- Project support ------------------------------

  /**
   * Save a project. This method requires that all the layers that can be
   * saved in the project have a filename. The project will be written to
   * the file in the Registry format. The name of the project will be stored.
   *
   * A project is reset (set to empty string) when a new main image is loaded.
   */
  void SaveProject(const std::string &proj_file);

  /**
   * Open an existing project.
   */
  void OpenProject(const std::string &proj_file, IRISWarningList &warn);

  /**
   * Check if the project has modified since the last time it was saved. This
   * is a bit tricky to keep track of, because the project includes both the
   * list of images and the parameters.
   */
  bool IsProjectUnsaved();

  /**
   * Check if a file constitutes a project. This needs to be done quickly,
   * and since the files passed in might be binary, loading the file in
   * memory is not an option
   */
  bool IsProjectFile(const char *filename);

  // --------------------- End project support ----------------------------

  // --------------------- Annotation support ----------------------------

  /**
   * Read annotations from file
   */
  void LoadAnnotations(const char *filename);

  /**
   * Save annotations to file
   */
  void SaveAnnotations(const char *filename);

protected:

  IRISApplication();
  virtual ~IRISApplication();

  // Map cursor from one image data to another
  void TransferCursor(GenericImageData *source, GenericImageData *target);

  // Image data objects
  GenericImageData *m_CurrentImageData;
  SmartPtr<IRISImageData> m_IRISImageData;
  SmartPtr<SNAPImageData> m_SNAPImageData;

  // Color label data
  SmartPtr<ColorLabelTable> m_ColorLabelTable;

  // Label use history
  SmartPtr<LabelUseHistory> m_LabelUseHistory;

  // Global state object
  // TODO: Incorporate GlobalState into IRISApplication more nicely
  SmartPtr<GlobalState> m_GlobalState;

  // SystemInterface used to get things from the system
  SystemInterface *m_SystemInterface;

  // History manager
  HistoryManager *m_HistoryManager;

  // Coordinate mapping between display space and anatomical space
  IRISDisplayGeometry m_DisplayGeometry;

  // Undo data manager. Perhaps this should really be in IRISImageData, but
  // there is a lot of stuff here that is ambiguous in this way. The manager
  // stores 'deltas', i.e., differences between states of the segmentation
  // image. These deltas are compressed, allowing us to store a bunch of 
  // undo steps with little cost in performance or memory
  UndoManagerType m_UndoManager;

  // Settings for the speed preprocessing. Still not sure this is the best
  // place to put this stuff!
  SmartPtr<EdgePreprocessingSettings> m_EdgePreprocessingSettings;

  // The last mixture model used for clustering. This is reused during repeated
  // calls to the active contour segmentation, as long as the layers haven't
  // been updated.
  SmartPtr<GaussianMixtureModel> m_LastUsedMixtureModel;

  // The threshold-based preview wrapper type
  typedef SlicePreviewFilterWrapper<SmoothBinaryThresholdFilterConfigTraits>
                                                   ThresholdPreviewWrapperType;

  // The edge-based preview wrapper type
  typedef SlicePreviewFilterWrapper<EdgePreprocessingFilterConfigTraits>
                                           EdgePreprocessingPreviewWrapperType;

  // The GMM preview wrapper type
  typedef SlicePreviewFilterWrapper<GMMPreprocessingFilterConfigTraits>
                                            GMMPreprocessingPreviewWrapperType;

  // The GMM preview wrapper type
  typedef SlicePreviewFilterWrapper<RFPreprocessingFilterConfigTraits>
                                            RFPreprocessingPreviewWrapperType;

  // The threshold-based wrapper
  SmartPtr<ThresholdPreviewWrapperType> m_ThresholdPreviewWrapper;

  // The edge-based wrapper
  SmartPtr<EdgePreprocessingPreviewWrapperType> m_EdgePreviewWrapper;

  // GMM-based preprocessing wrapper
  SmartPtr<GMMPreprocessingPreviewWrapperType> m_GMMPreviewWrapper;

  // Random forest preprocessing wrapper
  SmartPtr<RFPreprocessingPreviewWrapperType> m_RandomForestPreviewWrapper;

  // The EM classification object
  SmartPtr<UnsupervisedClustering> m_ClusteringEngine;

  // The Random Foreset classification object
  SmartPtr<RFClassificationEngine> m_ClassificationEngine;

  // The last classifier used for random forest segmentation. This is reused during
  // repeated calls to the active contour segmentation, as long as the layers haven't
  // been updated.
  SmartPtr<RandomForestClassifier> m_LastUsedRFClassifier;

  // The number of components for the last used RF classifier
  int m_LastUsedRFClassifierComponents;

  // Mesh object (used to manage meshes)
  SmartPtr<MeshManager> m_MeshManager;

  // Color map preset manager
  SmartPtr<ColorMapPresetManager> m_ColorMapPresetManager;

  // The currently hooked up preprocessing filter preview wrapper
  PreprocessingMode m_PreprocessingMode;

  // Array of bubbles
  BubbleArray m_BubbleArray;

  // State used in conjunction with BeginSegmentationUpdate/EndSegmentationUpdate
  std::string m_SegmentationUpdateName;
  unsigned int m_SegmentationChangeCount;

  // Save metadata for a layer to the associations file
  void SaveMetaDataAssociatedWithLayer(ImageWrapperBase *layer, int role,
                                       Registry *override = NULL);
  void LoadMetaDataAssociatedWithLayer(ImageWrapperBase *layer, int role,
                                       Registry *override = NULL);

  // Create layer-specific segmentation settings (threshold settings, e.g.)
  void CreateSegmentationSettings(ImageWrapperBase *wrapper, LayerRole role);

  // Helper functions for GMM mode enter/exit
  void EnterGMMPreprocessingMode();
  void LeaveGMMPreprocessingMode();

  // Helper functions for RF mode enter/exit
  void EnterRandomForestPreprocessingMode();
  void LeaveRandomForestPreprocessingMode();

  // ----------------------- Project support ------------------------------

  // Cached state of the project at the time of last open/save. Used to check
  // if the project has been modified.
  Registry m_LastSavedProjectState;

  // Internal method used by the project IO code
  void SaveProjectToRegistry(Registry &preg, const std::string proj_file_full);

  // Auto-adjust contrast of a layer on load
  void AutoContrastLayerOnLoad(ImageWrapperBase *layer);


};

#endif // __IRISApplication_h_
