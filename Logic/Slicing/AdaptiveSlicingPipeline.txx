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
#include "IRISSlicer.h"
#include "NonOrthogonalSlicer.h"
#include "IRISVectorTypesToITKConversion.h"

template<class TInputImage> class AdaptiveSlicingPipeline_PixelFiller
{
public:
  typedef TInputImage ImageType;
  typedef typename ImageType::PixelType PixelType;
  typedef typename ImageType::InternalPixelType ComponentType;
  static void FillPixel(const ImageType *image, PixelType &pixel, ComponentType value)
  {
    pixel = value;
  }
  static void MakePixel(const ImageType *image, PixelType &pixel, ComponentType *arr)
  {
    pixel = *arr;
  }

};

template<typename TPixel, unsigned int VDim>
class AdaptiveSlicingPipeline_PixelFiller< itk::VectorImage<TPixel, VDim> >
{
public:
  typedef itk::VectorImage<TPixel, VDim> ImageType;
  typedef typename ImageType::PixelType PixelType;
  typedef typename ImageType::InternalPixelType ComponentType;
  static void FillPixel(const ImageType *image, PixelType &pixel, ComponentType value)
  {
    pixel.SetSize(image->GetNumberOfComponentsPerPixel());
    pixel.Fill(value);
  }
  static void MakePixel(const ImageType *image, PixelType &pixel, ComponentType *arr)
  {
    pixel.SetSize(image->GetNumberOfComponentsPerPixel());
    for(int i = 0; i < pixel.GetSize(); i++)
      pixel.SetElement(i, arr[i]);
  }
};


template<typename TInputImage, typename TOutputImage, typename TPreviewImage>
AdaptiveSlicingPipeline<TInputImage, TOutputImage, TPreviewImage>
::AdaptiveSlicingPipeline()
{
  // Create the two slicer types
  m_OrthogonalSlicer = OrthogonalSlicerType::New();
  m_ObliqueSlicer = NonOrthogonalSlicerType::New();

  // Initially use the ortho
  m_UseOrthogonalSlicing = true;
}

template<typename TInputImage, typename TOutputImage, typename TPreviewImage>
AdaptiveSlicingPipeline<TInputImage, TOutputImage, TPreviewImage>
::~AdaptiveSlicingPipeline()
{
  // Prevent crash from grafting child filter outputs
  if(this->GetOutput())
    this->GetOutput()->SetPixelContainer(NULL);
}

template<typename TInputImage, typename TOutputImage, typename TPreviewImage>
void
AdaptiveSlicingPipeline<TInputImage, TOutputImage, TPreviewImage>
::MapInputsToSlicers()
{
  if(m_UseOrthogonalSlicing)
    {
    m_OrthogonalSlicer->SetInput(this->GetInput());
    m_OrthogonalSlicer->SetPreviewInput(
          const_cast<PreviewImageType *>(this->GetPreviewImage()));

    // Inverse transform
    ImageCoordinateTransform::Pointer tinv = ImageCoordinateTransform::New();
    this->GetOrthogonalTransform()->ComputeInverse(tinv);

    // Tell slicer in which directions to slice
    m_OrthogonalSlicer->SetSliceDirectionImageAxis(
          tinv->GetCoordinateIndexZeroBased(2));

    m_OrthogonalSlicer->SetLineDirectionImageAxis(
          tinv->GetCoordinateIndexZeroBased(1));

    m_OrthogonalSlicer->SetPixelDirectionImageAxis(
          tinv->GetCoordinateIndexZeroBased(0));

    m_OrthogonalSlicer->SetPixelTraverseForward(
          tinv->GetCoordinateOrientation(0) > 0);

    m_OrthogonalSlicer->SetLineTraverseForward(
          tinv->GetCoordinateOrientation(1) > 0);

    // Set the slice index
    m_OrthogonalSlicer->SetSliceIndex(
          m_SliceIndex[m_OrthogonalSlicer->GetSliceDirectionImageAxis()]);
    }
  else
    {
    m_ObliqueSlicer->SetInput(this->GetInput());
    m_ObliqueSlicer->SetTransform(this->GetObliqueTransform());
    m_ObliqueSlicer->SetReferenceImage(this->GetObliqueReferenceImage());
    }
}

template<typename TInputImage, typename TOutputImage, typename TPreviewImage>
void
AdaptiveSlicingPipeline<TInputImage, TOutputImage, TPreviewImage>
::GenerateOutputInformation()
{
  // Make sure the inputs are assigned to the corresponding slicers
  this->MapInputsToSlicers();

  // Get the outer filter's output
  OutputImageType *output = this->GetOutput();

  // Use appropriate sub-pipeline
  if(m_UseOrthogonalSlicing)
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

template<typename TInputImage, typename TOutputImage, typename TPreviewImage>
void
AdaptiveSlicingPipeline<TInputImage, TOutputImage, TPreviewImage>
::PropagateRequestedRegion(itk::DataObject *output)
{
  // Use appropriate sub-pipeline
  if(m_UseOrthogonalSlicing)
    {
    m_OrthogonalSlicer->PropagateRequestedRegion(output);
    }
  else
    {
    m_ObliqueSlicer->PropagateRequestedRegion(output);
    }
}

template<typename TInputImage, typename TOutputImage, typename TPreviewImage>
void
AdaptiveSlicingPipeline<TInputImage, TOutputImage, TPreviewImage>
::CallCopyOutputRegionToInputRegion(
    InputImageRegionType &destRegion, const OutputImageRegionType &srcRegion)
{
  Superclass::CallCopyOutputRegionToInputRegion(destRegion, srcRegion);
}


template<typename TInputImage, typename TOutputImage, typename TPreviewImage>
void
AdaptiveSlicingPipeline<TInputImage, TOutputImage, TPreviewImage>
::GenerateData()
{
  // Get the outer filter's output
  OutputImageType *output = this->GetOutput();

  // Use appropriate sub-pipeline
  if(m_UseOrthogonalSlicing)
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

template<typename TInputImage, typename TOutputImage, typename TPreviewImage>
typename AdaptiveSlicingPipeline<TInputImage, TOutputImage, TPreviewImage>::OutputPixelType
AdaptiveSlicingPipeline<TInputImage, TOutputImage, TPreviewImage>
::LookupIntensityAtSliceIndex(const itk::ImageBase<3> *ref_space)
{
  return this->LookupIntensityAtReferenceIndex(ref_space, m_SliceIndex);
}

template<typename TInputImage, typename TOutputImage, typename TPreviewImage>
typename AdaptiveSlicingPipeline<TInputImage, TOutputImage, TPreviewImage>::OutputPixelType
AdaptiveSlicingPipeline<TInputImage, TOutputImage, TPreviewImage>
::LookupIntensityAtReferenceIndex(const itk::ImageBase<3> *ref_space, const IndexType &index)
{
  // Update the filter
  this->Update();

  // The lookup location
  Vector3ui cursor(index);

  if(m_UseOrthogonalSlicing)
    {
    // If we are using ortho slicing, we can just sample the slice
    Vector3ui slice_3d = this->GetOrthogonalTransform()->TransformVoxelIndex(cursor);
    itk::Index<2> slice_idx; slice_idx[0] = slice_3d[0]; slice_idx[1] = slice_3d[1];
    return this->GetOutput()->GetPixel(slice_idx);
    }
  else
    {
    // The cursor may be outside of the slice, so we need to map the location back
    // to the input image and look up the intensity of the input image.

    // Use the reference space to map the cursor coordinate to physical coordinate
    itk::Point<double, 3> cursor_point, native_point;
    ref_space->TransformIndexToPhysicalPoint(to_itkIndex(cursor), cursor_point);

    // Use the transform to map the coordinate into the native space of the input
    native_point = this->GetObliqueTransform()->TransformPoint(cursor_point);

    // Map the native point to an index
    itk::ContinuousIndex<double, 3> native_cindex;
    this->GetInput()->TransformPhysicalPointToContinuousIndex(native_point, native_cindex);

    // Create a pointer to pixel data
    unsigned int k = this->GetOutput()->GetNumberOfComponentsPerPixel();
    OutputComponentType *out_arr = new OutputComponentType[k], *dummy = out_arr;

    // Use worker class to interpolate input image - out_arr will be filled
    typedef typename NonOrthogonalSlicerType::WorkerType WorkerType;
    WorkerType worker(const_cast<InputImageType *>(this->GetInput()));
    worker.ProcessVoxel(native_cindex.GetDataPointer(), false, &dummy);

    // Create a pixel to return - we use a specialized class for vector/non-vector
    OutputPixelType pix;
    AdaptiveSlicingPipeline_PixelFiller<OutputImageType>
        ::MakePixel(this->GetOutput(),pix, out_arr);

    delete[] out_arr;
    return pix;
    }
}




#endif // ADAPTIVESLICINGPIPELINE_TXX
