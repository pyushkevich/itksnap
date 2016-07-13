/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: ImageWrapper.txx,v $
  Language:  C++
  Date:      $Date: 2010/10/14 16:21:04 $
  Version:   $Revision: 1.11 $
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

  -----

  Copyright (c) 2003 Insight Software Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notices for more information.

=========================================================================*/
#ifndef NONORTHOGONALSLICER_TXX
#define NONORTHOGONALSLICER_TXX

#include "NonOrthogonalSlicer.h"
#include "FastLinearInterpolator.h"
#include "ImageRegionConstIteratorWithIndexOverride.h"

template <class TInputImage, class TOutputImage>
NonOrthogonalSlicer<TInputImage, TOutputImage>
::NonOrthogonalSlicer()
    : m_UseNearestNeighbor(false)
{
}

template <class TInputImage, class TOutputImage>
NonOrthogonalSlicer<TInputImage, TOutputImage>
::~NonOrthogonalSlicer()
{
}

template <class TInputImage, class TOutputImage>
void
NonOrthogonalSlicer<TInputImage, TOutputImage>
::GenerateOutputInformation()
{  
  // The output information can be left to defaults because it's in screen
  // space anyway and just does not matter for any geometry
  OutputImagePointer output = this->GetOutput();
  typename OutputImageType::SpacingType out_spacing;
  typename OutputImageType::DirectionType out_direction;
  typename OutputImageType::PointType out_origin;

  out_origin.Fill(0.0);
  out_spacing.Fill(1.0);
  out_direction.SetIdentity();

  // Set up the output region
  OutputImageRegionType out_region;
  for(int d = 0; d < ImageDimension; d++)
    {
    out_region.SetIndex(d, this->GetReferenceImage()->GetLargestPossibleRegion().GetIndex(d));
    out_region.SetSize(d, this->GetReferenceImage()->GetLargestPossibleRegion().GetSize(d));
    }

  output->SetSpacing(out_spacing);
  output->SetOrigin(out_origin);
  output->SetDirection(out_direction);
  output->SetLargestPossibleRegion(out_region);
  output->SetNumberOfComponentsPerPixel(this->GetInput()->GetNumberOfComponentsPerPixel());
}

template <class TInputImage, class TOutputImage>
void
NonOrthogonalSlicer<TInputImage, TOutputImage>
::GenerateInputRequestedRegion()
{
  // get pointers to the input and output
  InputImageType *inputPtr  =
    const_cast< InputImageType * >( this->GetInput() );

  // Request the entire input image
  if(inputPtr)
    inputPtr->SetRequestedRegionToLargestPossibleRegion();
}


template <class TInputImage, class TOutputImage>
void
NonOrthogonalSlicer<TInputImage, TOutputImage>
::ThreadedGenerateData(const OutputImageRegionType &outputRegionForThread,
                       itk::ThreadIdType threadId)
{
  // The input 3D image volume
  InputImageType *input = const_cast<InputImageType *>(this->GetInput());

  // The reference image
  const ReferenceImageBaseType *reference = this->GetReferenceImage();

  // The transform
  const TransformType *transform = this->GetTransform();

  // Length of the line along which we are going to be sampling
  int line_len = outputRegionForThread.GetSize(0);

  // Create an iterator over the output image
  typedef itk::ImageLinearIteratorWithIndex<OutputImageType> IterBase;
  typedef IteratorExtender<IterBase> IterType;

  // Determine the appropriate float/double type for the interpolator.
  typedef typename itk::NumericTraits<OutputComponentType>::MeasurementVectorType::ValueType FloatType;

  // Create a fast interpolator for the input image - via the traits, allowing for
  // partial specialization for imageadapters and other such things
  WorkerType worker(input);

  // Whether to use nn
  bool use_nn = this->GetUseNearestNeighbor();

  // Loop over the lines in the input image
  for(IterType it(this->GetOutput(), outputRegionForThread); !it.IsAtEnd(); it.NextLine())
    {
    // Get a pointer to the output pixels for this line
    OutputComponentType *outPixelPtr = it.GetPixelPointer(this->GetOutput());

    // Get the index of the first pixel - this is in 2D
    typename OutputImageType::IndexType outIndex = it.GetIndex();

    // Get the 3D index of the first pixel of the line
    typename ReferenceImageBaseType::IndexType idxStart;
    idxStart.Fill(0.0);
    for(int d = 0; d < ImageDimension; d++)
      idxStart[d] = outIndex[d];

    // Get the 3D index of the second pixel of the line
    typename ReferenceImageBaseType::IndexType idxNext = idxStart;
    idxNext[0] += 1;

    // Convert to a physical point relative to refernece image
    typename ReferenceImageBaseType::PointType pRefStart, pRefNext;
    reference->TransformIndexToPhysicalPoint(idxStart, pRefStart);
    reference->TransformIndexToPhysicalPoint(idxNext, pRefNext);

    // Apply the affine transform to this point - so it's relative to the input image
    typename ReferenceImageBaseType::PointType pInpStart, pInpNext;
    pInpStart = transform->TransformPoint(pRefStart);
    pInpNext = transform->TransformPoint(pRefNext);

    // Compute the sample point in input image space and the step
    itk::ContinuousIndex<double, InputImageDimension> cixSample, cixNext, cixStep;
    input->TransformPhysicalPointToContinuousIndex(pInpStart, cixSample);
    input->TransformPhysicalPointToContinuousIndex(pInpNext, cixNext);

    for(int d = 0; d < InputImageDimension; d++)
      cixStep[d] = cixNext[d] - cixSample[d];

    // TODO: figure out the range of locations in the line to interpolate
    // Iterate over the line
    for(int i = 0; i < line_len; i++)
      {
      // Delegate to the worker to actually process the voxel
      worker.ProcessVoxel(cixSample.GetDataPointer(), use_nn, &outPixelPtr);

      // Update the sample location
      for(int d = 0; d < InputImageDimension; d++)
        cixSample[d] += cixStep[d];
      }
    }
}

template <class TInputImage, class TOutputImage>
NonOrthogonalSlicerPixelAccessTraitsWorker<TInputImage, TOutputImage>
::NonOrthogonalSlicerPixelAccessTraitsWorker(TInputImage *image)
  : m_Interpolator(image)
{
  m_NumComponents = m_Interpolator.GetPointerIncrement();
  m_Buffer = new double[m_NumComponents];
}

template <class TInputImage, class TOutputImage>
NonOrthogonalSlicerPixelAccessTraitsWorker<TInputImage, TOutputImage>
::~NonOrthogonalSlicerPixelAccessTraitsWorker()
{
  delete m_Buffer;
}

template <class TInputImage, class TOutputImage>
void
NonOrthogonalSlicerPixelAccessTraitsWorker<TInputImage, TOutputImage>
::ProcessVoxel(double *cix, bool use_nn, OutputComponentType **out_ptr)
{
  // Perform the interpolation
  typename Interpolator::InOut status =
      use_nn
      ? m_Interpolator.InterpolateNearestNeighbor(cix,  m_Buffer)
      : m_Interpolator.Interpolate(cix, m_Buffer);

  if(status == Interpolator::INSIDE)
    {
    for(int k = 0; k < m_NumComponents; k++)
      *(*out_ptr)++ = static_cast<OutputComponentType>(m_Buffer[k]);
    }
  else
    {
    // TODO: this is problematic!!!!
    for(int k = 0; k < m_NumComponents; k++)
      *(*out_ptr)++ = 0; //itk::NumericTraits<OutputComponentType>::Zero;
    }
}


/*
 * Traits for the component extracting image adaptor. Note that in the call to the
 * constructor for the interpolator, we are passing the buffer pointer offset by
 * the component index, and only requesting a single component to be sampled
 */
template <typename TPixelType, unsigned int Dimension, typename TOutputImage>
NonOrthogonalSlicerPixelAccessTraitsWorker<itk::VectorImageToImageAdaptor<TPixelType, Dimension>, TOutputImage>
::NonOrthogonalSlicerPixelAccessTraitsWorker(AdaptorType *adaptor)
  : m_Interpolator(adaptor,
                   adaptor->GetBufferPointer() + adaptor->GetPixelAccessor().GetExtractComponentIdx(),
                   adaptor->GetPixelAccessor().GetVectorLength(), 1)
{
  m_NumComponents = m_Interpolator.GetPointerIncrement();
  m_ExtractComponent = adaptor->GetPixelAccessor().GetExtractComponentIdx();
}

template <typename TPixelType, unsigned int Dimension, typename TOutputImage>
NonOrthogonalSlicerPixelAccessTraitsWorker<itk::VectorImageToImageAdaptor<TPixelType, Dimension>, TOutputImage>
::~NonOrthogonalSlicerPixelAccessTraitsWorker()
{
}

template <typename TPixelType, unsigned int Dimension, typename TOutputImage>
void
NonOrthogonalSlicerPixelAccessTraitsWorker<itk::VectorImageToImageAdaptor<TPixelType, Dimension>, TOutputImage>
::ProcessVoxel(double *cix, bool use_nn, OutputComponentType **out_ptr)
{
  // Perform the interpolation
  typename Interpolator::InOut status =
      use_nn
      ? m_Interpolator.InterpolateNearestNeighbor(cix, &m_BufferValue)
      : m_Interpolator.Interpolate(cix, &m_BufferValue);

  if(status == Interpolator::INSIDE)
    {
    *(*out_ptr)++ = static_cast<OutputComponentType>(m_BufferValue);
    }
  else
    {
    *(*out_ptr)++ = 0;
    }
}


/*
 * Traits for a generic image adaptor with a vector image base
 */
template <typename TPixelType, unsigned int Dimension, typename TAccessor, typename TOutputImage>
NonOrthogonalSlicerPixelAccessTraitsWorker<
  itk::ImageAdaptor<itk::VectorImage<TPixelType, Dimension>, TAccessor>, TOutputImage>
::NonOrthogonalSlicerPixelAccessTraitsWorker(AdaptorType *adaptor)
  : m_Interpolator(adaptor, adaptor->GetBufferPointer(),
                    adaptor->GetPixelAccessor().GetVectorLength()),
    m_VectorPixel(adaptor->GetPixelAccessor().GetVectorLength()),
    m_Adaptor(adaptor)
{
  m_NumComponents = m_Interpolator.GetPointerIncrement();
  m_Buffer = new double[m_NumComponents];
}

template <typename TPixelType, unsigned int Dimension, typename TAccessor, typename TOutputImage>
NonOrthogonalSlicerPixelAccessTraitsWorker<
  itk::ImageAdaptor<itk::VectorImage<TPixelType, Dimension>, TAccessor>, TOutputImage>
::~NonOrthogonalSlicerPixelAccessTraitsWorker()
{
  delete m_Buffer;
}

template <typename TPixelType, unsigned int Dimension, typename TAccessor, typename TOutputImage>
void
NonOrthogonalSlicerPixelAccessTraitsWorker<
  itk::ImageAdaptor<itk::VectorImage<TPixelType, Dimension>, TAccessor>, TOutputImage>
::ProcessVoxel(double *cix, bool use_nn, OutputComponentType **out_ptr)
{
  // Perform the interpolation
  typename Interpolator::InOut status =
      use_nn
      ? m_Interpolator.InterpolateNearestNeighbor(cix, m_Buffer)
      : m_Interpolator.Interpolate(cix, m_Buffer);

  if(status == Interpolator::INSIDE)
    {
    // Cast to the vector pixel type
    for(int i = 0; i < m_NumComponents; i++)
      m_VectorPixel[i] = static_cast<OutputComponentType>(m_Buffer[i]);

    // Apply the accessor
    *(*out_ptr)++ = m_Adaptor->GetPixelAccessor().Get(m_VectorPixel);
    }
  else
    {
    *(*out_ptr)++ = 0;
    }
}



#endif // NONORTHOGONALSLICER_TXX
