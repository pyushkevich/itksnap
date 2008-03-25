/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: IRISApplication.h,v $
  Language:  C++
  Date:      $Date: 2008/03/25 19:31:31 $
  Version:   $Revision: 1.7 $
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
#include "IntensityCurveInterface.h"
#include "ImageCoordinateTransform.h"
#include "itkImageRegion.h"
#include "itkExceptionObject.h"
#include "GlobalState.h"
#include "ColorLabelTable.h"
#include "itkCommand.h"
#include "SystemInterface.h"
#include "UndoDataManager.h"

// #include "itkImage.h"

// Forward reference to the classes pointed at
class GenericImageData;
class IRISImageData;
class SNAPImageData;
class MeshExportSettings;
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
  typedef IntensityCurveInterface::Pointer IntensityCurvePointer;
  typedef itk::ImageRegion<3> RegionType;
  typedef itk::Size<3> SizeType;
  typedef itk::Image<GreyType,3> GreyImageType;
  typedef itk::Image<RGBType,3> RGBImageType;
  typedef itk::Image<LabelType,3> LabelImageType;
  typedef itk::Image<float,3> SpeedImageType;
  typedef itk::Command CommandType;

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
   * Set a new grey image for the IRIS Image data.  This method is called when the
   * grey image is loaded.  The prerequisite to this method is that the SNAP data
   * not be active (CurrentImageData == IRISImageData).
   */
  void UpdateIRISGreyImage(GreyImageType *newGreyImage,
                           const char *newImageRAICode);

  void UpdateIRISRGBImage(RGBImageType *newRGBImage,
                           const char *newImageRAICode);

  void UpdateIRISRGBImage(RGBImageType *newRGBImage);

  /** 
   * Update the IRIS image data with an external segmentation image (e.g., 
   * loaded from a file).
   */
  void UpdateIRISSegmentationImage(LabelImageType *newSegmentationImage);

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
  const char *GetImageToAnatomyRAI();

  /** Get the current display to anatomy RAI code */
  const char *GetDisplayToAnatomyRAI(unsigned int slice);

  /** Get the image axis for a given anatomical direction */
  size_t GetImageDirectionForAnatomicalDirection(
    AnatomicalDirection iAnat);

  /** Get the display window corresponding to an anatomical direction */
  size_t GetDisplayWindowForAnatomicalDirection(
    AnatomicalDirection iAnat);

  /**
   * Intensity mapping curve used for Grey images
   * in the application
   */
  irisGetMacro(IntensityCurve,IntensityCurvePointer);

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
    throw(itk::ExceptionObject);

  /** 
   * This method is used to selectively override labels in a target 
   * segmentation image with the current drawing color.  It uses the 
   * current coverage mode to determine whether to override the pixel 
   * or to keep it */
  LabelType DrawOverLabel(LabelType iTarget);

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
   * This is the most high-level way to load an image in SNAP. This method 
   * will use whatever prior information exists in image associations to 
   * load an image based on the filename. The second parameter is the rai
   * code (orientation), which you can override (i.e., on command line)
   */
  void LoadGreyImageFile(const char *filename, const char *rai = NULL);

  /**
   * This is the most high-level method to load a segmentation image. The
   * segmentation image can only be loaded after the grey image has been 
   * loaded and it must have the same dimensions
   */
  void LoadLabelImageFile(const char *filename);

  /**
   * Store the current state as an undo point, allowing the user to revert
   * to this state at a later point. The state in this context is just the
   * segmentation image in IRIS.
   */
  void StoreUndoPoint(const char *text);

  /** Check whether undo is possible */
  bool IsUndoPossible();

  /** Check whether undo is possible */
  bool IsRedoPossible();

  /** Undo (revert to last stored undo point) */
  void Undo();

  /** Redo (undo the undo) */
  void Redo();

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

  /** RAI between image space and anatomy space */
  std::string m_ImageToAnatomyRAI;

  /** RAI between anatomy space and image space */
  std::string m_DisplayToAnatomyRAI[3];

  // Slice intensity mapping information
  IntensityCurveInterface::Pointer m_IntensityCurve;

  // Undo data manager. Perhaps this should really be in IRISImageData, but
  // there is a lot of stuff here that is ambiguous in this way. The manager
  // stores 'deltas', i.e., differences between states of the segmentation
  // image. These deltas are compressed, allowing us to store a bunch of 
  // undo steps with little cost in performance or memory
  typedef UndoDataManager<LabelType> UndoManagerType;
  UndoManagerType m_UndoManager;
};

