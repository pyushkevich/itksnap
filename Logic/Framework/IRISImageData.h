/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    $RCSfile: IRISImageData.h,v $
  Language:  C++
  Date:      $Date: 2007/12/25 15:46:23 $
  Version:   $Revision: 1.4 $
  Copyright (c) 2003 Insight Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notices for more information.
=========================================================================*/
#ifndef __IRISImageData_h_
#define __IRISImageData_h_

#include "GenericImageData.h"

/**
 * \class IRISImageData
 * \brief This class encapsulates the image data used by 
 * the IRIS component of SnAP.  
 */
class IRISImageData : public GenericImageData
{
public:

  IRISImageData(IRISApplication *parent) 
    : GenericImageData(parent) {}
  virtual ~IRISImageData() {};

  /**
   * Access the segmentation image (read only access allowed 
   * to preserve state)
   */
  LabelImageWrapper* GetUndoImage() {
    assert(m_GreyWrapper.IsInitialized() && m_UndoWrapper.IsInitialized());
    return &m_UndoWrapper;
  }

  /**
   * We override the parent's SetSegmentationImage in order to initialize the
   * undo wrapper to match.
   */
  void SetSegmentationImage(LabelImageType *newLabelImage)
    {
    GenericImageData::SetSegmentationImage(newLabelImage);
    m_UndoWrapper.InitializeToWrapper(&m_LabelWrapper, (LabelType) 0);
    }

  void SetGreyImage(GreyImageType *newGreyImage,
                    const ImageCoordinateGeometry &newGeometry) 
    {
    GenericImageData::SetGreyImage(newGreyImage, newGeometry);
    m_UndoWrapper.InitializeToWrapper(&m_LabelWrapper, (LabelType) 0);
    }

  void SetRGBImage(RGBImageType *newRGBImage,
                   const ImageCoordinateGeometry &newGeometry) 
    {
    GenericImageData::SetRGBImage(newRGBImage, newGeometry);
    m_UndoWrapper.InitializeToWrapper(&m_LabelWrapper, (LabelType) 0);
    }

  void SetRGBImage(RGBImageType *newRGBImage)
    {
    GenericImageData::SetRGBImage(newRGBImage);
    }


protected:

  // Starting with SNAP 1.6, the IRISImageData object will store a second
  // copy of the segmentation image for the purpose of implementing fast 
  // undo functionality. This image is used to support situations where we
  // want to allow multiple updates to the segmentation image between saving
  // 'undo points'. This is necessary for paintbrush operation, since it would
  // be too expensive to store an undo point for every movement of the paintbrush
  // (and it would be difficult for the user too). So this UndoWrapper stores the
  // segmentation image _at the last undo point_. See IRISApplication::StoreUndoPoint
  // for details.
  LabelImageWrapper m_UndoWrapper;

};

#endif
