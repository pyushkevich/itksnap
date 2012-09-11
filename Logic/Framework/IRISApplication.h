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
#include "ImageCoordinateTransform.h"
#include "itkImageRegion.h"
#include "itkExceptionObject.h"
#include "GlobalState.h"
#include "ColorLabelTable.h"
#include "itkCommand.h"
#include "SystemInterface.h"
#include "UndoDataManager.h"

//#include "itkImage.h"

// Forward reference to the classes pointed at
class GenericImageData;
class IRISImageData;
class SNAPImageData;
class MeshExportSettings;
class GuidedNativeImageIO;
class IRISException;
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
class IRISApplication 
{
public:
  // Typedefs
  typedef itk::ImageRegion<3> RegionType;
  typedef itk::Size<3> SizeType;
  typedef itk::Image<GreyType,3> GreyImageType;
  typedef itk::Image<RGBType,3> RGBImageType;
  typedef itk::Image<LabelType,3> LabelImageType;
  typedef itk::Image<float,3> SpeedImageType;
  typedef itk::Command CommandType;
  typedef UndoDataManager<LabelType> UndoManagerType;

  // The main image can be of these types
  enum MainImageType { MAIN_SCALAR, MAIN_RGB, MAIN_ANY };

  /**
   * Constructor for the IRIS/SNAP application
   */
  IRISApplication();

  /**
   * Destructor for the application
   */
  virtual ~IRISApplication();

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
  void ExportSegmentationStatistics(const char *file) 
    throw(itk::ExceptionObject);

  /**
   * Export the 3D mesh to a file, using settings passed in the
   * MeshExportSettings structure.
   */
  void ExportSegmentationMesh(const MeshExportSettings &sets, itk::Command *cmd)
    throw(itk::ExceptionObject, IRISException);

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

private:
  // Image data objects
  GenericImageData *m_CurrentImageData;
  IRISImageData *m_IRISImageData;
  SNAPImageData *m_SNAPImageData;

  // Color label data
  ColorLabelTable *m_ColorLabelTable;

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
};

