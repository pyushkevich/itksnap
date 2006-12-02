/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    $RCSfile: GLToPNG.cxx,v $
  Language:  C++
  Date:      $Date: 2006/12/02 04:22:27 $
  Version:   $Revision: 1.1 $
  Copyright (c) 2003 Insight Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notices for more information.
=========================================================================*/
#include "GLToPNG.h"

#include <iostream>
using namespace std;

vtkImageData* GLToVTKImageData(unsigned int format, int x, int y, int w, int h) 
{
  // Cast the format to GL enum
  GLenum glformat = (GLenum) format;

  // OSX does double buffer automatically (???)
  //glReadBuffer(GL_BACK);
  unsigned int GL_comps = 0;
  if (glformat == GL_RGBA) 
    {
    GL_comps = 4;
    } 
  else if (glformat == GL_RGB) 
    {
    GL_comps = 3;
    } 
  else 
    {
    cerr << "Invalid GLenum" << endl;
    exit(1);
    }

  unsigned char* pixmap = new unsigned char[w*h*GL_comps];
  glReadPixels(x, y, w, h, format, GL_UNSIGNED_BYTE, pixmap);

  // convert to vtkImageData
  vtkImageData* img = vtkImageData::New();
  if (format == GL_RGBA) 
    {
    img->SetExtent(0, w-1, 0, h-1, 0, 0);
    } 
  else if (format == GL_RGB) 
    {
    img->SetExtent(0, w, 0, h, 0, 0);
    }
  img->SetSpacing(1.0, 1.0, 1.0);
  img->SetOrigin(0.0, 0.0, 0.0);
  img->SetNumberOfScalarComponents(GL_comps);
  img->SetScalarType(VTK_UNSIGNED_CHAR);
  int rowSize = w*GL_comps;
  unsigned char* pixmap2 = pixmap;
  unsigned char* imgPtr = (unsigned char*) img->GetScalarPointer(0, 0, 0);
  for (int i = 0; i < h; ++i) 
    {
    memcpy(imgPtr, pixmap2, rowSize);
    imgPtr += rowSize;
    pixmap2 += rowSize;
    }
  delete[] pixmap;
  return img;
}

void VTKImageDataToPNG(vtkImageData* img, const char* filename)
{
  vtkPNGWriter* pngw = vtkPNGWriter::New();
  pngw->SetInput(img);
  pngw->SetFileName(filename);
  pngw->Write();
}

