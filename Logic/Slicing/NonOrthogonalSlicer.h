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

using itk::DataObjectDecorator;

template <typename TPixel, unsigned int VDim, typename CounterType> class RLEImage;

namespace itk
{
template <typename TImage, typename TAccessor> class ImageAdaptor;
template <typename TPixelType, unsigned int Dimension> class VectorImageToImageAdaptor;
}

/**
 * This is an alternative slicer that functions more or less like an ITK
 * ResampleImageFilter, but more efficiently and directly maps the data to
 * a 2D slice.
 *
 * The filter takes a transform and a reference image from which the slice is
 * generated.
 */
template <typename TInputImage, typename TOutputImage>
class NonOrthogonalSlicerBase
    : public itk::ImageToImageFilter<TInputImage, TOutputImage>
{
public:
  /** Standard class typedefs. */
  typedef NonOrthogonalSlicerBase                                        Self;
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
  itkTypeMacro(NonOrthogonalSlicerBase, ImageToImageFilter)

  itkStaticConstMacro(ImageDimension, unsigned int, TOutputImage::ImageDimension);
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

  NonOrthogonalSlicerBase();
  ~NonOrthogonalSlicerBase();

  /*
  virtual void ThreadedGenerateData(const OutputImageRegionType& outputRegionForThread,
                                    itk::ThreadIdType threadId);
*/

  virtual void VerifyInputInformation() { }

  virtual void GenerateOutputInformation();

  virtual void GenerateInputRequestedRegion();

private:

  bool m_UseNearestNeighbor;
};

/**
 * An actual implementation of the base class
 */
template <typename TInputImage, typename TOutputImage>
class NonOrthogonalSlicer
    : public NonOrthogonalSlicerBase<TInputImage, TOutputImage>
{
public:

  /** Standard class typedefs. */
  typedef NonOrthogonalSlicer                                            Self;
  typedef NonOrthogonalSlicerBase<TInputImage, TOutputImage>       Superclass;
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

  typedef typename Superclass::ReferenceImageBaseType ReferenceImageBaseType;
  typedef typename Superclass::TransformType TransformType;
  typedef typename Superclass::InputImageRegionType InputImageRegionType;
  typedef typename Superclass::OutputImageRegionType OutputImageRegionType;

  itkStaticConstMacro(ImageDimension, unsigned int, TOutputImage::ImageDimension);
  itkStaticConstMacro(InputImageDimension, unsigned int, TInputImage::ImageDimension);

  /** Method for creation through the object factory. */
  itkNewMacro(Self)

  /** Run-time type information (and related methods). */
  itkTypeMacro(NonOrthogonalSlicer, NonOrthogonalSlicerBase)

protected:
  NonOrthogonalSlicer() {}
  ~NonOrthogonalSlicer() {}

  virtual void ThreadedGenerateData(const OutputImageRegionType& outputRegionForThread,
                                    itk::ThreadIdType threadId);
};


/**
 * An specialization of the base class for RLE images
 */
template <typename TPixel, typename CounterType, typename TOutputImage>
class NonOrthogonalSlicer< RLEImage<TPixel, 3, CounterType>, TOutputImage>
    : public NonOrthogonalSlicerBase<RLEImage<TPixel, 3, CounterType>, TOutputImage>
{
public:

  typedef RLEImage<TPixel, 3, CounterType>                     InputImageType;

  /** Standard class typedefs. */
  typedef NonOrthogonalSlicer                                            Self;
  typedef NonOrthogonalSlicerBase<InputImageType, TOutputImage>    Superclass;
  typedef itk::SmartPointer<Self>                                     Pointer;
  typedef itk::SmartPointer<const Self>                          ConstPointer;

  typedef typename Superclass::OutputImageRegionType OutputImageRegionType;

  /** Method for creation through the object factory. */
  itkNewMacro(Self)

  /** Run-time type information (and related methods). */
  itkTypeMacro(NonOrthogonalSlicer, NonOrthogonalSlicerBase)

protected:
  NonOrthogonalSlicer() {}
  ~NonOrthogonalSlicer() {}

  virtual void ThreadedGenerateData(const OutputImageRegionType& outputRegionForThread,
                                    itk::ThreadIdType threadId)
  {
    // Do nothing - crash
    assert(0);
  }
};


template <typename TImage, typename TAccessor, typename TOutputImage>
class NonOrthogonalSlicer< itk::ImageAdaptor<TImage, TAccessor>, TOutputImage>
    : public NonOrthogonalSlicerBase<itk::ImageAdaptor<TImage, TAccessor>, TOutputImage>
{
public:

  typedef itk::ImageAdaptor<TImage, TAccessor>            InputImageType;

  /** Standard class typedefs. */
  typedef NonOrthogonalSlicer                                            Self;
  typedef NonOrthogonalSlicerBase<InputImageType, TOutputImage>    Superclass;
  typedef itk::SmartPointer<Self>                                     Pointer;
  typedef itk::SmartPointer<const Self>                          ConstPointer;

  typedef typename Superclass::OutputImageRegionType OutputImageRegionType;

  /** Method for creation through the object factory. */
  itkNewMacro(Self)

  /** Run-time type information (and related methods). */
  itkTypeMacro(NonOrthogonalSlicer, NonOrthogonalSlicerBase)

protected:
  NonOrthogonalSlicer() {}
  ~NonOrthogonalSlicer() {}

  virtual void ThreadedGenerateData(const OutputImageRegionType& outputRegionForThread,
                                    itk::ThreadIdType threadId)
  {
    // Do nothing - crash
    assert(0);
  }
};


template <typename TPixelType, unsigned int Dimension, typename TOutputImage>
class NonOrthogonalSlicer< itk::VectorImageToImageAdaptor<TPixelType, Dimension>, TOutputImage>
    : public NonOrthogonalSlicerBase<itk::VectorImageToImageAdaptor<TPixelType, Dimension>, TOutputImage>
{
public:

  typedef itk::VectorImageToImageAdaptor<TPixelType, Dimension>            InputImageType;

  /** Standard class typedefs. */
  typedef NonOrthogonalSlicer                                            Self;
  typedef NonOrthogonalSlicerBase<InputImageType, TOutputImage>    Superclass;
  typedef itk::SmartPointer<Self>                                     Pointer;
  typedef itk::SmartPointer<const Self>                          ConstPointer;

  typedef typename Superclass::OutputImageRegionType OutputImageRegionType;

  /** Method for creation through the object factory. */
  itkNewMacro(Self)

  /** Run-time type information (and related methods). */
  itkTypeMacro(NonOrthogonalSlicer, NonOrthogonalSlicerBase)

protected:
  NonOrthogonalSlicer() {}
  ~NonOrthogonalSlicer() {}

  virtual void ThreadedGenerateData(const OutputImageRegionType& outputRegionForThread,
                                    itk::ThreadIdType threadId)
  {
    // Do nothing - crash
    assert(0);
  }
};


#ifndef ITK_MANUAL_INSTANTIATION
#include "NonOrthogonalSlicer.txx"
#endif

#endif // NONORTHOGONALSLICER_H
