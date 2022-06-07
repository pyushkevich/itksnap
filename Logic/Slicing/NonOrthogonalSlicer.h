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
#ifndef NONORTHOGONALSLICER_H
#define NONORTHOGONALSLICER_H

#include "itkImageToImageFilter.h"
#include "itkTransform.h"
#include "itkDataObjectDecorator.h"
#include "itkVectorImage.h"
#include "itkImageAdaptor.h"

using itk::DataObjectDecorator;
using itk::ProcessObject;

template <typename TPixel, unsigned int VDim, typename CounterType> class RLEImage;
template <typename TImage, typename TFloat, unsigned int VDim> class FastLinearInterpolator;

namespace itk
{
template <typename TPixelType, unsigned int Dimension> class VectorImageToImageAdaptor;
}

/**
 * This is a helper traits class that can be used to modify the access of pixel
 * data in the input image by the NonOrthogonalSlicer.
 */
template <typename TInputImage, typename TOutputImage>
class DefaultNonOrthogonalSlicerWorkerTraits
{
public:
  typedef typename TOutputImage::InternalPixelType OutputComponentType;

  DefaultNonOrthogonalSlicerWorkerTraits(TInputImage *image);
  ~DefaultNonOrthogonalSlicerWorkerTraits();

  inline void ProcessVoxel(double *cix, bool use_nn, OutputComponentType **out_ptr);

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

  inline void SkipVoxels(int n, OutputComponentType **out_ptr);

protected:
  typename InputImageType::Pointer m_Image;
};


/**
 * This is an alternative slicer that functions more or less like an ITK
 * ResampleImageFilter, but more efficiently and directly maps the data to
 * a 2D slice.
 *
 * The filter takes a transform and a reference image from which the slice is
 * generated.
 */
template <typename TInputImage, typename TOutputImage,
          typename TWorkerTraits = DefaultNonOrthogonalSlicerWorkerTraits<TInputImage, TOutputImage> >
class NonOrthogonalSlicer
    : public itk::ImageToImageFilter<TInputImage, TOutputImage>
{
public:
  /** Standard class typedefs. */
  typedef NonOrthogonalSlicer                                            Self;
  typedef itk::ImageToImageFilter<TInputImage, TOutputImage>       Superclass;
  typedef itk::SmartPointer<Self>                                     Pointer;
  typedef itk::SmartPointer<const Self>                          ConstPointer;

  typedef TInputImage                                          InputImageType;
  typedef typename InputImageType::ConstPointer             InputImagePointer;
  typedef typename InputImageType::PixelType                   InputPixelType;
  typedef typename InputImageType::InternalPixelType       InputComponentType;

  typedef TOutputImage                                        OutputImageType;
  typedef typename OutputImageType::Pointer                OutputImagePointer;
  typedef typename OutputImageType::PixelType                 OutputPixelType;
  typedef typename OutputImageType::InternalPixelType     OutputComponentType;

  /** Method for creation through the object factory. */
  itkNewMacro(Self)

  /** Run-time type information (and related methods). */
  itkTypeMacro(NonOrthogonalSlicer, ImageToImageFilter)

  itkStaticConstMacro(SliceDimension, unsigned int, TOutputImage::ImageDimension);
  itkStaticConstMacro(InputImageDimension, unsigned int, TInputImage::ImageDimension);

  typedef itk::ImageBase<InputImageDimension>          ReferenceImageBaseType;

  /** Transform */
  typedef itk::Transform<double, InputImageDimension, InputImageDimension> TransformType;

  /** Some more typedefs. */
  typedef typename InputImageType::RegionType             InputImageRegionType;
  typedef typename OutputImageType::RegionType           OutputImageRegionType;

  /** Reference image input */
  itkSetInputMacro(ReferenceImage, ReferenceImageBaseType)
  itkGetInputMacro(ReferenceImage, ReferenceImageBaseType)

  /** Transform input */
  itkSetDecoratedObjectInputMacro(Transform, TransformType)
  itkGetDecoratedObjectInputMacro(Transform, TransformType)

  /** Interpolation type */
  itkSetMacro(UseNearestNeighbor, bool)
  itkGetMacro(UseNearestNeighbor, bool)

protected:

  NonOrthogonalSlicer();
  ~NonOrthogonalSlicer();

  /** The traits class */

  virtual void DynamicThreadedGenerateData(const OutputImageRegionType& outputRegionForThread) ITK_OVERRIDE;

  virtual void VerifyInputInformation() const ITK_OVERRIDE { }

  virtual void GenerateOutputInformation() ITK_OVERRIDE;

  virtual void GenerateInputRequestedRegion() ITK_OVERRIDE;

  bool m_UseNearestNeighbor;
};









#ifndef ITK_MANUAL_INSTANTIATION
#include "NonOrthogonalSlicer.txx"
#endif

#endif // NONORTHOGONALSLICER_H
