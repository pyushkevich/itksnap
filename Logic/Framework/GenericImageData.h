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
#include "ImageWrapperTraits.h"
#include <itkObject.h>
#include "GlobalState.h"
#include "ImageCoordinateGeometry.h"
#include <string>
#include "LayerIterator.h"

class IRISApplication;
class GenericImageData;
class LayerIterator;
class Registry;

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
class GenericImageData : public itk::Object
{
public:
  irisITKObjectMacro(GenericImageData, itk::Object)

  // Image type definitions
  typedef itk::ImageRegion<3> RegionType;
  typedef itk::ImageBase<3> ImageBaseType;

  /**
   * The type of anatomical images. For the time being, all anatomic images
   * are made to be of type short. Eventually, it may make sense to allow
   * both short and char images, to save memory in some cases. However, it
   * is not that common to only have 8-bit precision, so for the time being
   * we are going to stick to short
   */
  typedef AnatomicImageWrapper::ImageType                   AnatomicImageType;
  typedef LabelImageWrapper::ImageType                         LabelImageType;


  // Support for lists of wrappers
  typedef SmartPtr<ImageWrapperBase> WrapperPointer;
  typedef std::vector<WrapperPointer> WrapperList;
  typedef WrapperList::iterator WrapperIterator;
  typedef WrapperList::const_iterator WrapperConstIterator;

  /**
   * Set the parent driver
   */
  irisGetSetMacro(Parent, IRISApplication *)

  /** 
   Access the 'main' image, either grey or RGB. The main image is the
   one that all other images must mimic. This object will be destroyed
   when a new image is loaded. This means that downstream objects should
   not make copies of this pointer.
   */
  AnatomicImageWrapper* GetMain()
  {
    assert(m_MainImageWrapper->IsInitialized());
    return m_MainImageWrapper;
  }

  bool IsMainLoaded() const
  {
    return m_MainImageWrapper && m_MainImageWrapper->IsInitialized();
  }

  /**
    Get the number of layers in certain role(s). This is not as fast
    as calling GetLayers(role).size(), but you can query for combinations
    of roles, i.e., MAIN_ROLE | OVERLAY_ROLE
    */
  virtual unsigned int GetNumberOfLayers(int role_filter = 0xffffffff);


  /**
    Get an iterator that iterates throught the layers in certain roles
    */
  LayerIterator GetLayers(int role_filter = 0xffffffff)
  {
    return LayerIterator(this, role_filter);
  }

  /**
    Get one of the layers (counting main and overlays). This is the same as
    calling GetLayers(role_filter) and then iterating n-times. Throws an
    exception if n exceeds the number of layers.
    */
  ImageWrapperBase *GetNthLayer(int n, int role_filter = 0xffffffff)
  {
    LayerIterator it(this, role_filter);
    for(int i = 0; i < n && !it.IsAtEnd(); i++)
      ++it;
    if(it.IsAtEnd())
      throw IRISException("Illegal layer (%d of %d) requested",
                          n, GetNumberOfLayers());
    return it.GetLayer();
  }

  /**
    Find a layer given the layer's unique id. The role_filter restricts the
    search to specific layers, and the search_derived flag enables searching
    among the derived (component, mean) wrappers in vector wrappers.
    */
  ImageWrapperBase *FindLayer(unsigned long unique_id, bool search_derived,
                              int role_filter = 0xffffffff);


  int GetNumberOfOverlays();

  ImageWrapperBase *GetLastOverlay();

  // virtual ImageWrapperBase* GetLayer(unsigned int layer) const;

  /**
   * Access the segmentation image (read only access allowed 
   * to preserve state)
   */
  LabelImageWrapper* GetSegmentation()
  {
    assert(m_MainImageWrapper->IsInitialized() && m_LabelWrapper->IsInitialized());
    return m_LabelWrapper;
  }

  /** 
   * Get the extents of the image volume
   */
  Vector3ui GetVolumeExtents() const
  {
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
   * Set the main image. The main image is the anatomical image that defines
   * the coordinate space of all other images in a SNAP session. It is the
   * image in which structures are traced. The main image can have multiple
   * components or channels (e.g., red, green, blue).
   *
   * Note: this method replaces the internal pointer to the grey image
   * by the pointer that is passed in.  That means that the caller should relinquish
   * control of this pointer and that the GenericImageData class will dispose of the
   * pointer properly.
   *
   * The second parameter to this method is the new geometry object, which depends
   * on the size of the grey image and will be updated.
   *
   * The third parameter is a functor that maps intensity values in each channel
   * from the internal type (short) to the native type (float or int, depending
   * on how the image was stored on disk)
   */
  virtual void SetMainImage(AnatomicImageType *image,
                            const ImageCoordinateGeometry &newGeometry,
                            const LinearInternalToNativeIntensityMapping &native);


  /** Unload the main image (and everything else) */
  virtual void UnloadMainImage();

  /**
   * Reset the segmentation wrapper. This happens when the main image is loaded
   * or when the user asks for a new segmentation image
   */
  virtual void ResetSegmentationImage();

  /** Handle overlays */
  virtual void AddOverlay(AnatomicImageType *image,
                          const LinearInternalToNativeIntensityMapping &native);
  virtual void UnloadOverlays();
  virtual void UnloadOverlayLast();
  virtual void UnloadOverlay(ImageWrapperBase *overlay);

  /**
   * Change the ordering of the layers within a particular role (for now just
   * overlays are supported in the GUI) by moving the specified layer up or
   * down one spot. The sign of the direction determines whether the layer is
   * moved up or down.
   */
  virtual void MoveLayer(ImageWrapperBase *layer, int direction);

  /**
   * This method sets the segmentation image (see note for SetGrey).
   */
  virtual void SetSegmentationImage(LabelImageType *newLabelImage);

  /**
   * Set voxel in segmentation image
   */
  void SetSegmentationVoxel(const Vector3ui &index, LabelType value);

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

  GenericImageData();
  virtual ~GenericImageData();

  // The base storage for the layers in the image data. For each role, there
  // is a list of wrappers serving in that role. For many roles, there will
  // be only one wrapper serving in that role.
  typedef std::map<LayerRole, WrapperList> WrapperStorage;

  // This is where the all the wrappers are maintained. Child classes should
  // aslo add their own wrappers to this list of wrappers.
  WrapperStorage m_Wrappers;

  // A pointer to the 'main' image, i.e., the image that is treated as the
  // reference for all other images.
  // Equal to m_Wrappers[MAIN].first()
  AnatomicImageWrapper *m_MainImageWrapper;

  // Wrapper around the segmentatoin image.
  // Equal to m_Wrappers[SEGMENTATION].first()
  SmartPtr<LabelImageWrapper> m_LabelWrapper;

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
  friend class JOINImageData;
  friend class LayerIterator;

  // Append an image wrapper to a role
  void PushBackImageWrapper(LayerRole role, ImageWrapperBase *wrapper);
  void PopBackImageWrapper(LayerRole role);
  void RemoveImageWrapper(LayerRole role, ImageWrapperBase *wrapper);

  // For roles that only have one wrapper
  void SetSingleImageWrapper(LayerRole, ImageWrapperBase *wrapper);
  void RemoveSingleImageWrapper(LayerRole);

};

#endif
