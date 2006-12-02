/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    $RCSfile: SNAPBorlandDummyTypes.h,v $
  Language:  C++
  Date:      $Date: 2006/12/02 04:22:09 $
  Version:   $Revision: 1.1 $
  Copyright (c) 2003 Insight Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notices for more information.
=========================================================================*/
#ifndef __SNAPBorlandDummyTypes_h_
#define __SNAPBorlandDummyTypes_h_

#include "IRISSlicer.h"
#include <itkSmartPointer.h>
#include <itkCovariantVector.h>
typedef IRISSlicer<unsigned char> SNAPBorlandDummyIRISSlicer;
typedef itk::SmartPointer<SNAPBorlandDummyIRISSlicer> SNAPBorlandDummyIRISSlicerSP;
typedef IRISSlicer<float> SNAPBorlandDummyIRISSlicer2;
typedef itk::SmartPointer<SNAPBorlandDummyIRISSlicer2> SNAPBorlandDummyIRISSlicer2SP;

#include <itkImageRegion.h>
#include <itkImageBase.h>
#include <itkImage.h>
#include <itkImageSource.h>
#include <itkImageRegionConstIterator.h>
#include <itkRGBAPixel.h>
#include <itkRGBPixel.h>
#include <itkImageIOBase.h>

typedef itk::ImageBase<2> SNAPBorlandDummyImageBase2Type;
typedef itk::ImageBase<3> SNAPBorlandDummyImageBase3Type;
typedef itk::ImageRegion<3> SNAPBorlandDummyImageRegion3Type;
typedef itk::ImageRegion<2> SNAPBorlandDummyImageRegion2Type;

typedef itk::Image<itk::RGBAPixel<unsigned char>,2> SNAPBorlandDummyImageType;
typedef itk::Image<unsigned char,3> SNAPBorlandDummyImageType3;
typedef itk::Image<short,3> SNAPBorlandDummyImageType2;
typedef itk::Image<float,2> SNAPBorlandDummyImageType4;
typedef itk::Image<short,2> SNAPBorlandDummyImageType5;
typedef itk::Image<double,3> SNAPBorlandDummyImageType6;
typedef itk::Image<unsigned char,2> SNAPBorlandDummyImageType7;
typedef itk::Image<float,3> SNAPBorlandDummyImageType8;
typedef itk::Image<unsigned char,3> SNAPBorlandDummyImageType9;
typedef itk::Image<itk::RGBPixel<unsigned char>,2> SNAPBorlandDummyImageType10;
typedef itk::Image<itk::CovariantVector<float,2>,2> SNAPBorlandDummyImageType11;

typedef itk::ImageSource<SNAPBorlandDummyImageType5> SNAPBorlandDummyImageSourceType5;
typedef itk::ImageSource<SNAPBorlandDummyImageType6> SNAPBorlandDummyImageSourceType2;
typedef itk::ImageSource<SNAPBorlandDummyImageType7> SNAPBorlandDummyImageSourceType3;
typedef itk::ImageSource<SNAPBorlandDummyImageType> SNAPBorlandDummyImageSourceType;
typedef itk::ImageSource<SNAPBorlandDummyImageType9> SNAPBorlandDummyImageSourceType9;

typedef itk::ImageRegionConstIterator<SNAPBorlandDummyImageType> SNAPBorlandDummyImageTypeIterator;
typedef itk::ImageRegionConstIterator<SNAPBorlandDummyImageType2> SNAPBorlandDummyImageType2Iterator;
typedef itk::ImageRegionConstIterator<SNAPBorlandDummyImageType3> SNAPBorlandDummyImageType3Iterator;
typedef itk::ImageRegionConstIterator<SNAPBorlandDummyImageType4> SNAPBorlandDummyImageType4Iterator;
typedef itk::ImageRegionConstIterator<SNAPBorlandDummyImageType5> SNAPBorlandDummyImageType5Iterator;
typedef itk::ImageRegionConstIterator<SNAPBorlandDummyImageType6> SNAPBorlandDummyImageType6Iterator;
typedef itk::ImageRegionConstIterator<SNAPBorlandDummyImageType7> SNAPBorlandDummyImageType7Iterator;
typedef itk::ImageRegionConstIterator<SNAPBorlandDummyImageType8> SNAPBorlandDummyImageType8Iterator;
typedef itk::ImageRegionConstIterator<SNAPBorlandDummyImageType10> SNAPBorlandDummyImageType10Iterator;

typedef itk::Size<3> SNAPBorlandDummySizeType;
typedef itk::ImageIOBase SNAPBorlandDummyImageIOBaseType;

#endif // __SNAPBorlandDummyTypes_h_
