/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    $RCSfile: ImageIORoutines.h,v $
  Language:  C++
  Date:      $Date: 2006/12/02 04:22:12 $
  Version:   $Revision: 1.1 $
  Copyright (c) 2003 Insight Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notices for more information.
=========================================================================*/
#ifndef __ImageIORoutines_h_
#define __ImageIORoutines_h_

#include "itkImage.h"
#include "itkImageIOBase.h"
#include "itkImageFileReader.h"
#include "itkImageFileWriter.h"

// This file contains a collection of useful IO routines

/** Load an image from a file, throw exception if something happens */
template <class TImageType>
void LoadImageFromFile(const char *fname,
                       typename itk::SmartPointer<TImageType> &target,
                       itk::ImageIOBase *io = 0) throw(itk::ExceptionObject)
{
  // Load the image using itk IO
  typedef itk::ImageFileReader<TImageType> ReaderType;
  typename ReaderType::Pointer reader = ReaderType::New();
  
  reader->SetFileName(fname);
  if(io)
    reader->SetImageIO(io);
  reader->Update();
  target = reader->GetOutput();    
}

/** Save an image to a file, throw exception if something happens */
template <class TImageType>
void SaveImageToFile(const char *fname,
                     TImageType *source,
                     itk::ImageIOBase *io = 0) throw(itk::ExceptionObject)
{
  // Load the image using itk IO
  typedef itk::ImageFileWriter<TImageType> WriterType;
  typename WriterType::Pointer writer = WriterType::New();
  
  writer->SetFileName(fname);
  if(io)
    writer->SetImageIO(io);
  writer->SetInput(source);
  writer->Update();
}

#endif // __ImageIORoutines_h_



