/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    $RCSfile: IRISImageData.h,v $
  Language:  C++
  Date:      $Date: 2007/09/17 04:53:35 $
  Version:   $Revision: 1.3 $
  Copyright (c) 2003 Insight Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notices for more information.
=========================================================================*/
#ifndef __IRISImageData_h_
#define __IRISImageData_h_

#include "SNAPCommon.h"
#include "IRISException.h"
#include "LabelImageWrapper.h"
#include "GreyImageWrapper.h"
#include "RGBImageWrapper.h"
#include "GlobalState.h"
#include "ImageCoordinateGeometry.h"

class IRISApplication;

/**
 * \class IRISImageData
 * \brief This class encapsulates the image data used by 
 * the IRIS component of SnAP.  
 *
 * This data consists of a grey image [gi] and a segmentation image [si].
 * The following rules must be satisfied by this class:
 *  + exists(si) ==> exists(gi)
 *  + if exists(si) then size(si) == size(gi)
 */
class IRISImageData 
{
public:
  // Image type definitions
  typedef GreyImageWrapper::ImageType GreyImageType;
  typedef RGBImageWrapper::ImageType RGBImageType;
  typedef LabelImageWrapper::ImageType LabelImageType;
  typedef itk::ImageRegion<3> RegionType;

  IRISImageData(IRISApplication *parent);
  virtual ~IRISImageData() {};


  /**
   * Access the greyscale image (read only access is allowed)
   */
  GreyImageWrapper* GetGrey() {
    assert(m_GreyWrapper.IsInitialized());
    return &m_GreyWrapper;
  }

  /**
   * Access the RGB image (read only access is allowed)
   */
  RGBImageWrapper* GetRGB() {
    assert(m_GreyWrapper.IsInitialized() && m_RGBWrapper.IsInitialized());
    return &m_RGBWrapper;
  }

  /**
   * Access the segmentation image (read only access allowed 
   * to preserve state)
   */
  LabelImageWrapper* GetSegmentation() {
    assert(m_GreyWrapper.IsInitialized() && m_LabelWrapper.IsInitialized());
    return &m_LabelWrapper;
  }

  /** 
   * Get the extents of the image volume
   */
  Vector3ui GetVolumeExtents() const {
    assert(m_GreyWrapper.IsInitialized());
    assert(m_GreyWrapper.GetSize() == m_Size);
    return m_Size;
  }

  /** 
   * Get the ImageRegion (largest possible region of all the images)
   */
  RegionType GetImageRegion() const;

  /**
   * Get the spacing of the gray scale image (and all the associated images) 
   */
  Vector3d GetImageSpacing();

  /**
   * Get the origin of the gray scale image (and all the associated images) 
   */
  Vector3d GetImageOrigin();

  /**
   * Set the grey image (read important note).
   * 
   * Note: this method replaces the internal pointer to the grey image
   * by the pointer that is passed in.  That means that the caller should relinquish
   * control of this pointer and that the IRISImageData class will dispose of the
   * pointer properly. 
   *
   * The second parameter to this method is the new geometry object, which depends
   * on the size of the grey image and will be updated.
   */
  void SetGreyImage(GreyImageType *newGreyImage,
                    const ImageCoordinateGeometry &newGeometry);

  void SetRGBImage(RGBImageType *newRGBImage,
                    const ImageCoordinateGeometry &newGeometry);
  
  void SetRGBImage(RGBImageType *newRGBImage);
  
  /**
   * This method sets the segmentation image (see note for SetGrey).
   */
  void SetSegmentationImage(LabelImageType *newLabelImage);

  /**
   * Set voxel in segmentation image
   */
  void SetSegmentationVoxel(const Vector3ui &index, LabelType value);

  /**
   * Check validity of greyscale image
   */
  bool IsGreyLoaded();

  /**
   * Check validity of RGB image
   */
  bool IsRGBLoaded();

  /**
   * Check validity of segmentation image
   */
  bool IsSegmentationLoaded();

  /**
   * Set the cursor (crosshairs) position, in pixel coordinates
   */
  virtual void SetCrosshairs(const Vector3ui &crosshairs);

  /**
   * Set the image coordinate geometry for this image set.  Propagates
   * the transform to the internal image wrappers
   */
  virtual void SetImageGeometry(const ImageCoordinateGeometry &geometry);

  /** Get the image coordinate geometry */
  irisGetMacro(ImageGeometry,ImageCoordinateGeometry);

protected:
  // Wrapper around the grey-scale image
  GreyImageWrapper m_GreyWrapper;

  // Wrapper around the RGB image
  RGBImageWrapper m_RGBWrapper;

  // Wrapper around the segmentatoin image
  LabelImageWrapper m_LabelWrapper;

  // A list of linked wrappers, whose cursor position and image geometry
  // are updated concurrently
  std::list<ImageWrapperBase *> m_LinkedWrappers;

  // Dimensions of the images (must match) 
  Vector3ui m_Size;

  // Parent object
  IRISApplication *m_Parent;

  // Image coordinate geometry (it's placed here because the transform depends
  // on image size)
  ImageCoordinateGeometry m_ImageGeometry;
};

#endif
