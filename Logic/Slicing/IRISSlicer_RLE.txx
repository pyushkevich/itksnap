/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: IRISSlicer.txx,v $
  Language:  C++
  Date:      $Date: 2007/12/30 04:05:15 $
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
#include "itkDefaultPixelAccessor.h"
#include "itkDefaultPixelAccessorFunctor.h"
#include "itkImageAdaptor.h"
#include "itkImage.h"
#include "itkVectorImage.h"
#include "itkVectorImageToImageAdaptor.h"

//now goes version specialized for RLEImage
//The only difference is ThreadedGenerateData method
template< typename TPixel, typename CounterType, class TOutputImage, class TPreviewImage>
IRISSlicer<RLEImage<TPixel, 3, CounterType>, TOutputImage, TPreviewImage>
::IRISSlicer()
{
  // Two inputs are allowed (second being the preview input)
  this->SetNumberOfIndexedInputs(2);
  this->SetPreviewInput(NULL);

  // There is a single input to the filter
  this->SetNumberOfRequiredInputs(1);

  // There are three outputs from the filter
  this->SetNumberOfRequiredOutputs(1);

  // Initialize the slice to be along the z-direction in the image
  m_SliceDirectionImageAxis = 2;
  m_LineDirectionImageAxis = 1;
  m_PixelDirectionImageAxis = 0;

  m_PixelTraverseForward = true;
  m_LineTraverseForward = true;

  // Initialize to a zero slice index
  m_SliceIndex = 0;
}

template< typename TPixel, typename CounterType, class TOutputImage, class TPreviewImage>
void IRISSlicer<RLEImage<TPixel, 3, CounterType>, TOutputImage, TPreviewImage>
::GenerateOutputInformation()
{
  // Get pointers to the inputs and outputs
  typename Superclass::InputImageConstPointer inputPtr = this->GetInput();
  typename Superclass::OutputImagePointer outputPtr = this->GetOutput();

  // The inputs and outputs should exist
  if (!outputPtr || !inputPtr) return;

  // Get the input's largest possible region
  InputImageRegionType inputRegion = inputPtr->GetLargestPossibleRegion();

  // Arrays to specify the output spacing and origin
  double outputSpacing[2];
  double outputOrigin[2] = { 0.0, 0.0 };

  // Initialize the output image region
  OutputImageRegionType outputRegion;
  outputRegion.SetIndex(0, inputRegion.GetIndex(m_PixelDirectionImageAxis));
  outputRegion.SetSize(0, inputRegion.GetSize(m_PixelDirectionImageAxis));
  outputRegion.SetIndex(1, inputRegion.GetIndex(m_LineDirectionImageAxis));
  outputRegion.SetSize(1, inputRegion.GetSize(m_LineDirectionImageAxis));

  // Set the origin and spacing
  outputSpacing[0] = inputPtr->GetSpacing()[m_PixelDirectionImageAxis];
  outputSpacing[1] = inputPtr->GetSpacing()[m_LineDirectionImageAxis];

  // Set the region of the output slice
  outputPtr->SetLargestPossibleRegion(outputRegion);

  // Set the spacing and origin
  outputPtr->SetSpacing(outputSpacing);
  outputPtr->SetOrigin(outputOrigin);
}

template< typename TPixel, typename CounterType, class TOutputImage, class TPreviewImage>
void IRISSlicer<RLEImage<TPixel, 3, CounterType>, TOutputImage, TPreviewImage>
::CallCopyOutputRegionToInputRegion(InputImageRegionType &destRegion,
                                    const OutputImageRegionType &srcRegion)
{
  // Set the size of the region to 1 in the slice direction
  destRegion.SetSize(m_SliceDirectionImageAxis, 1);

  // Set the index of the region in that dimension to the number of the slice
  destRegion.SetIndex(m_SliceDirectionImageAxis, m_SliceIndex);

  // Compute the bounds of the input region for the other two dimensions (for
  // the case when the output region is not equal to the largest possible
  // region (i.e., we are requesting a partial slice)

  // The size of the region does not depend of the direction of axis
  // traversal
  destRegion.SetSize(m_PixelDirectionImageAxis, srcRegion.GetSize(0));
  destRegion.SetSize(m_LineDirectionImageAxis, srcRegion.GetSize(1));

  // However, the index of the region does depend on the direction!
  if (m_PixelTraverseForward)
    {
    destRegion.SetIndex(m_PixelDirectionImageAxis, srcRegion.GetIndex(0));
    }
  else
    {
    // This case is a bit trickier.  The axis direction is reversed, so
    // range [i,...,i+s-1] in the output image corresponds to the range
    // [S-(i+s),S-(i+1)] in the input image, where i is the in-slice index,
    // S is the largest size of the input and s is the requested size of the
    // output
    destRegion.SetIndex(
          m_PixelDirectionImageAxis,
          this->GetInput()->GetLargestPossibleRegion().GetSize(m_PixelDirectionImageAxis)
          - (srcRegion.GetIndex(0) + srcRegion.GetSize(0)));
    }

  // Same as above for line index
  if (m_LineTraverseForward)
    {
    destRegion.SetIndex(m_LineDirectionImageAxis, srcRegion.GetIndex(1));
    }
  else
    {
    destRegion.SetIndex(
          m_LineDirectionImageAxis,
          this->GetInput()->GetLargestPossibleRegion().GetSize(m_LineDirectionImageAxis)
          - (srcRegion.GetIndex(1) + srcRegion.GetSize(1)));
    }
}

template< typename TPixel, typename CounterType, class TOutputImage, class TPreviewImage>
void IRISSlicer<RLEImage<TPixel, 3, CounterType>, TOutputImage, TPreviewImage>
::GenerateInputRequestedRegion()
{
  // If there is a preview input, and the pipeline of the preview input is
  // older than the main input, we don't ask the preview to generate a new
  // slice. Instead, we leave it's requested region as is, so that the
  // preview does not actually update. This results in a selective behavior,
  // where we choose the preview if it's newer than the main input, otherwise
  // we choose the normal input

  // Actually compute what the input region should be
  InputImageRegionType inputRegion;
  this->CallCopyOutputRegionToInputRegion(
        inputRegion, this->GetOutput()->GetRequestedRegion());

  // Get the main input
  InputImageType *main = const_cast<InputImageType *>(this->GetInput(0));

  // Decide if we want to use the preview input instead
  InputImageType *preview = const_cast<InputImageType *>(this->GetInput(1));

  if (preview)
    {
    if (preview->GetPipelineMTime() > main->GetMTime())
      {
      // We want the preview to be updated
      preview->SetRequestedRegion(inputRegion);
      }
    else
      {
      // Ignore the preview, prevent it from updating itself needlessly
      preview->SetRequestedRegion(preview->GetBufferedRegion());
      }

    main->SetRequestedRegion(inputRegion);
    }
}

#include "RLEImageRegionConstIterator.h"

#define sign(forward) (forward ? 1 : -1)

template< typename TPixel, typename CounterType, class TOutputImage, class TPreviewImage>
void IRISSlicer<RLEImage<TPixel, 3, CounterType>, TOutputImage, TPreviewImage>
::GenerateData()
{
  // Here's the input and output
  const InputImageType *inputPtr = this->GetInput();
  OutputImageType *outputPtr = this->GetOutput();

  // Decide if we want to use the preview input instead
  const InputImageType *preview =
      (InputImageType *) this->GetInputs()[1].GetPointer();

  if (preview && preview->GetMTime() > inputPtr->GetMTime())
    {
    inputPtr = preview;
    }

  this->AllocateOutputs();

  // Important: the size needs to be cast to long to avoid problems with
  // pointer arithmetic on some MSVC versions!
  long szVol[3];
  szVol[0] = inputPtr->GetBufferedRegion().GetSize(0);
  szVol[1] = inputPtr->GetBufferedRegion().GetSize(1);
  szVol[2] = inputPtr->GetBufferedRegion().GetSize(2);

  // Also cast the output size to long
  long szSlice[2];
  szSlice[0] = outputPtr->GetBufferedRegion().GetSize(0);
  szSlice[1] = outputPtr->GetBufferedRegion().GetSize(1);

  // The sign of the line and pixel traversal directions
  int s_line = (m_LineTraverseForward) ? 1 : -1;
  int s_pixel = (m_PixelTraverseForward) ? 1 : -1;

  typename TOutputImage::IndexType oStartInd;
  oStartInd[1] = (m_LineTraverseForward) ? 0 : szSlice[1] - 1;
  oStartInd[0] = (m_PixelTraverseForward) ? 0 : szSlice[0] - 1;

  typename OutputImageType::PixelType *outSlice = &outputPtr->GetPixel(oStartInd);

  if (m_SliceDirectionImageAxis == 2) //slicing along z
    {
#pragma omp parallel for
    for (int y = 0; y < szVol[1]; y++)
      {
      typename InputImageType::BufferType::IndexType lineIndex = { { y, (int) m_SliceIndex } };
      const typename InputImageType::RLLine & line = inputPtr->GetBuffer()->GetPixel(lineIndex);
      if (m_LineDirectionImageAxis == 1) //y is line coordinate
        {
        assert(m_PixelDirectionImageAxis == 0); //x is pixel coordinate
        uncompressLine(line, outSlice + s_line*y*szVol[0], s_pixel * 1);
        }
      else if (m_LineDirectionImageAxis == 0) //x is line coordinate
        {
        assert(m_PixelDirectionImageAxis == 1); //y is pixel coordinate
        uncompressLine(line, outSlice + s_pixel*y, s_line*szVol[1]);
        }
      else
        throw itk::ExceptionObject(__FILE__, __LINE__, "SliceDirectionImageAxis and SliceDirectionImageAxis cannot both have a value of 2!", __FUNCTION__);
      }
    }
  else if (m_SliceDirectionImageAxis == 1) //slicing along y
    {
#pragma omp parallel for
    for (int z = 0; z < szVol[2]; z++)
      {
      typename InputImageType::BufferType::IndexType lineIndex = { { (int) m_SliceIndex, z } };
      const typename InputImageType::RLLine & line = inputPtr->GetBuffer()->GetPixel(lineIndex);
      if (m_LineDirectionImageAxis == 2) //z is line coordinate
        {
        assert(m_PixelDirectionImageAxis == 0); //x is pixel coordinate
        uncompressLine(line, outSlice + s_line*z*szVol[0], s_pixel * 1);
        }
      else if (m_LineDirectionImageAxis == 0) //x is line coordinate
        {
        assert(m_PixelDirectionImageAxis == 2); //z is pixel coordinate
        uncompressLine(line, outSlice + s_pixel*z, s_line*szVol[2]);
        }
      else
        throw itk::ExceptionObject(__FILE__, __LINE__, "SliceDirectionImageAxis and SliceDirectionImageAxis cannot both have a value of 1!", __FUNCTION__);
      }
    }
  else //slicing along x, the low-preformance case
    {
    assert(m_SliceDirectionImageAxis == 0);
#pragma omp parallel for
    for (int z = 0; z < szVol[2]; z++)
      for (int y = 0; y < szVol[1]; y++)
        {
        typename InputImageType::BufferType::IndexType lineIndex = { { y, z } };
        const typename InputImageType::RLLine & line = inputPtr->GetBuffer()->GetPixel(lineIndex);
        int t = 0;
        for (int x = 0; x < line.size(); x++)
          {
          t += line[x].first;
          if (t > m_SliceIndex)
            {
            if (m_LineDirectionImageAxis == 2) //z is line coordinate
              {
              assert(m_PixelDirectionImageAxis == 1); //y is pixel coordinate
              *(outSlice + s_line*z*szVol[1] + s_pixel *y) = line[x].second;;
              }
            else if (m_LineDirectionImageAxis == 1) //y is line coordinate
              {
              assert(m_PixelDirectionImageAxis == 2); //z is pixel coordinate
              *(outSlice + s_pixel*z + s_line *y*szVol[2]) = line[x].second;
              }
            else
              throw itk::ExceptionObject(__FILE__, __LINE__, "SliceDirectionImageAxis and SliceDirectionImageAxis cannot both have a value of 0!", __FUNCTION__);
            break;
            }
          }
        }
    }
}

//template< typename TPixel, typename CounterType, class TOutputImage, class TPreviewImage>
//void IRISSlicer<RLEImage<TPixel, 3, CounterType>, TOutputImage, TPreviewImage>
//::AfterThreadedGenerateData()
//{
//
//}

template< typename TPixel, typename CounterType, class TOutputImage, class TPreviewImage>
void IRISSlicer<RLEImage<TPixel, 3, CounterType>, TOutputImage, TPreviewImage>
::PrintSelf(std::ostream &os, itk::Indent indent) const
{
  Superclass::PrintSelf(os, indent);

  os << indent << "Slice Image Axis: " << m_SliceDirectionImageAxis << std::endl;
  os << indent << "Slice Index: " << m_SliceIndex << std::endl;
  os << indent << "Line Image Axis:  " << m_LineDirectionImageAxis << std::endl;
  os << indent << "Lines Traversed Forward: " << m_LineTraverseForward << std::endl;
  os << indent << "Pixel Image Axis: " << m_PixelDirectionImageAxis << std::endl;
  os << indent << "Pixels Traversed Forward: " << m_PixelTraverseForward << std::endl;
}

template< typename TPixel, typename CounterType, class TOutputImage, class TPreviewImage>
void IRISSlicer<RLEImage<TPixel, 3, CounterType>, TOutputImage, TPreviewImage>
::SetPreviewInput(PreviewImageType *input)
{
  this->SetNthInput(1, input);
}

template< typename TPixel, typename CounterType, class TOutputImage, class TPreviewImage>
typename IRISSlicer<RLEImage<TPixel, 3, CounterType>, TOutputImage, TPreviewImage>::PreviewImageType *
IRISSlicer<RLEImage<TPixel, 3, CounterType>, TOutputImage, TPreviewImage>
::GetPreviewInput()
{
  return static_cast<PreviewImageType *>(itk::ProcessObject::GetInput(1));
}
