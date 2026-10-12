/*=========================================================================

  Program:   ITK-SNAP
  Language:  C++
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

=========================================================================*/
#ifndef NONORTHOGONALSLICERWORKERTRAITS_H
#define NONORTHOGONALSLICERWORKERTRAITS_H

#include "itkVectorImage.h"
#include "itkImageAdaptor.h"

template <typename TPixel, unsigned int VDim, typename CounterType> class RLEImage;
template <typename TImage, typename TFloat, unsigned int VDim> class FastLinearInterpolator;

namespace itk
{
template <typename TPixelType, unsigned int Dimension> class VectorImageToImageAdaptor;
}

/**
 * This is a helper traits class that can be used to modify the access of pixel
 * data in the input image by the NonOrthogonalSlicer. It samples the input
 * image at arbitrary (continuous index) positions, either to fill a slice
 * (ProcessVoxel, SkipVoxels) or to accumulate intensity statistics
 * (CollectStatistics).
 */
template <typename TInputImage, typename TOutputImage>
class DefaultNonOrthogonalSlicerWorkerTraits
{
public:
  typedef typename TOutputImage::InternalPixelType OutputComponentType;

  DefaultNonOrthogonalSlicerWorkerTraits(TInputImage *image);
  ~DefaultNonOrthogonalSlicerWorkerTraits();

  inline void ProcessVoxel(double *cix, bool use_nn, OutputComponentType **out_ptr);
  inline void CollectStatistics(double *cix, bool use_nn, double &out_nvalid, double &out_sum, double &out_sumsq);
  inline void SkipVoxels(int n, OutputComponentType **out_ptr);

protected:

  // The interpolator object used internally
  typedef FastLinearInterpolator<TInputImage, double, TInputImage::ImageDimension> Interpolator;
  Interpolator m_Interpolator;

  // Number of components
  int m_NumComponents;

  // Temporary buffer
  double *m_Buffer;
};


/**
 * Partial template specialization of DefaultNonOrthogonalSlicerWorkerTraits
 * for itk::VectorImageToImageAdaptor
 */
template <typename TPixelType, unsigned int Dimension, typename TOutputImage>
class DefaultNonOrthogonalSlicerWorkerTraits<
    itk::VectorImageToImageAdaptor<TPixelType, Dimension>, TOutputImage>
{
public:
  // Dimensions of the individual volume
  typedef itk::VectorImageToImageAdaptor<TPixelType, Dimension> AdaptorType;
  typedef typename TOutputImage::InternalPixelType OutputComponentType;

  DefaultNonOrthogonalSlicerWorkerTraits(AdaptorType *adaptor);
  ~DefaultNonOrthogonalSlicerWorkerTraits();

  inline void ProcessVoxel(double *cix, bool use_nn, OutputComponentType **out_ptr);
  inline void CollectStatistics(double *cix, bool use_nn, double &out_nvalid, double &out_sum, double &out_sumsq);
  inline void SkipVoxels(int n, OutputComponentType **out_ptr);

protected:

  // The actual image type
  typedef typename AdaptorType::InternalImageType InternalImageType;

  // The interpolator object used internally
  typedef FastLinearInterpolator<InternalImageType, double, AdaptorType::ImageDimension> Interpolator;
  Interpolator m_Interpolator;

  // Number of components
  int m_NumComponents, m_ExtractComponent;

  // Temporary buffer
  double m_BufferValue;
};


/**
 * Partial template specialization of DefaultNonOrthogonalSlicerWorkerTraits
 * for generic image adapters wrapping a vector image
 */
template <typename TPixelType, unsigned int Dimension, typename TAccessor, typename TOutputImage>
class DefaultNonOrthogonalSlicerWorkerTraits<
    itk::ImageAdaptor<itk::VectorImage<TPixelType, Dimension>, TAccessor>, TOutputImage>
{
public:

  // Dimensions of the individual volume
  typedef itk::VectorImage<TPixelType, Dimension> InternalImageType;
  typedef itk::ImageAdaptor<InternalImageType, TAccessor> AdaptorType;
  typedef typename TOutputImage::InternalPixelType OutputComponentType;

  DefaultNonOrthogonalSlicerWorkerTraits(AdaptorType *adaptor);
  ~DefaultNonOrthogonalSlicerWorkerTraits();

  inline void ProcessVoxel(double *cix, bool use_nn, OutputComponentType **out_ptr);
  inline void CollectStatistics(double *cix, bool use_nn, double &out_nvalid, double &out_sum, double &out_sumsq);
  inline void SkipVoxels(int n, OutputComponentType **out_ptr);

protected:

  // The interpolator object used internally
  typedef FastLinearInterpolator<InternalImageType, double, AdaptorType::ImageDimension> Interpolator;
  Interpolator m_Interpolator;

  // Number of components
  int m_NumComponents;

  // Temporary buffer for interpolation
  double *m_Buffer;

  // Temporary buffer for computing the derived quantity
  typename InternalImageType::PixelType m_VectorPixel;

  // A pointer to the adaptor
  typename AdaptorType::Pointer m_Adaptor;
};


/**
 * Partial template specialization of DefaultNonOrthogonalSlicerWorkerTraits
 * for RLEImage.
 *
 * TODO: this should be updated to support nearest neighbor interpolation!
 */
template <typename TPixel, unsigned int Dimension, typename TCounter, typename TOutputImage>
class DefaultNonOrthogonalSlicerWorkerTraits<
    RLEImage<TPixel, Dimension, TCounter>, TOutputImage>
{
public:

  typedef RLEImage<TPixel, Dimension, TCounter> InputImageType;
  typedef typename TOutputImage::InternalPixelType OutputComponentType;

  DefaultNonOrthogonalSlicerWorkerTraits(InputImageType *adaptor);
  ~DefaultNonOrthogonalSlicerWorkerTraits();

  inline void ProcessVoxel(double *cix, bool use_nn, OutputComponentType **out_ptr);
  inline void CollectStatistics(double *cix, bool use_nn, double &out_nvalid, double &out_sum, double &out_sumsq);
  inline void SkipVoxels(int n, OutputComponentType **out_ptr);

protected:
  typename InputImageType::Pointer m_Image;
};

#ifndef ITK_MANUAL_INSTANTIATION
#include "NonOrthogonalSlicerWorkerTraits.txx"
#endif

#endif // NONORTHOGONALSLICERWORKERTRAITS_H
