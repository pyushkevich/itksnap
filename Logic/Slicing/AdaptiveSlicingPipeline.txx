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
#ifndef ADAPTIVESLICINGPIPELINE_TXX
#define ADAPTIVESLICINGPIPELINE_TXX

#include "AdaptiveSlicingPipeline.h"
#include "IRISVectorTypesToITKConversion.h"

template <typename TInputImage, typename TOutputImage, typename TPreviewImage>
AdaptiveSlicingPipeline<TInputImage, TOutputImage, TPreviewImage>::AdaptiveSlicingPipeline()
{
  // Create the two slicer types
  m_OrthogonalSlicer = OrthogonalSlicerType::New();
  m_ObliqueSlicer = NonOrthogonalSlicerType::New();

  // Initially use the ortho
  m_UseOrthogonalSlicing = true;
}

template <typename TInputImage, typename TOutputImage, typename TPreviewImage>
AdaptiveSlicingPipeline<TInputImage, TOutputImage, TPreviewImage>::~AdaptiveSlicingPipeline()
{
  // Prevent crash from grafting child filter outputs
  if (this->GetOutput())
    this->GetOutput()->SetPixelContainer(NULL);
}

template <typename TInputImage, typename TOutputImage, typename TPreviewImage>
bool
AdaptiveSlicingPipeline<TInputImage, TOutputImage, TPreviewImage>::GetUseNearestNeighbor() const
{
  return m_ObliqueSlicer->GetUseNearestNeighbor();
}

template <typename TInputImage, typename TOutputImage, typename TPreviewImage>
void
AdaptiveSlicingPipeline<TInputImage, TOutputImage, TPreviewImage>::SetUseNearestNeighbor(bool flag)
{
  return m_ObliqueSlicer->SetUseNearestNeighbor(flag);
}

template <typename TInputImage, typename TOutputImage, typename TPreviewImage>
void
AdaptiveSlicingPipeline<TInputImage, TOutputImage, TPreviewImage>::MapInputsToSlicers()
{
  if (m_UseOrthogonalSlicing)
  {
    m_OrthogonalSlicer->SetInput(this->GetInput());
    m_OrthogonalSlicer->SetPreviewInput(const_cast<PreviewImageType *>(this->GetPreviewImage()));

    // Inverse transform
    ImageCoordinateTransform::Pointer tinv = ImageCoordinateTransform::New();
    this->GetOrthogonalTransform()->ComputeInverse(tinv);

    // Tell slicer in which directions to slice
    m_OrthogonalSlicer->SetSliceDirectionImageAxis(tinv->GetCoordinateIndexZeroBased(2));

    m_OrthogonalSlicer->SetLineDirectionImageAxis(tinv->GetCoordinateIndexZeroBased(1));

    m_OrthogonalSlicer->SetPixelDirectionImageAxis(tinv->GetCoordinateIndexZeroBased(0));

    m_OrthogonalSlicer->SetPixelTraverseForward(tinv->GetCoordinateOrientation(0) > 0);

    m_OrthogonalSlicer->SetLineTraverseForward(tinv->GetCoordinateOrientation(1) > 0);

    // Set the slice index
    m_OrthogonalSlicer->SetSliceIndex(m_SliceIndex[m_OrthogonalSlicer->GetSliceDirectionImageAxis()]);
  }
  else
  {
    m_ObliqueSlicer->SetInput(this->GetInput());
    m_ObliqueSlicer->SetTransform(this->GetObliqueTransform());
    m_ObliqueSlicer->SetReferenceImage(m_ObliqueReferenceImage);
  }
}

template <typename TInputImage, typename TOutputImage, typename TPreviewImage>
void
AdaptiveSlicingPipeline<TInputImage, TOutputImage, TPreviewImage>::SetSliceIndex(const IndexType &index)
{
  // Handle modified carefully, since slice index only affects us if slicing is
  // orthogonal and if the direction of slicing is changed
  if (m_UseOrthogonalSlicing)
  {
    auto axis = m_OrthogonalSlicer->GetSliceDirectionImageAxis();
    if (index[axis] != m_SliceIndex[axis])
      this->Modified();
  }
  m_SliceIndex = index;
}


template <typename TInputImage, typename TOutputImage, typename TPreviewImage>
void
AdaptiveSlicingPipeline<TInputImage, TOutputImage, TPreviewImage>::GenerateOutputInformation()
{
  // Make sure the inputs are assigned to the corresponding slicers
  this->MapInputsToSlicers();

  // Get the outer filter's output
  OutputImageType *output = this->GetOutput();

  // Use appropriate sub-pipeline
  if (m_UseOrthogonalSlicing)
  {
    m_OrthogonalSlicer->UpdateOutputInformation();
    m_OrthogonalSlicer->GetOutput()->SetRequestedRegionToLargestPossibleRegion();
    output->CopyInformation(m_OrthogonalSlicer->GetOutput());
  }
  else
  {
    m_ObliqueSlicer->UpdateOutputInformation();
    m_ObliqueSlicer->GetOutput()->SetRequestedRegionToLargestPossibleRegion();
    output->CopyInformation(m_ObliqueSlicer->GetOutput());
  }

  // Copy information does not update the requested region, so we must update
  // it by hand here
  output->SetRequestedRegionToLargestPossibleRegion();
}

template <typename TInputImage, typename TOutputImage, typename TPreviewImage>
void
AdaptiveSlicingPipeline<TInputImage, TOutputImage, TPreviewImage>::PropagateRequestedRegion(
  itk::DataObject *output)
{
  // Use appropriate sub-pipeline
  if (m_UseOrthogonalSlicing)
  {
    m_OrthogonalSlicer->PropagateRequestedRegion(output);
  }
  else
  {
    m_ObliqueSlicer->PropagateRequestedRegion(output);
  }
}

template <typename TInputImage, typename TOutputImage, typename TPreviewImage>
void
AdaptiveSlicingPipeline<TInputImage, TOutputImage, TPreviewImage>::CallCopyOutputRegionToInputRegion(
  InputImageRegionType        &destRegion,
  const OutputImageRegionType &srcRegion)
{
  Superclass::CallCopyOutputRegionToInputRegion(destRegion, srcRegion);
}


template <typename TInputImage, typename TOutputImage, typename TPreviewImage>
void
AdaptiveSlicingPipeline<TInputImage, TOutputImage, TPreviewImage>::GenerateData()
{
  // Get the outer filter's output
  OutputImageType *output = this->GetOutput();

  // Use appropriate sub-pipeline
  if (m_UseOrthogonalSlicing)
  {
    m_OrthogonalSlicer->Update();
    output->Graft(m_OrthogonalSlicer->GetOutput());
  }
  else
  {
    m_ObliqueSlicer->Update();
    output->Graft(m_ObliqueSlicer->GetOutput());
  }
}


template <typename TInputImage, typename TOutputImage, typename TPreviewImage>
void
AdaptiveSlicingPipeline<TInputImage, TOutputImage, TPreviewImage>::SetUseOrthogonalSlicing(bool value)
{
  if (m_UseOrthogonalSlicing != value)
  {
    m_UseOrthogonalSlicing = value;
    if (m_UseOrthogonalSlicing)
    {
      this->RemoveInput("ObliqueReferenceImage");
    }
    else
    {
      this->SetInput(
        "ObliqueReferenceImage",
        const_cast<NonOrthogonalSliceReferenceSpace *>(m_ObliqueReferenceImage.GetPointer()));
    }
    this->Modified();
  }
}

template <typename TInputImage, typename TOutputImage, typename TPreviewImage>
const typename AdaptiveSlicingPipeline<TInputImage, TOutputImage, TPreviewImage>::NonOrthogonalSliceReferenceSpace *
AdaptiveSlicingPipeline<TInputImage, TOutputImage, TPreviewImage>::GetObliqueReferenceImage() const
{
  return m_ObliqueReferenceImage;
}

template <typename TInputImage, typename TOutputImage, typename TPreviewImage>
void
AdaptiveSlicingPipeline<TInputImage, TOutputImage, TPreviewImage>::SetObliqueReferenceImage(
  const NonOrthogonalSliceReferenceSpace *input)
{
  m_ObliqueReferenceImage = input;
  if (!m_UseOrthogonalSlicing)
  {
    this->SetInput(
      "ObliqueReferenceImage",
      const_cast<NonOrthogonalSliceReferenceSpace *>(m_ObliqueReferenceImage.GetPointer()));
  }
}


#endif // ADAPTIVESLICINGPIPELINE_TXX
