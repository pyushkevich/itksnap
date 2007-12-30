/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: PreprocessingImageIOWizardLogic.h,v $
  Language:  C++
  Date:      $Date: 2007/12/30 04:05:16 $
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
#ifndef __PreprocessingImageIOWizardLogic_h_
#define __PreprocessingImageIOWizardLogic_h_

#include "SNAPCommonUI.h"
#include "RestrictedImageIOWizardLogic.h"

/**
 * \class PreprocessingImageIOWizardLogic
 * \brief A concrete instantiation of a wizard for loading and saving 
 * floating point images
 */
class PreprocessingImageIOWizardLogic : 
  public RestrictedImageIOWizardLogic<float> 
{
public:
  typedef RestrictedImageIOWizardLogic<float> Superclass;
  typedef Superclass::FileFormat FileFormat;

  /** Not all file types support saving and loading floating 
   * point images. This method returns the allowed types */
  virtual bool CanLoadFileFormat(FileFormat type) const
  {
    return Superclass::CanLoadFileFormat(type) && (
      type == GuidedImageIOBase::FORMAT_MHA || 
      type == GuidedImageIOBase::FORMAT_ANALYZE || 
      type == GuidedImageIOBase::FORMAT_NRRD || 
      type == GuidedImageIOBase::FORMAT_RAW);
  }

  /** Not all file types support saving and loading floating 
   * point images. This method returns the allowed types */
  virtual bool CanSaveFileFormat(FileFormat type) const
  {
    return Superclass::CanSaveFileFormat(type) && CanLoadFileFormat(type);
  }

};

#endif

