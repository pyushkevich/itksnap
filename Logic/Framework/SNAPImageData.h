/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: SNAPImageData.h,v $
  Language:  C++
  Date:      $Date: 2009/01/23 20:09:38 $
  Version:   $Revision: 1.4 $
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
#ifndef __SNAPImageData_h_
#define __SNAPImageData_h_

#include "SNAPCommon.h"
#include "IRISException.h"

#include "GlobalState.h"
#include "ColorLabel.h"
#include "ImageCoordinateGeometry.h"

#include "IRISImageData.h"
#include "SnakeParameters.h"

#include "SNAPLevelSetDriver.h"

#include <vector>

#include "SNAPLevelSetFunction.h"
#include "itkImageAdaptor.h"

namespace itk {
  class Command;
  class FastMutexLock;
}

class SNAPSegmentationROISettings;


/**
 * \class SNAPImageData
 * \brief Wrapper around the SNAP automatic segmentation pipelines.
 *
 * This class encapsulates several images used in the SNAP application, 
 * including the speed image, the bubble-initialization image and the
 * segmentation result image.
 */
class SNAPImageData : public GenericImageData 
{
public:
  irisITKObjectMacro(SNAPImageData, GenericImageData)

  // This class fires LevelSetImageChangeEvent
  FIRES(LevelSetImageChangeEvent)

  // The type of the internal level set image
  typedef itk::Image<float,3>                                   FloatImageType;
  typedef Superclass::AnatomicImageType                      AnatomicImageType;
  typedef SpeedImageWrapper::ImageType                          SpeedImageType;
  typedef LevelSetImageWrapper::ImageType                    LevelSetImageType;

  /** Initialize to an ROI from another image data object */
  void InitializeToROI(GenericImageData *source,
                       const SNAPSegmentationROISettings &roi,
                       itk::Command *progressCommand);

  /**
    Unload all images in the SNAP image data, releasing memory and returning
    this object to initial state.
    */
  void UnloadAll();

  /** 
   * Get the preprocessed (speed) image wrapper
   */
  SpeedImageWrapper* GetSpeed();

  /**
   * Initialize the Speed image wrapper to blank data
   */
  void InitializeSpeed();
  
  /**
   * Clear the preprocessed (speed) image (discard data, etc)
   */
  void ClearSpeed()
    { m_SpeedWrapper->Reset(); }

  /**
   * Check the preprocessed image for validity
   */
  bool IsSpeedLoaded();
  
  /** Get the current snake image wrapper */
  LevelSetImageWrapper* GetSnake();

  /**
   * Clear the current snake (discard data, etc)
   */
  void ClearSnake()
    { m_SnakeWrapper->Reset(); }

  /**
   * Check the current snake for validity
   */
  bool IsSnakeLoaded();

  /**
   * This optional method allows us to load an external advection
   * field. This field can be used when image data includes some
   * directional components, i.e., DTI 
   */
  void SetExternalAdvectionField( FloatImageType *imgX, 
    FloatImageType *imgY, FloatImageType *imgZ);

  /** This method reverts back to using gradient based advection fields */
  void RemoveExternalAdvectionField()
    { m_ExternalAdvectionField = NULL; }

  /** Set the color label used for the segmentation */
  irisSetMacro(ColorLabel, ColorLabel);
  irisGetMacro(ColorLabel, ColorLabel);

  /** =========== Methods dealing with the segmentation pipeline ============ */

  /**
   * This method computes the two-sided distance transform from the array of
   * bubbles passed on and, if the segmentation image is not blank, the pixels
   * in that image.  This method also initializes the level set driver, ie, the
   * pipeline driving the segmentation process.  This method may take a while
   * to run because of the distance transforms.
   * 
   * @return If there were no voxels of labelColor in the union of the 
   * segmentation image with the bubbles, false will be returned and 
   * initialization will not be completed.  Otherwise, true will be returned.
   */
  bool InitializeSegmentation(const SnakeParameters &parameters, 
    const std::vector<Bubble> &bubbles, unsigned int labelColor);

  /** Run the segmentation for a fixed number of iterations */
  void RunSegmentation(unsigned int nIterations);

  /** Revert the segmentation to the beginning */
  void RestartSegmentation();

  /** Check for convergence */
  bool IsEvolutionConverged();

  /** Update the segmentation parameters, can be done either from the 
   * segmentation pipeline callback or on the fly.  This method is smart enough 
   * to reinitialize the level set driver if the Solver parameter changes */
  void SetSegmentationParameters(const SnakeParameters &parameters);

  /** Check if the segmentation is active */
  bool IsSegmentationActive() const
    { return m_LevelSetDriver != NULL; }

  /** Get the number of elapsed iterations */
  unsigned int GetElapsedSegmentationIterations() const;

  /** Release the resources associated with the level set segmentation.  This 
   * method must be called once the segmentation pipeline has terminated, or 
   * else it would create a nasty crash */
  void TerminateSegmentation();

  /** ====================================================================== */
  
  /**
   * Merge the segmentation result with the segmentation contained in a
   * IRIS image data object.
   */
  void MergeSnakeWithIRIS(IRISImageData *target) const;

  /**
   * Get the level set image currently being evolved
   */
  LevelSetImageType *GetLevelSetImage();

  /** This method is public for testing purposes.  It will give a pointer to 
   * the level set function used internally for segmentation */
  SNAPLevelSetDriver<3>::LevelSetFunctionType *GetLevelSetFunction();

  /**
   * SNAPImageData provides a mutex lock that prevents multiple threads from
   * causing the level set pipeline to update at once. In particular this
   * can happen if the meshes are being generated from the level set data
   * in a background thread
   */
  irisGetMacro(LevelSetPipelineMutexLock, itk::FastMutexLock *)

  
protected:

  SNAPImageData();
  ~SNAPImageData();



  /** A functor for inverting an image */
  class InvertFunctor {
  public:
    unsigned char operator()(unsigned char input) 
      { return input == 0 ? 1 : 0; }
  };

  /** Copy nickname, settings, and other such junk from IRIS to SNAP during
   * initialization */
  void CopyLayerMetadata(ImageWrapperBase *target, ImageWrapperBase *source);


  /** Initialize the driver used to control the snake.  This driver is used to
   * run the snake several iterations at a time, without resetting the filter
   * between iteration blocks.  After executing each block of iterations, the
   * filter will call a callback routine, which is provided as a parameter to
   * this method.  In a UI environment, that callback routine should check for
   * user input.  */
  void InitalizeSnakeDriver(const SnakeParameters &param);

  /** A callback used internally to communicate with the LevelSetDriver */
  void IntermediatePauseCallback(
    itk::Object *object,const itk::EventObject &event);

  /** Another callback, used in non-interactive mode */
  void TerminatingPauseCallback();

  /** Type of fommands used for callbacks to the user of this class */
  typedef itk::SmartPointer<itk::Command> CommandPointer;
  
  /** A callback for idle cycle of the segmentation pipeline */
  CommandPointer m_SegmentationIdleCallback;

  /** A callback made after an update in the segmentation pipeline */
  CommandPointer m_SegmentationUpdateCallback;

  // Speed image adata
  SmartPtr<SpeedImageWrapper> m_SpeedWrapper;

  // Wrapper around the level set image
  SmartPtr<LevelSetImageWrapper> m_SnakeWrapper;
  
  // Snake driver
  SNAPLevelSetDriver<3> *m_LevelSetDriver;

  // Label color used for the snake images
  LabelType m_SnakeColorLabel;

  // Current value of snake parameters
  SnakeParameters m_CurrentSnakeParameters;       

  // Typedefs for defining the advection image that can be loaded externally
  typedef itk::FixedArray<float, 3> VectorType;
  typedef itk::Image< VectorType, 3> VectorImageType;
  typedef itk::SmartPointer<VectorImageType> VectorImagePointer;

  // The advection image
  VectorImagePointer m_ExternalAdvectionField;

  // The color label that is used for this segmentation
  ColorLabel m_ColorLabel;

  // SNAPImageData provides a mutex lock that prevents multiple threads from
  // causing the level set pipeline to update at once.
  SmartPtr<itk::FastMutexLock> m_LevelSetPipelineMutexLock;
};







#endif
