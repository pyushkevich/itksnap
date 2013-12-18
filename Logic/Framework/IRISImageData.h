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
  irisITKObjectMacro(IRISImageData, GenericImageData)

  typedef Superclass::ImageBaseType ImageBaseType;
  typedef Superclass::AnatomicImageType AnatomicImageType;

  /**
   * We override the parent's SetSegmentationImage in order to initialize the
   * undo wrapper to match.
   */
  void SetSegmentationImage(LabelImageType *newLabelImage);

  /**
   * We override the parent's ResetSegmentationImage in order to initialize the
   * undo wrapper to match.
   */
  void ResetSegmentationImage();


  virtual void UnloadMainImage();

protected:

  IRISImageData();
  virtual ~IRISImageData();
};

#endif
