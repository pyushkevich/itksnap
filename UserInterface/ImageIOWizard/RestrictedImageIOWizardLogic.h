/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    $RCSfile: RestrictedImageIOWizardLogic.h,v $
  Language:  C++
  Date:      $Date: 2006/12/02 04:22:22 $
  Version:   $Revision: 1.1 $
  Copyright (c) 2003 Insight Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notices for more information.
=========================================================================*/
#ifndef __RestrictedImageIOWizardLogic_h_
#define __RestrictedImageIOWizardLogic_h_

#include "ImageIOWizardLogic.h"

/**
 * \class RestrictedImageIOWizardLogic.h
 * \brief A wizard for loading and saving images whose size and spacing are 
 * restricted by another (in our case, greyscale) image
 */
template<class TPixel>
class RestrictedImageIOWizardLogic : public ImageIOWizardLogic<TPixel>
{
public:
  typedef ImageIOWizardLogic<TPixel> Superclass;
  typedef typename itk::Size<3> SizeType;

  typedef typename itk::Image<GreyType,3> GreyImageType;
  typedef typename itk::SmartPointer<GreyImageType> GreyImagePointer;

  /** Some extra work is done before displaying the wizard in this method */
  virtual bool DisplayInputWizard(const char *file);

  /** Set the grey image that provides some metadata for loading the 
   * segmentation image */
  irisSetMacro(GreyImage,GreyImageType *);

protected:

  // Validity of the image is checked by comparing the image size to the
  // size of the greyscale image
  virtual bool CheckImageValidity();

  // The image orientation is set to match the orientation of the source image
  virtual void GuessImageOrientation() {};

private:
  /** Grey image template */
  GreyImagePointer m_GreyImage;
};

// TXX not included on purporse

#endif

