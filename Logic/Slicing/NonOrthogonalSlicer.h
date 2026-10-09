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
#include "NonOrthogonalSlicerWorkerTraits.h"

using itk::DataObjectDecorator;
using itk::ProcessObject;

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

  virtual void DynamicThreadedGenerateData(const OutputImageRegionType& outputRegionForThread) override;

  virtual void VerifyInputInformation() const override { }

  virtual void GenerateOutputInformation() override;

  virtual void GenerateInputRequestedRegion() override;

  bool m_UseNearestNeighbor;
};

#ifndef ITK_MANUAL_INSTANTIATION
#include "NonOrthogonalSlicer.txx"
#endif

#endif // NONORTHOGONALSLICER_H
