/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: IRISImageData.h,v $
  Language:  C++
  Date:      $Date: 2009/08/29 23:02:44 $
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
    assert(m_MainImageWrapper->IsInitialized() && m_UndoWrapper.IsInitialized());
    return &m_UndoWrapper;
  }

  /**
   * We override the parent's SetSegmentationImage in order to initialize the
   * undo wrapper to match.
   */
  void SetSegmentationImage(LabelImageType *newLabelImage);

  void SetGreyImage(
    GreyImageType *newGreyImage,
    const ImageCoordinateGeometry &newGeometry,
    const GreyTypeToNativeFunctor &native);

  void SetRGBImage(RGBImageType *newRGBImage,
                   const ImageCoordinateGeometry &newGeometry);

  virtual void UnloadMainImage();

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
