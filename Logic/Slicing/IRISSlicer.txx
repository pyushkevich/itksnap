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

template <class TInputImage, class TOutputImage>
IRISSlicer<TInputImage, TOutputImage>
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

  m_BypassMainInput = false;
}

template <class TInputImage, class TOutputImage>
void IRISSlicer<TInputImage, TOutputImage>
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
  double outputOrigin[2] = {0.0,0.0};
  
  // Initialize the output image region
  OutputImageRegionType outputRegion; 
  outputRegion.SetIndex(0,inputRegion.GetIndex(m_PixelDirectionImageAxis));
  outputRegion.SetSize(0,inputRegion.GetSize(m_PixelDirectionImageAxis));
  outputRegion.SetIndex(1,inputRegion.GetIndex(m_LineDirectionImageAxis));
  outputRegion.SetSize(1,inputRegion.GetSize(m_LineDirectionImageAxis));

  // Set the origin and spacing
  outputSpacing[0] = inputPtr->GetSpacing()[m_PixelDirectionImageAxis];
  outputSpacing[1] = inputPtr->GetSpacing()[m_LineDirectionImageAxis];
      
  // Set the region of the output slice
  outputPtr->SetLargestPossibleRegion(outputRegion);

  // Set the spacing and origin
  outputPtr->SetSpacing(outputSpacing);
  outputPtr->SetOrigin(outputOrigin);
}

template <class TInputImage, class TOutputImage>
void IRISSlicer<TInputImage, TOutputImage>
::CallCopyOutputRegionToInputRegion(InputImageRegionType &destRegion,
                                    const OutputImageRegionType &srcRegion)
{
  // Set the size of the region to 1 in the slice direction
  destRegion.SetSize(m_SliceDirectionImageAxis,1);

  // Set the index of the region in that dimension to the number of the slice
  destRegion.SetIndex(m_SliceDirectionImageAxis,m_SliceIndex);

  // Compute the bounds of the input region for the other two dimensions (for 
  // the case when the output region is not equal to the largest possible 
  // region (i.e., we are requesting a partial slice)

  // The size of the region does not depend of the direction of axis 
  // traversal
  destRegion.SetSize(m_PixelDirectionImageAxis,srcRegion.GetSize(0));
  destRegion.SetSize(m_LineDirectionImageAxis,srcRegion.GetSize(1));

  // However, the index of the region does depend on the direction!
  if(m_PixelTraverseForward)
    {
    destRegion.SetIndex(m_PixelDirectionImageAxis,srcRegion.GetIndex(0));
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
  if(m_LineTraverseForward)
    {
    destRegion.SetIndex(m_LineDirectionImageAxis,srcRegion.GetIndex(1));
    }
  else
    {    
    destRegion.SetIndex(
      m_LineDirectionImageAxis,
      this->GetInput()->GetLargestPossibleRegion().GetSize(m_LineDirectionImageAxis)
      - (srcRegion.GetIndex(1) + srcRegion.GetSize(1)));
    }
}

template <class TInputImage, class TOutputImage>
void
IRISSlicer<TInputImage, TOutputImage>
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

  if(preview)
    {
    if(m_BypassMainInput || preview->GetPipelineMTime() > main->GetMTime())
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

#include "itkImageLinearIteratorWithIndex.h"
#include "itkImageRegionConstIterator.h"
#include "itkImageConstIterator.h"

/*
template <class TInputImage, class TOutputImage>
void
IRISSlicer<TInputImage, TOutputImage>
::ThreadedGenerateData(
    const OutputImageRegionType &outputRegionForThread,
    ThreadIdType threadId)
{
  // Here's the input and output
  const InputImageType *inputPtr = this->GetInput();
  OutputImageType *outputPtr = this->GetOutput();

  // Decide if we want to use the preview input instead
  const InputImageType *preview =
      (InputImageType *) this->GetInputs()[1].GetPointer();

  if(preview && preview->GetMTime() > inputPtr->GetMTime())
    {
    inputPtr = preview;
    }

  // Allocate (why is this necessary?)
  this->AllocateOutputs();

  // Get the image dimensions
  typename InputImageType::SizeType szVol = inputPtr->GetBufferedRegion().GetSize();

  // Set the strides in image coordinates
  Vector3i stride_image(1, szVol[0], szVol[1] * szVol[0]);

  // Scale the strides by the number of components. We can't get it from the
  // image itself since the image may be an ImageAdaptor, so we need to use
  // the pixel container's size
  long nintpix = inputPtr->GetPixelContainer()->Size();
  long nvoxels = szVol[2] * stride_image[2];
  unsigned int ncomp = static_cast<unsigned int>(nintpix/nvoxels);
  stride_image *= ncomp;

  // Determine the strides for the pixel step and line step
  int sPixel = (m_PixelTraverseForward ? 1 : -1) *
    stride_image[m_PixelDirectionImageAxis];
  int sLine = (m_LineTraverseForward ? 1 : -1) *
    stride_image[m_LineDirectionImageAxis];

  // We never take full line-strides, because as we iterate, we
  // take n pixel-strides before needing to worry about changing
  // the line. Therefore, we compute the step needed to go to the
  // start of next line after taking n pixel-strides
  int sRowOfPixels = sPixel * szVol[m_PixelDirectionImageAxis];
  int sLineDelta = sLine - sRowOfPixels;

  // Determine the first voxel that we will traverse
  Vector3i xStartVoxel;
  xStartVoxel[m_PixelDirectionImageAxis] =
    m_PixelTraverseForward ? 0 : szVol[m_PixelDirectionImageAxis] - 1;
  xStartVoxel[m_LineDirectionImageAxis] =
    m_LineTraverseForward ? 0 : szVol[m_LineDirectionImageAxis] - 1;
  xStartVoxel[m_SliceDirectionImageAxis] =
    szVol[m_SliceDirectionImageAxis] == 1 ? 0 : m_SliceIndex;

  // Get the offset of the first voxel
  size_t iStart = dot_product(stride_image, xStartVoxel);

  // Get the size of the output region (whole slice)
  typename OutputImageType::RegionType rgn = outputPtr->GetBufferedRegion();
  size_t nPixel = rgn.GetSize()[0], nLine = rgn.GetSize()[1];

  // Get pointers to input and output data
  const InputComponentType *pSource = inputPtr->GetBufferPointer();

  // Set up the output iterator
  typedef itk::ImageLinearIteratorWithIndex<OutputImageType> OutIterType;
  OutIterType it_out(outputPtr, outputPtr->GetBufferedRegion());

  // Get the pixel accessor functor - for unified access to voxels
  typedef typename InputImageType::AccessorFunctorType AccessorFunctorType;
  typedef typename InputImageType::AccessorType AccessorType;
  typedef typename InputImageType::OffsetValueType OffsetType;
  AccessorType accessor = inputPtr->GetPixelAccessor();
  AccessorFunctorType accessor_functor;
  accessor_functor.SetPixelAccessor(accessor);
  accessor_functor.SetBegin(pSource);

  // Position the source at the first component of the first voxel to traverse
  // OffsetType offset =
  const InputComponentType *pBegin = pSource;
  pSource += iStart;

  // Main loop: copy data from source to target
  while(!it_out.IsAtEnd())
    {
    while( !it_out.IsAtEndOfLine() )
      {
      // Use the accessor
      accessor_functor.SetBegin(pSource);
      OutputPixelType val = accessor_functor.Get(*pSource);

      // Set the pixel
      it_out.Set(val);

      // Go to next pixel
      ++it_out;
      pSource += sPixel;
      }
    it_out.NextLine();
    pSource += sLineDelta;
    }

}
*/

template <class TInputImage, class TOutputImage>
void IRISSlicer<TInputImage, TOutputImage>
::GenerateData()
{
  // Here's the input and output
  const InputImageType *inputPtr = this->GetInput();
  OutputImageType *outputPtr = this->GetOutput();

  // Decide if we want to use the preview input instead
  const InputImageType *preview =
      (InputImageType *) this->GetInputs()[1].GetPointer();

  if(preview)
    if(m_BypassMainInput || preview->GetMTime() > inputPtr->GetMTime())
      inputPtr = preview;
  
  // Allocate (why is this necessary?)
  this->AllocateOutputs();

  // Get the image dimensions
  typename InputImageType::SizeType szVol = inputPtr->GetBufferedRegion().GetSize();

  // Set the strides in image coordinates
  Vector3i stride_image(1, szVol[0], szVol[1] * szVol[0]);

  // Scale the strides by the number of components. We can't get it from the
  // image itself since the image may be an ImageAdaptor, so we need to use
  // the pixel container's size
  long nintpix = inputPtr->GetPixelContainer()->Size();
  long nvoxels = szVol[2] * stride_image[2];
  unsigned int ncomp = static_cast<unsigned int>(nintpix/nvoxels);
  stride_image *= ncomp;

  // Determine the strides for the pixel step and line step
  int sPixel = (m_PixelTraverseForward ? 1 : -1) *
    stride_image[m_PixelDirectionImageAxis];
  int sLine = (m_LineTraverseForward ? 1 : -1) *
    stride_image[m_LineDirectionImageAxis];
  
  // We never take full line-strides, because as we iterate, we
  // take n pixel-strides before needing to worry about changing
  // the line. Therefore, we compute the step needed to go to the
  // start of next line after taking n pixel-strides
  int sRowOfPixels = sPixel * szVol[m_PixelDirectionImageAxis];
  int sLineDelta = sLine - sRowOfPixels;

  // Determine the first voxel that we will traverse
  Vector3i xStartVoxel;
  xStartVoxel[m_PixelDirectionImageAxis] = 
    m_PixelTraverseForward ? 0 : szVol[m_PixelDirectionImageAxis] - 1;
  xStartVoxel[m_LineDirectionImageAxis] = 
    m_LineTraverseForward ? 0 : szVol[m_LineDirectionImageAxis] - 1;
  xStartVoxel[m_SliceDirectionImageAxis] = 
    szVol[m_SliceDirectionImageAxis] == 1 ? 0 : m_SliceIndex;

  // Get the offset of the first voxel. As pointed out by Roman Grothausmann, the VNL
  // dot product causes overflow so we compute directly.
  size_t iStart = 0;
  for(int i = 0; i < 3; i++)
    iStart += static_cast<long>(stride_image[i]) * static_cast<long>(xStartVoxel[i]);

  // Get the size of the output region (whole slice)
  typename OutputImageType::RegionType rgn = outputPtr->GetBufferedRegion();
  size_t nPixel = rgn.GetSize()[0], nLine = rgn.GetSize()[1];

  // Get pointers to input and output data
  const InputComponentType *pSource = inputPtr->GetBufferPointer();

  // Set up the output iterator
  typedef itk::ImageLinearIteratorWithIndex<OutputImageType> OutIterType;
  OutIterType it_out(outputPtr, outputPtr->GetBufferedRegion());

  // Get the pixel accessor functor - for unified access to voxels
  typedef typename InputImageType::AccessorFunctorType AccessorFunctorType;
  typedef typename InputImageType::AccessorType AccessorType;
  typedef typename InputImageType::OffsetValueType OffsetType;
  AccessorType accessor = inputPtr->GetPixelAccessor();
  AccessorFunctorType accessor_functor;
  accessor_functor.SetPixelAccessor(accessor);
  accessor_functor.SetBegin(pSource);

  // Position the source at the first component of the first voxel to traverse
  // OffsetType offset =
  const InputComponentType *pBegin = pSource;
  pSource += iStart;

  // Main loop: copy data from source to target
  while(!it_out.IsAtEnd())
    {
    while( !it_out.IsAtEndOfLine() )
      {
      // Use the accessor
      accessor_functor.SetBegin(pSource);
      OutputPixelType val = accessor_functor.Get(*pSource);

      // Set the pixel
      it_out.Set(val);

      // Go to next pixel
      ++it_out;
      pSource += sPixel;
      }
    it_out.NextLine();
    pSource += sLineDelta;
    }
}

template <class TInputImage, class TOutputImage>
void IRISSlicer<TInputImage, TOutputImage>
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

template <class TInputImage, class TOutputImage>
void IRISSlicer<TInputImage, TOutputImage>
::SetPreviewInput(InputImageType *input)
{
  this->SetNthInput(1, input);
}

template <class TInputImage, class TOutputImage>
typename IRISSlicer<TInputImage, TOutputImage>::InputImageType *
IRISSlicer<TInputImage, TOutputImage>
::GetPreviewInput()
{
  return const_cast<InputImageType *>(this->GetInput(1));
}

/*
template <class TInputImage, class TOutputImage>
void
IRISSlicer<TInputImage, TOutputImage>
::BeforeThreadedGenerateData()
{

}

template <class TInputImage, class TOutputImage>
void
IRISSlicer<TInputImage, TOutputImage>
::AfterThreadedGenerateData()
{

}
*/
