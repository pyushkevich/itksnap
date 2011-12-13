/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: GenericImageData.h,v $
  Language:  C++
  Date:      $Date: 2009/08/29 23:02:43 $
  Version:   $Revision: 1.11 $
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
#ifndef __GenericImageData_h_
#define __GenericImageData_h_

#include "SNAPCommon.h"
#include "IRISException.h"
#include "GreyImageWrapper.h"
#include "LabelImageWrapper.h"
#include "RGBImageWrapper.h"
#include "GlobalState.h"
#include "ImageCoordinateGeometry.h"

template <class TPixel> class GreyImageWrapper;
template <class TPixel> class RGBImageWrapper;
class LabelImageWrapper;

class IRISApplication;

class GenericImageData;

class LayerIterator
{
public:
  enum LayerRole
  {
    MAIN_ROLE = 0x0001,
    OVERLAY_ROLE = 0x0002,
    LABEL_ROLE = 0x0004,
    SNAP_ROLE = 0x0008,
    NO_ROLE = 0x0010
  };

  LayerIterator(GenericImageData *data, int role_filter = 0x11111111);

  bool IsAtEnd();

  LayerIterator & operator++();

  /** Get the layer being pointed to */
  ImageWrapperBase *GetLayer();

  /** Get the layer being pointed to, cast as Gray (or NULL) */
  GreyImageWrapperBase *GetLayerAsGray();

  /** Get the layer being pointed to, cast as RGB (or NULL) */
  RGBImageWrapperBase *GetLayerAsRGB();

  /** Get the role of the current layer */
  LayerRole GetRole();

private:

  void FindNextUsableRole();

  typedef std::map<LayerRole, std::vector<ImageWrapperBase*> > WrapperStorage;

  GenericImageData *m_ImageData;
  WrapperStorage *m_Wrappers;
  int m_RoleFilter;

  // The iterator bit
  LayerRole m_IterRole;
  std::vector<ImageWrapperBase *>::iterator m_Iter;
};


/**
 * \class GenericImageData
 * \brief This class encapsulates the image data used by 
 * the IRIS component of SnAP.  
 *
 * This data consists of a grey image [gi] and a segmentation image [si].
 * The following rules must be satisfied by this class:
 *  + exists(si) ==> exists(gi)
 *  + if exists(si) then size(si) == size(gi)
 */
class GenericImageData 
{
public:
  // Image type definitions
  typedef itk::ImageRegion<3> RegionType;
  typedef itk::ImageBase<3> ImageBaseType;

  typedef itk::OrientedImage<GreyType, 3> GreyImageType;
  typedef itk::OrientedImage<RGBType, 3> RGBImageType;
  typedef itk::OrientedImage<LabelType, 3> LabelImageType;

  typedef std::vector<ImageWrapperBase *> WrapperList;
  typedef WrapperList::iterator WrapperIterator;
  typedef WrapperList::const_iterator WrapperConstIterator;

  GenericImageData(IRISApplication *parent);
  virtual ~GenericImageData();

  /** 
   Access the 'main' image, either grey or RGB. The main image is the
   one that all other images must mimic. This object will be destroyed
   when a new image is loaded. This means that downstream objects should
   not make copies of this pointer.
   */
  ImageWrapperBase* GetMain()
  {
    assert(m_MainImageWrapper->IsInitialized());
    return m_MainImageWrapper;
  }

  bool IsMainLoaded() const
  {
    return m_MainImageWrapper && m_MainImageWrapper->IsInitialized();
  }

  /**
    Access the main image as a scalar image.
   */
  GreyImageWrapperBase* GetGrey()
  {
    assert(m_GreyImageWrapper && m_GreyImageWrapper->IsInitialized());
    return m_GreyImageWrapper;
  }

  /**
   * Access the RGB image (read only access is allowed)
   */
  RGBImageWrapperBase* GetRGB() {
    assert(m_RGBImageWrapper && m_RGBImageWrapper->IsInitialized());
    return m_RGBImageWrapper;
  }

  /**
   * Access the overlay images (read only access is allowed)
   */
  /*
  WrapperList* GetOverlays() {
    return &m_OverlayWrappers;
  }
  */

  /**
   * Get the total number of layers (counting main and overlays). If main
   * has not been loaded, returns 0
   */
  // virtual unsigned int GetNumberOfLayers() const;


  /**
    Get the number of layers in certain role(s). This is not as fast
    as calling GetLayers(role).size(), but you can query for combinations
    of roles, i.e., MAIN_ROLE | OVERLAY_ROLE
    */
  virtual unsigned int GetNumberOfLayers(int role_filter = 0x11111111);

  /**
    Get one of the layers (counting main and overlays)
    */
  // virtual ImageWrapperBase* GetLayer(unsigned int layer) const;

  /**
   * Get layer as a gray image type. If the layer is not of gray type, NULL
   * will be returned.
   */
  // virtual GreyImageWrapperBase* GetLayerAsGray(unsigned int layer) const;

  /**
   * Get layer as an RGB image type. If the layer is not of RGB type, NULL
   * will be returned.
   */
  // virtual RGBImageWrapperBase* GetLayerAsRGB(unsigned int layer) const;

  /**
   * Access the segmentation image (read only access allowed 
   * to preserve state)
   */
  LabelImageWrapper* GetSegmentation() {
    assert(m_MainImageWrapper->IsInitialized() && m_LabelWrapper->IsInitialized());
    return m_LabelWrapper;
  }

  /** 
   * Get the extents of the image volume
   */
  Vector3ui GetVolumeExtents() const {
    assert(m_MainImageWrapper->IsInitialized());
    return m_MainImageWrapper->GetSize();
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
   * control of this pointer and that the GenericImageData class will dispose of the
   * pointer properly. 
   *
   * The second parameter to this method is the new geometry object, which depends
   * on the size of the grey image and will be updated.
   */
  virtual void SetGreyImage(GreyImageType *image,
                            const ImageCoordinateGeometry &newGeometry,
                            const InternalToNativeFunctor &native);

  virtual void SetRGBImage(RGBImageType *image,
                           const ImageCoordinateGeometry &newGeometry);

  virtual void UnloadMainImage();

  virtual void AddGreyOverlay(GreyImageType *image,
                              const InternalToNativeFunctor &native);

  virtual void AddRGBOverlay(RGBImageType *image);

  virtual void UnloadOverlays();
  virtual void UnloadOverlayLast();

  /**
   * This method sets the segmentation image (see note for SetGrey).
   */
  virtual void SetSegmentationImage(LabelImageType *newLabelImage);

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
   * Check validity of overlay images
   */
  bool IsOverlayLoaded();

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
  irisGetMacro(ImageGeometry,ImageCoordinateGeometry)

protected:
  virtual void SetMainImageCommon(const ImageCoordinateGeometry &geometry);
  virtual void AddOverlayCommon(ImageWrapperBase *wrapper);
  virtual void SetCrosshairs(ImageWrapperBase *wrapper, const Vector3ui &crosshairs);
  virtual void SetImageGeometry(ImageWrapperBase *wrapper, const ImageCoordinateGeometry &geometry);

  /*
  virtual ImageWrapperBase* GetNextLayer(
      int iLayer, LayerIterator::LayerRole role);
      */

  typedef LayerIterator::LayerRole LayerRole;

  // The base storage for the layers in the image data. For each role, there
  // is a list of wrappers serving in that role. For many roles, there will
  // be only one wrapper serving in that role.
  typedef std::map<LayerRole, WrapperList> WrapperStorage;

  // This is where the all the wrappers are maintained. Child classes should
  // aslo add their own wrappers to this list of wrappers.
  WrapperStorage m_Wrappers;

  // A pointer to the 'main' image, i.e., the image that is treated as the
  // reference for all other images. It is typically the grey image, but
  // since we now allow for RGB images, it can point to the RGB image too
  // Equal to m_Wrappers[MAIN].first()
  ImageWrapperBase *m_MainImageWrapper;

  // This pointer is NULL if the main image is of RGB type, and equal to
  // m_MainImageWrapper otherwise.
  GreyImageWrapper<GreyType> *m_GreyImageWrapper;

  // Vice versa
  RGBImageWrapper<unsigned char> *m_RGBImageWrapper;

  // Wrapper around the segmentatoin image.
  // Equal to m_Wrappers[SEGMENTATION].first()
  LabelImageWrapper *m_LabelWrapper;

  // A list of linked wrappers, whose cursor position and image geometry
  // are updated concurrently
  // WrapperList m_MainWrappers;
  // WrapperList m_OverlayWrappers;

  // Parent object
  IRISApplication *m_Parent;

  // Image coordinate geometry (it's placed here because the transform depends
  // on image size)
  ImageCoordinateGeometry m_ImageGeometry;

  friend class SNAPImageData;
  friend class LayerIterator;
};

#endif
