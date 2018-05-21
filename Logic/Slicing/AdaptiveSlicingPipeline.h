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
#ifndef ADAPTIVESLICINGPIPELINE_H
#define ADAPTIVESLICINGPIPELINE_H

#include "itkImageToImageFilter.h"
#include "itkTransform.h"
#include "itkDataObjectDecorator.h"
#include "SNAPCommon.h"

template <class TInputImage, class TOutputImage, class TPreviewImage> class IRISSlicer;
template <class TInputImage, class TOutputImage> class NonOrthogonalSlicer;
class ImageCoordinateTransform;

using itk::DataObjectDecorator;
using itk::ProcessObject;

namespace itk {
template<typename TParametersValueType,
          unsigned int NInputDimensions,
          unsigned int NOutputDimensions> class Transform;
}


/**
 * This filter encapsulates the ITK-SNAP slicing pipeline. It includes both
 * the straight (orthogonal) slicer and the oblique slicer.
 *
 *
 */
template <typename TInputImage, typename TOutputImage, typename TPreviewImage>
class AdaptiveSlicingPipeline
    : public itk::ImageToImageFilter<TInputImage, TOutputImage>
{
public:
  /** Standard class typedefs. */
  typedef AdaptiveSlicingPipeline                                        Self;
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

  typedef TPreviewImage                                      PreviewImageType;
  typedef typename PreviewImageType::Pointer              PreviewImagePointer;
  typedef typename PreviewImageType::PixelType               PreviewPixelType;
  typedef typename PreviewImageType::InternalPixelType   PreviewComponentType;

  itkStaticConstMacro(ImageDimension, unsigned int, TOutputImage::ImageDimension);
  itkStaticConstMacro(InputImageDimension, unsigned int, TInputImage::ImageDimension);

  /** Slicers */
  typedef IRISSlicer<TInputImage,TOutputImage,TPreviewImage> OrthogonalSlicerType;
  typedef NonOrthogonalSlicer<TInputImage,TOutputImage>   NonOrthogonalSlicerType;

  /** Reference space for non-orthogonal slicing */
  typedef typename itk::ImageBase<InputImageDimension> NonOrthogonalSliceReferenceSpace;

  /** Transform */
  typedef ImageCoordinateTransform                     OrthogonalTransformType;
  typedef itk::Transform<double, InputImageDimension, InputImageDimension> ObliqueTransformType;

  /** Some more typedefs. */
  typedef typename InputImageType::RegionType             InputImageRegionType;
  typedef typename OutputImageType::RegionType           OutputImageRegionType;

  /** Slice index */
  typedef itk::Index<InputImageDimension>                            IndexType;

  /** Method for creation through the object factory. */
  itkNewMacro(Self)

  /** Run-time type information (and related methods). */
  itkTypeMacro(AdaptiveSlicingPipeline, ImageToImageFilter)

  /** Reference image input */
  itkSetInputMacro(ObliqueReferenceImage, NonOrthogonalSliceReferenceSpace)
  itkGetInputMacro(ObliqueReferenceImage, NonOrthogonalSliceReferenceSpace)

  /** Preview image input */
  itkSetInputMacro(PreviewImage, PreviewImageType)
  itkGetInputMacro(PreviewImage, PreviewImageType)

  /** Orthogonal Transform input */
  itkSetDecoratedObjectInputMacro(OrthogonalTransform, OrthogonalTransformType)
  itkGetDecoratedObjectInputMacro(OrthogonalTransform, OrthogonalTransformType)

  /** Oblique Transform input */
  itkSetDecoratedObjectInputMacro(ObliqueTransform, ObliqueTransformType)
  itkGetDecoratedObjectInputMacro(ObliqueTransform, ObliqueTransformType)

  /** Which slicing pipeline to use */
  itkSetMacro(UseOrthogonalSlicing, bool)
  itkGetMacro(UseOrthogonalSlicing, bool)

  /** Set the slice index for the orthogonal slicer */
  itkGetMacro(SliceIndex, IndexType)
  itkSetMacro(SliceIndex, IndexType)

  /** Interpolation type */
  void SetUseNearestNeighbor(bool flag);
  bool GetUseNearestNeighbor() const;

  /** Look up intensity at the current slice index. This may update the filter */
  OutputPixelType LookupIntensityAtSliceIndex(const itk::ImageBase<3> *ref_space);

  /** Loop up intensity at an arbitrary slice index in reference space */
  OutputPixelType LookupIntensityAtReferenceIndex(const itk::ImageBase<3> *ref_space, const IndexType &index);

protected:

  AdaptiveSlicingPipeline();
  ~AdaptiveSlicingPipeline();

  virtual void VerifyInputInformation() ITK_OVERRIDE { }

  virtual void GenerateOutputInformation() ITK_OVERRIDE;

  virtual void PropagateRequestedRegion(itk::DataObject *object) ITK_OVERRIDE;

  virtual void CallCopyOutputRegionToInputRegion(
      InputImageRegionType &destRegion,
      const OutputImageRegionType &srcRegion) ITK_OVERRIDE;

  virtual void GenerateData() ITK_OVERRIDE;

  itk::SmartPointer<OrthogonalSlicerType> m_OrthogonalSlicer;
  itk::SmartPointer<NonOrthogonalSlicerType> m_ObliqueSlicer;

  bool m_UseOrthogonalSlicing;

  IndexType m_SliceIndex;

  void MapInputsToSlicers();
};


#ifndef ITK_MANUAL_INSTANTIATION
#include "AdaptiveSlicingPipeline.txx"
#endif


#endif // ADAPTIVESLICINGPIPELINE_H
