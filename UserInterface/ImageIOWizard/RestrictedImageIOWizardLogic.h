/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: RestrictedImageIOWizardLogic.h,v $
  Language:  C++
  Date:      $Date: 2009/07/22 21:06:24 $
  Version:   $Revision: 1.6 $
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
#ifndef __RestrictedImageIOWizardLogic_h_
#define __RestrictedImageIOWizardLogic_h_

#include "ImageIOWizardLogic.h"
#include "ImageWrapper.h"

/**
 * \class RestrictedImageIOWizardLogic.h
 * \brief A wizard for loading and saving images whose size and spacing are 
 * restricted by another (in our case, greyscale) image
 */
class RestrictedImageIOWizardLogic : public ImageIOWizardLogic
{
public:
  typedef itk::ImageBase<3> ImageBaseType;
  typedef ImageIOWizardLogic Superclass;
  typedef itk::Size<3> SizeType;

  /** Constructor */
  RestrictedImageIOWizardLogic();

  /** Some extra work is done before displaying the wizard in this method */
  virtual bool DisplayInputWizard(const char *file, const char *type = NULL);

  /** Set the main image that provides some metadata for loading the 
   * segmentation image */
  irisSetMacro(MainImage,ImageBaseType *);

  /** Set a restriction on the number of components */
  irisSetMacro(NumberOfComponentsRestriction, size_t);

protected:

  // Validity of the image is checked by comparing the image size to the
  // size of the greyscale image
  virtual bool CheckImageValidity();

  // The image orientation is set to match the orientation of the source image
  virtual void GuessImageOrientation() {};

  // Restriction on the number of components
  size_t m_NumberOfComponentsRestriction;


private:
  /** Main image template */
  ImageBaseType *m_MainImage;
};

#endif

