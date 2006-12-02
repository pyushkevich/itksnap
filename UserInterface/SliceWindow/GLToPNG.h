/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    $RCSfile: GLToPNG.h,v $
  Language:  C++
  Date:      $Date: 2006/12/02 04:22:27 $
  Version:   $Revision: 1.1 $
  Copyright (c) 2003 Insight Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notices for more information.
=========================================================================*/
#ifndef __GLToPNG_h_
#define __GLToPNG_h_

#include <FL/gl.h>

/**
 * 
 * Simple Functions that convert GL buffer into vtkImageData
 * and save the vtkImageData as PNG file
 * 
 * 01/25/2004
 * Hui Gary Zhang
 *
 */

#include "vtkImageData.h"
#include "vtkPNGWriter.h"

// convert a GL buffer into vtkImageData
vtkImageData* GLToVTKImageData(unsigned int format, int x, int y, int w, int h);

// output vtkImageData as PNG
void VTKImageDataToPNG(vtkImageData* img, const char* filename);

#endif

