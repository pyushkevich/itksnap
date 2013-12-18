/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: IRISImageData.cxx,v $
  Language:  C++
  Date:      $Date: 2009/08/29 23:02:43 $
  Version:   $Revision: 1.8 $
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
// ITK Includes
#include "IRISImageData.h"
#include "itkImage.h"

IRISImageData
::IRISImageData()
{
}

IRISImageData::~IRISImageData()
{
}

void
IRISImageData
::SetSegmentationImage(LabelImageType *newLabelImage)
{
  // Set the new segmentation image
  GenericImageData::SetSegmentationImage(newLabelImage);
}

void
IRISImageData
::ResetSegmentationImage()
{
  GenericImageData::ResetSegmentationImage();
}

void
IRISImageData
::UnloadMainImage()
{
  GenericImageData::UnloadMainImage();
}

