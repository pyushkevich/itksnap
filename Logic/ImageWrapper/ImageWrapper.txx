/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    $RCSfile: ImageWrapper.txx,v $
  Language:  C++
  Date:      $Date: 2006/12/02 04:22:12 $
  Version:   $Revision: 1.1 $
  Copyright (c) 2003 Insight Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notices for more information.
=========================================================================*/
#ifndef __ImageWrapper_txx_
#define __ImageWrapper_txx_

#include "itkImageRegionIterator.h"
#include "itkImageSliceConstIteratorWithIndex.h"
#include "itkNumericTraits.h"
#include "itkRegionOfInterestImageFilter.h"
#include "itkRescaleIntensityImageFilter.h"
#include "itkIdentityTransform.h"
#include "itkResampleImageFilter.h"
#include "itkNearestNeighborInterpolateImageFunction.h"
#include "itkBSplineInterpolateImageFunction.h"
#include "itkWindowedSincInterpolateImageFunction.h"
#include "itkLinearInterpolateImageFunction.h"
#include "IRISSlicer.h"
#include "SNAPSegmentationROISettings.h"
#include "itkCommand.h"

#include <iostream>

template <class TPixel> 
ImageWrapper<TPixel>
::ImageWrapper() 
{
  CommonInitialization();
}

template <class TPixel>
ImageWrapper<TPixel>
::~ImageWrapper() 
{
  Reset();
}

template <class TPixel>
void 
ImageWrapper<TPixel>
::CommonInitialization()
{
  // Set initial state    
  m_Initialized = false;

  // Create slicer objects
  m_Slicer[0] = SlicerType::New();
  m_Slicer[1] = SlicerType::New();
  m_Slicer[2] = SlicerType::New();

  // Set the transform to identity, which will initialize the directions of the
  // slicers
  this->SetImageToDisplayTransformsToDefault();
}

template <class TPixel>
ImageWrapper<TPixel>
::ImageWrapper(const ImageWrapper<TPixel> &copy) 
{
  CommonInitialization();

  // If the source contains an image, make a copy of that image
  if (copy.IsInitialized() && copy.GetImage())
    {
    // Create and allocate the image
    ImagePointer newImage = ImageType::New();
    newImage->SetRegions(copy.GetImage()->GetBufferedRegion());
    newImage->Allocate();

    // Copy the image contents
    TPixel *ptrTarget = newImage->GetBufferPointer();
    TPixel *ptrSource = copy.GetImage()->GetBufferPointer();
    memcpy(ptrTarget,ptrSource,
           sizeof(TPixel) * newImage->GetBufferedRegion().GetNumberOfPixels());
    
    UpdateImagePointer(newImage);
    }
}

template <class TPixel> 
void 
ImageWrapper<TPixel>
::PrintDebugInformation() 
{
  std::cout << "=== Image Properties ===" << std::endl;
  std::cout << "   Dimensions         : " << m_Image->GetLargestPossibleRegion().GetSize() << std::endl;
  std::cout << "   Origin             : " << m_Image->GetOrigin() << std::endl;
  std::cout << "   Spacing            : " << m_Image->GetSpacing() << std::endl;
  std::cout << "------------------------" << std::endl;
}

template <class TPixel>
void 
ImageWrapper<TPixel>
::UpdateImagePointer(ImageType *newImage) 
{
  // Check if the image size has changed
  bool hasSizeChanged = 
    (!m_Image) || 
    (newImage->GetLargestPossibleRegion().GetSize() !=
     m_Image->GetLargestPossibleRegion().GetSize());
  
  // Change the input of the slicers 
  m_Slicer[0]->SetInput(newImage);
  m_Slicer[1]->SetInput(newImage);
  m_Slicer[2]->SetInput(newImage);
    
  // If so, the coordinate transform needs to be reinitialized to identity
  if(hasSizeChanged)
    {
    // Reset the transform to identity
    this->SetImageToDisplayTransformsToDefault();

    // Reset the slice positions to zero
    this->SetSliceIndex(Vector3ui(0,0,0));
    }
  
  // Update the max-min pipeline once we have one setup
  m_MinMaxCalc = MinMaxCalculatorType::New();
  m_MinMaxCalc->SetImage(newImage);

  // Update the image
  m_Image = newImage;

  // Mark the image as Modified to enforce correct sequence of 
  // operations with MinMaxCalc
  m_Image->Modified();

  // We have been initialized
  m_Initialized = true;
}

template <class TPixel>
void 
ImageWrapper<TPixel>
::InitializeToWrapper(const ImageWrapperBase *source, ImageType *image) 
{
  // Call the common update method
  UpdateImagePointer(image);

  // Update the image-display transforms
  for(unsigned int d=0;d<3;d++)
    SetImageToDisplayTransform(d,source->GetImageToDisplayTransform(d));

  // Update the slice index
  SetSliceIndex(source->GetSliceIndex());
}

template <class TPixel>
void 
ImageWrapper<TPixel>
::InitializeToWrapper(const ImageWrapperBase *source, const TPixel &value) 
{
  // Allocate the image
  ImagePointer newImage = ImageType::New();
  newImage->SetRegions(source->GetImageBase()->GetBufferedRegion().GetSize());
  newImage->Allocate();
  newImage->FillBuffer(value);
  newImage->SetOrigin(source->GetImageBase()->GetOrigin());
  newImage->SetSpacing(source->GetImageBase()->GetSpacing());

  // Call the common update method
  UpdateImagePointer(newImage);

  // Update the image-display transforms
  for(unsigned int d=0;d<3;d++)
    SetImageToDisplayTransform(d,source->GetImageToDisplayTransform(d));

  // Update the slice index
  SetSliceIndex(source->GetSliceIndex());
}

template <class TPixel>
void 
ImageWrapper<TPixel>
::SetImage(ImagePointer newImage) 
{
  UpdateImagePointer(newImage);
}


template <class TPixel>
void 
ImageWrapper<TPixel>
::Reset() 
{
  if (m_Initialized)
    m_Image->ReleaseData();
  m_Initialized = false;
}

template <class TPixel>
bool 
ImageWrapper<TPixel>
::IsInitialized() const 
{
  return m_Initialized;
}

template <class TPixel>
typename ImageWrapper<TPixel>::ImagePointer
ImageWrapper<TPixel>
::DeepCopyRegion(const SNAPSegmentationROISettings &roi,
                 itk::Command *progressCommand) const
{
  // The filter used to chop off the region of interest
  typedef itk::RegionOfInterestImageFilter <ImageType,ImageType> ChopFilterType;
  typename ChopFilterType::Pointer fltChop = ChopFilterType::New();

  // Check if there is a difference in voxel size, i.e., user wants resampling
  Vector3ul vOldSize(m_Image->GetLargestPossibleRegion().GetSize().GetSize());
  Vector3d vOldSpacing(m_Image->GetSpacing().GetDataPointer());
  
  if(roi.GetResampleFlag())
    {
    // Compute the number of voxels in the output
    typedef typename itk::ImageRegion<3> RegionType;
    typedef typename itk::Size<3> SizeType;

    SizeType vNewSize;
    RegionType vNewROI;
    Vector3d vNewSpacing;

    for(unsigned int i = 0; i < 3; i++) 
      {
      double scale = roi.GetVoxelScale()[i];
      vNewSize.SetElement(i, (unsigned long) (vOldSize[i] / scale));
      vNewROI.SetSize(i,(unsigned long) (roi.GetROI().GetSize(i) / scale));
      vNewROI.SetIndex(i,(long) (roi.GetROI().GetIndex(i) / scale));
      vNewSpacing[i] = scale * vOldSpacing[i];
      }

    // Create a filter for resampling the image
    typedef itk::ResampleImageFilter<ImageType,ImageType> ResampleFilterType;
    typename ResampleFilterType::Pointer fltSample = ResampleFilterType::New();

    // Initialize the resampling filter
    fltSample->SetInput(m_Image);
    fltSample->SetTransform(itk::IdentityTransform<double,3>::New());

    // Typedefs for interpolators
    typedef itk::NearestNeighborInterpolateImageFunction<
      ImageType,double> NNInterpolatorType;
    typedef itk::LinearInterpolateImageFunction<
      ImageType,double> LinearInterpolatorType;
    typedef itk::BSplineInterpolateImageFunction<
      ImageType,double> CubicInterpolatorType;

    // More typedefs are needed for the sinc interpolator
    static const unsigned int VRadius = 5;
    typedef itk::Function::HammingWindowFunction<VRadius> WindowFunction;
    typedef itk::ConstantBoundaryCondition<ImageType> Condition;
    typedef itk::WindowedSincInterpolateImageFunction<
      ImageType, VRadius, 
      WindowFunction, Condition, double> SincInterpolatorType;

    // Choose the interpolator
    switch(roi.GetInterpolationMethod())
      {
      case SNAPSegmentationROISettings::NEAREST_NEIGHBOR :
        fltSample->SetInterpolator(NNInterpolatorType::New());
        break;

      case SNAPSegmentationROISettings::TRILINEAR : 
        fltSample->SetInterpolator(LinearInterpolatorType::New());
        break;

      case SNAPSegmentationROISettings::TRICUBIC :
        fltSample->SetInterpolator(CubicInterpolatorType::New());
        break;  

      case SNAPSegmentationROISettings::SINC_WINDOW_05 :
        fltSample->SetInterpolator(SincInterpolatorType::New());
        break;  
      };

    // Set the image sizes and spacing
    fltSample->SetSize(vNewSize);
    fltSample->SetOutputSpacing(vNewSpacing.data_block());
    fltSample->SetOutputOrigin(m_Image->GetOrigin());

    // Set the progress bar
    if(progressCommand)
      fltSample->AddObserver(itk::AnyEvent(),progressCommand);

  // Perform resampling
    fltSample->GetOutput()->SetRequestedRegion(vNewROI);
  fltSample->Update();  

    // Pipe into the chopper
    fltChop->SetInput(fltSample->GetOutput());

    // Update the region of interest
    fltChop->SetRegionOfInterest(vNewROI);
    }
  else
    {
    // Pipe image into the chopper
    fltChop->SetInput(m_Image);    
    
    // Set the region of interest
    fltChop->SetRegionOfInterest(roi.GetROI());
    }

  // Update the pipeline
  fltChop->Update();

  // Return the resulting image
  return fltChop->GetOutput();
}

template <class TPixel>
inline const TPixel& 
ImageWrapper<TPixel>
::GetVoxel(const Vector3ui &index) const 
{
  return GetVoxel(index[0],index[1],index[2]);
}

template <class TPixel>
inline TPixel& 
ImageWrapper<TPixel>
::GetVoxelForUpdate(const Vector3ui &index) 
{
  return GetVoxelForUpdate(index[0],index[1],index[2]);
}

template <class TPixel>
inline TPixel& 
ImageWrapper<TPixel>
::GetVoxelForUpdate(unsigned int x, unsigned int y, unsigned int z) 
{
  itk::Index<3> index;
  index[0] = x;
  index[1] = y;
  index[2] = z;

  // Verify that the pixel is contained by the image at debug time
  assert(m_Image && m_Image->GetLargestPossibleRegion().IsInside(index));

  // Return the pixel
  return m_Image->GetPixel(index);
}

template <class TPixel>
inline const TPixel& 
ImageWrapper<TPixel>
::GetVoxel(unsigned int x, unsigned int y, unsigned int z) const
{
  itk::Index<3> index;
  index[0] = x;
  index[1] = y;
  index[2] = z;

  // Verify that the pixel is contained by the image at debug time
  assert(m_Image && m_Image->GetLargestPossibleRegion().IsInside(index));

  // Return the pixel
  return m_Image->GetPixel(index);
}


template <class TPixel> 
void 
ImageWrapper<TPixel>
::CheckImageIntensityRange() 
{
  // Image should be loaded
  assert(m_Image && m_MinMaxCalc);

  // Check if the image has been updated since the last time that
  // the min/max has been computed
  if (m_Image->GetMTime() > m_MinMaxCalc->GetMTime())
    {
    m_MinMaxCalc->Compute();
    m_MinMaxCalc->Modified();
    m_ImageScaleFactor = 1.0 / (m_MinMaxCalc->GetMaximum() - m_MinMaxCalc->GetMinimum());
    }
}

template <class TPixel> 
TPixel 
ImageWrapper<TPixel>
::GetImageMin() 
{
  // Make sure min/max are up-to-date
  CheckImageIntensityRange();

  // Return the max or min
  return m_MinMaxCalc->GetMinimum();
}

template <class TPixel>
double 
ImageWrapper<TPixel>
::GetImageScaleFactor()
{
  // Make sure min/max are up-to-date
  CheckImageIntensityRange();

  // Return the max or min
  return m_ImageScaleFactor;    
}

template <class TPixel> 
TPixel 
ImageWrapper<TPixel>
::GetImageMax()
{
  // Make sure min/max are up-to-date
  CheckImageIntensityRange();

  // Return the max or min
  return m_MinMaxCalc->GetMaximum();
}

template <class TPixel> 
typename ImageWrapper<TPixel>::ConstIterator 
ImageWrapper<TPixel>
::GetImageConstIterator() const 
{
  ConstIterator it(m_Image,m_Image->GetLargestPossibleRegion());
  it.GoToBegin();
  return it;
}

template <class TPixel> 
typename ImageWrapper<TPixel>::Iterator 
ImageWrapper<TPixel>
::GetImageIterator() 
{
  Iterator it(m_Image,m_Image->GetLargestPossibleRegion());
  it.GoToBegin();
  return it;
}

template <class TPixel>    
void 
ImageWrapper<TPixel>
::RemapIntensityToRange(TPixel min, TPixel max)
{
  typedef itk::RescaleIntensityImageFilter<ImageType> FilterType;
  typedef typename FilterType::Pointer FilterPointer;

  // Create a filter to remap the intensities
  FilterPointer filter = FilterType::New();
  filter->SetInput(m_Image);
  filter->SetOutputMinimum(min);
  filter->SetOutputMaximum(max);

  // Run the filter
  filter->Update();

  // Store the output and point everything to it
  UpdateImagePointer(filter->GetOutput());
}

template <class TPixel>    
void 
ImageWrapper<TPixel>
::RemapIntensityToMaximumRange()
{
  RemapIntensityToRange(itk::NumericTraits<TPixel>::min(),
                        itk::NumericTraits<TPixel>::max());
}

template <class TPixel>    
Vector3ui
ImageWrapper<TPixel>
::GetSliceIndex() const
{
  return m_SliceIndex;
}

template <class TPixel>    
void 
ImageWrapper<TPixel>
::SetSliceIndex(const Vector3ui &cursor)
{
  // Save the cursor position
  m_SliceIndex = cursor;

  // Select the appropriate slice for each slicer
  for(unsigned int i=0;i<3;i++)
  {
    // Which axis does this slicer slice?
    unsigned int axis = m_Slicer[i]->GetSliceDirectionImageAxis();

    // Set the slice using that axis
    m_Slicer[i]->SetSliceIndex(cursor[axis]);
  }
}

template <class TPixel>    
void 
ImageWrapper<TPixel>
::SetImageToDisplayTransformsToDefault()
{
  ImageCoordinateTransform id[3];
  id[0].SetTransform(Vector3i(1,2,3),Vector3ui(0,0,0));
  id[1].SetTransform(Vector3i(1,3,2),Vector3ui(0,0,0));
  id[2].SetTransform(Vector3i(2,3,1),Vector3ui(0,0,0));
  SetImageToDisplayTransform(0,id[0]);
  SetImageToDisplayTransform(1,id[1]);
  SetImageToDisplayTransform(2,id[2]);
}

template <class TPixel>    
const ImageCoordinateTransform&
ImageWrapper<TPixel>
::GetImageToDisplayTransform(unsigned int iSlice) const 
{
  return m_ImageToDisplayTransform[iSlice];
}

template <class TPixel>    
void 
ImageWrapper<TPixel>
::SetImageToDisplayTransform(unsigned int iSlice,
                             const ImageCoordinateTransform &transform)
{
  // Get the transform and its inverse
  m_ImageToDisplayTransform[iSlice] = transform;
  m_DisplayToImageTransform[iSlice] = transform.Inverse();

  // Tell slicer in which directions to slice
  m_Slicer[iSlice]->SetSliceDirectionImageAxis(
    m_DisplayToImageTransform[iSlice].GetCoordinateIndexZeroBased(2));
  
  m_Slicer[iSlice]->SetLineDirectionImageAxis(
    m_DisplayToImageTransform[iSlice].GetCoordinateIndexZeroBased(1));

  m_Slicer[iSlice]->SetPixelDirectionImageAxis(
    m_DisplayToImageTransform[iSlice].GetCoordinateIndexZeroBased(0));

  m_Slicer[iSlice]->SetPixelTraverseForward(
    m_DisplayToImageTransform[iSlice].GetCoordinateOrientation(0) > 0);

  m_Slicer[iSlice]->SetLineTraverseForward(
    m_DisplayToImageTransform[iSlice].GetCoordinateOrientation(1) > 0);
}


  /** For each slicer, find out which image dimension does is slice along */

template <class TPixel>    
unsigned int 
ImageWrapper<TPixel>
::GetDisplaySliceImageAxis(unsigned int iSlice)
{
  return m_Slicer[iSlice]->GetSliceDirectionImageAxis();
}

template <class TPixel>
typename ImageWrapper<TPixel>::SliceType*
ImageWrapper<TPixel>
::GetSlice(unsigned int dimension)
{
  return m_Slicer[dimension]->GetOutput();
}

template <class TPixel>
const TPixel *
ImageWrapper<TPixel>
::GetVoxelPointer() const
{
  return m_Image->GetBufferPointer();
}

template <class TPixel>
Vector3ui
ImageWrapper<TPixel>
::GetSize() const
{
  // Cast the size to our vector format
  itk::Size<3> size = m_Image->GetLargestPossibleRegion().GetSize();      
  return Vector3ui(
    (unsigned int) size[0],
    (unsigned int) size[1],
    (unsigned int) size[2]);
}

template<class TPixel>
Vector3d
ImageWrapper<TPixel>
::TransformVoxelIndexToPosition(const Vector3ui &iVoxel) const
{
  // Use the ITK method to do this
  typename ImageType::IndexType xIndex;
  for(size_t d = 0; d < 3; d++) xIndex[d] = iVoxel[d];
  
  itk::Point<double, 3> xPoint;
  m_Image->TransformIndexToPhysicalPoint(xIndex, xPoint);

  Vector3d xOut;
  for(size_t q = 0; q < 3; q++) xOut[q] = xPoint[q];

  return xOut;
}

template <class TPixel>
unsigned int 
ImageWrapper<TPixel>
::ReplaceIntensity(TPixel iOld, TPixel iNew)
{
  // Counter for the number of replaced voxels
  unsigned int nReplaced = 0;

  // Replace the voxels
  for(Iterator it = GetImageIterator(); !it.IsAtEnd(); ++it)
    if(it.Value() == iOld)
      {
      it.Set(iNew);
      ++nReplaced;
      }

  // Flag that changes have been made
  if(nReplaced > 0)
    m_Image->Modified();

  // Return the number of replacements
  return nReplaced;
}

template <class TPixel>
unsigned int 
ImageWrapper<TPixel>
::SwapIntensities(TPixel iFirst, TPixel iSecond)
{
  // Counter for the number of replaced voxels
  unsigned int nReplaced = 0;

  // Replace the voxels
  for(Iterator it = GetImageIterator(); !it.IsAtEnd(); ++it)
    if(it.Value() == iFirst)
      {
      it.Set(iSecond);
      ++nReplaced;
      }
    else if(it.Value() == iSecond)
      {
      it.Set(iFirst);
      ++nReplaced;
      }

  // Flag that changes have been made
  if(nReplaced > 0)
    m_Image->Modified();

  // Return the number of replacements
  return nReplaced;
}
  

#endif // __ImageWrapper_txx_
