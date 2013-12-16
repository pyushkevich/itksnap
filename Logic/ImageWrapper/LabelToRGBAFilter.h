/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    $RCSfile: LabelToRGBAFilter.h,v $
  Language:  C++
  Date:      $Date: 2009/10/26 16:22:52 $
  Version:   $Revision: 1.2 $
  Copyright (c) 2003 Insight Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notices for more information.
=========================================================================*/
#ifndef __LabelToRGBAFilter_h_
#define __LabelToRGBAFilter_h_

#include "SNAPCommon.h"
#include "itkImage.h"
#include "itkRGBAPixel.h"
#include "itkImageToImageFilter.h"
#include "ColorLabelTable.h"

#include <itkRGBAPixel.h>
#include <itkNumericTraitsRGBAPixel.h>

/**
 * \class LabelToRGBAFilter
 * \brief Simple filter that maps label image to RGB color image
 */
class LabelToRGBAFilter: 
  public itk::ImageToImageFilter<
  itk::Image<LabelType, 2> , itk::Image<itk::RGBAPixel<unsigned char>,2> >
{
public:
  
  /** Pixel Type of the input image */
  typedef LabelType                                      InputPixelType;
  typedef itk::Image<InputPixelType, 2>                  InputImageType;
  typedef itk::SmartPointer<InputImageType>           InputImagePointer;

  /** Pixel Type of the output image */
  typedef itk::RGBAPixel<unsigned char>                 OutputPixelType;
  typedef itk::Image<OutputPixelType, 2>                OutputImageType;
  typedef itk::SmartPointer<OutputImageType>         OutputImagePointer;

  /** Standard class typedefs. */
  typedef LabelToRGBAFilter                                        Self;
  typedef itk::ImageToImageFilter<InputImageType,OutputImageType>  Superclass;
  typedef itk::SmartPointer<Self>                               Pointer;
  typedef itk::SmartPointer<const Self>                    ConstPointer;  
  
  /** Method for creation through the object factory. */
  itkNewMacro(Self)
    
  /** Image dimension. */
  itkStaticConstMacro(ImageDimension, unsigned int,
                      InputImageType::ImageDimension);

  /** Set color table macro */
  void SetColorTable(ColorLabelTable *table)
  {
    m_ColorTable = table;
    this->SetNthInput(1, table);
  }
  
  /** Get color table */
  ColorLabelTable *GetColorTable()
  {
    return m_ColorTable;
  }

protected:

  void PrintSelf(std::ostream& os, itk::Indent indent) const
    { os << indent << "LabelToRGBAFilter"; }
  
  /** Generate Data */
  void GenerateData( void )
    {
    // Here's the input and output
    InputImageType::ConstPointer inputPtr = this->GetInput();
    OutputImageType::Pointer outputPtr = this->GetOutput();

    // Get the number of pixels in the input
    size_t n = inputPtr->GetBufferedRegion().GetNumberOfPixels();

    // Allocate output if needed
    if(outputPtr->GetBufferedRegion().GetNumberOfPixels() != n)
      {
      outputPtr->SetBufferedRegion(inputPtr->GetBufferedRegion());
      outputPtr->Allocate();
      }

    // Get the clear label
    const ColorLabel &clear = m_ColorTable->GetColorLabel(0);
    const ColorLabel *cllast = &clear;

    // Keep track of the last pixel - this reduces the amount of color label
    // lookups, taking advantage of the fact that segmentations are homogeneous
    InputPixelType last_pixel = 0;

    // Simple loop
    const LabelType *xin = inputPtr->GetBufferPointer(), *xinend = xin + n;
    OutputPixelType *xout = outputPtr->GetBufferPointer();
    for(; xin < xinend; ++xin, ++xout)
      {
      if(*xin != last_pixel)
        {
        last_pixel = *xin;
        const ColorLabel &cl = m_ColorTable->GetColorLabel(last_pixel);
        if(cl.IsVisible())
          cllast = &cl;
        else
          cllast = &clear;
        }

      cllast->GetRGBAVector(xout->GetDataPointer());
      }
    }

private:
  ColorLabelTable *m_ColorTable;
};

#endif
