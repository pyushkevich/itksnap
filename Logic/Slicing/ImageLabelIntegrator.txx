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
#ifndef IMAGELABELINTEGRATOR_TXX
#define IMAGELABELINTEGRATOR_TXX

#include "ImageLabelIntegrator.h"
#include "FastLinearInterpolator.h"
#include "ImageRegionConstIteratorWithIndexOverride.h"

template <typename TInputImage, typename TOutputImage, typename TWorkerTraits>
NonOrthogonalImageLabelIntegrator<TInputImage, TOutputImage, TWorkerTraits>::NonOrthogonalImageLabelIntegrator()
{}

template <typename TInputImage, typename TOutputImage, typename TWorkerTraits>
NonOrthogonalImageLabelIntegrator<TInputImage, TOutputImage, TWorkerTraits>::~NonOrthogonalImageLabelIntegrator()
{
  if (m_Worker)
    delete m_Worker;
}

template <typename TInputImage, typename TOutputImage, typename TWorkerTraits>
void
NonOrthogonalImageLabelIntegrator<TInputImage, TOutputImage, TWorkerTraits>::Initialize()
{
  // The input image volume
  InputImageType *input = const_cast<InputImageType *>(this->GetInputImage());

  // Create the worker
  if (m_Worker)
    delete m_Worker;
  m_Worker = new TWorkerTraits(input);
}

template <typename TInputImage, typename TOutputImage, typename TWorkerTraits>
void
NonOrthogonalImageLabelIntegrator<TInputImage, TOutputImage, TWorkerTraits>::GetRunLengthIntensityStatistics(
  const itk::Index<3>       &startIdx,
  long                       runlength,
  double                    &out_nvalid,
  double                    &out_sum,
  double                    &out_sumsq)
{
  // Get the 3D index of the second pixel of the line
  typename ReferenceImageBaseType::IndexType idxNext = startIdx;
  idxNext[0] += 1;

  // Convert to a physical point relative to refernece image
  typename ReferenceImageBaseType::PointType pRefStart, pRefNext;
  const ReferenceImageBaseType *reference = this->GetReferenceImage();
  reference->TransformIndexToPhysicalPoint(startIdx, pRefStart);
  reference->TransformIndexToPhysicalPoint(idxNext, pRefNext);

  // Apply the affine transform to this point - so it's relative to the input image
  typename ReferenceImageBaseType::PointType pInpStart, pInpNext;
  const TransformType *transform = this->GetTransform();
  pInpStart = transform->TransformPoint(pRefStart);
  pInpNext = transform->TransformPoint(pRefNext);

  // Compute the sample point in input image space and the step
  itk::ContinuousIndex<double, InputImageDimension> cixSample, cixNext, cixStep;
  InputImageType *input = const_cast<InputImageType *>(this->GetInputImage());
  input->TransformPhysicalPointToContinuousIndex(pInpStart, cixSample);
  input->TransformPhysicalPointToContinuousIndex(pInpNext, cixNext);
  for(int d = 0; d < InputImageDimension; d++)
    cixStep[d] = cixNext[d] - cixSample[d];

  for(long q = 0; q < runlength; q++)
  {
    m_Worker->CollectStatistics(cixSample.GetDataPointer(), m_UseNearestNeighbor, out_nvalid, out_sum, out_sumsq);

    // Update the sample location
    for (int d = 0; d < InputImageDimension; d++)
      cixSample[d] += cixStep[d];
  }
}

#endif
