/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    $RCSfile: VectorImageWrapper.txx,v $
  Language:  C++
  Date:      $Date: 2007/06/06 22:27:21 $
  Version:   $Revision: 1.1 $
  Copyright (c) 2003 Insight Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notices for more information.
=========================================================================*/

#include "VectorImageWrapper.h"
#include "itkImageRegionIterator.h"
#include "itkImageSliceConstIteratorWithIndex.h"
#include "itkNumericTraits.h"
#include "itkRegionOfInterestImageFilter.h"
#include "itkRescaleIntensityImageFilter.h"
#include "itkIdentityTransform.h"
#include "IRISSlicer.h"
#include "SNAPSegmentationROISettings.h"
#include "itkCommand.h"
#include "ImageWrapperTraits.h"
#include "itkVectorImageToImageAdaptor.h"

#include <iostream>


template <class TTraits, class TBase>
VectorImageWrapper<TTraits,TBase>
::VectorImageWrapper()
{
  // Initialize the derived wrapper array
  m_DefaultScalarRepType = VectorImageWrapperBase::SCALAR_REP_COMPONENT;
  m_DefaultScalarRepIndex = 0;
}


template <class TTraits, class TBase>
typename VectorImageWrapper<TTraits,TBase>::ImagePointer
VectorImageWrapper<TTraits,TBase>
::DeepCopyRegion(const SNAPSegmentationROISettings &roi,
                 itk::Command *progressCommand) const
{
   std::cout << "VectorImageWrapper::DeepCopyRegion" << std::endl;
   std::cout << std::flush;
   return NULL;
}

template <class TTraits, class TBase>
void
VectorImageWrapper<TTraits,TBase>
::SetNativeMapping(NativeIntensityMapping mapping)
{
  Superclass::SetNativeMapping(mapping);

  // Propagate to owned scalar wrappers
  for(ScalarRepIterator it = m_ScalarReps.begin(); it != m_ScalarReps.end(); ++it)
    {
    AbstractNativeIntensityMapping *abstract_mapping =
        const_cast<AbstractNativeIntensityMapping *>(
          it->second->GetNativeIntensityMapping());

    NativeIntensityMapping *real_mapping =
        dynamic_cast<NativeIntensityMapping *>(abstract_mapping);

    (*real_mapping) = mapping;
    }
}

template <class TTraits, class TBase>
template <class TFunctor>
SmartPtr<ScalarImageWrapperBase>
VectorImageWrapper<TTraits,TBase>
::CreateDerivedWrapper(ImageType *image, ColorMap *refmap)
{
  typedef VectorDerivedQuantityImageWrapperTraits<TFunctor> WrapperTraits;
  typedef typename WrapperTraits::WrapperType DerivedWrapper;
  typedef typename DerivedWrapper::ImageType AdaptorType;

  SmartPtr<AdaptorType> adaptor = AdaptorType::New();
  adaptor->SetImage(image);

  SmartPtr<DerivedWrapper> wrapper = DerivedWrapper::New();
  wrapper->InitializeToWrapper(this, adaptor);
  wrapper->GetDisplayMapping()->SetColorMap(refmap);

  SmartPtr<ScalarImageWrapperBase> ptrout = wrapper.GetPointer();

  return ptrout;
}

template <class TTraits, class TBase>
void
VectorImageWrapper<TTraits,TBase>
::UpdateImagePointer(ImageType *newImage)
{
  // Create the component wrappers
  int nc = newImage->GetNumberOfComponentsPerPixel();

  // The first component image will serve as the reference for the other
  // component images
  ComponentWrapperType *cref = NULL;

  for(int i = 0; i < nc; i++)
    {
    // Create a component image
    typedef itk::VectorImageToImageAdaptor<InternalPixelType,3> ComponentImage;
    SmartPtr<ComponentImage> comp = ComponentImage::New();
    comp->SetImage(newImage);
    comp->SetExtractComponentIndex(i);

    // Create a wrapper for this image and assign the component image
    SmartPtr<ComponentWrapperType> cw = ComponentWrapperType::New();
    cw->InitializeToWrapper(this, comp);

    // Make sure intensity curve is shared by the components
    // TODO: what should be shared is the entire pipeline. That requires us to
    // compute the min/max of the vector components and return them as an
    // itk::DataObject.
    if(i == 0)
      {
      cref = cw;
      }
    else
      {
      typedef typename ComponentWrapperType::DisplayMapping ComponentDM;
      SmartPtr<ComponentDM> cdm = cw->GetDisplayMapping();
      SmartPtr<ComponentDM> cdmref = cref->GetDisplayMapping();
      cdm->SetIntensityCurve(cdmref->GetIntensityCurve());
      cdm->SetColorMap(cdmref->GetColorMap());
      }

    // Store the component
    m_ScalarReps[std::make_pair(VectorImageWrapperBase::SCALAR_REP_COMPONENT, i)]
        = cw.GetPointer();
    }

  // Initialize the computed derived wrappers
  ColorMap *cm = cref->GetDisplayMapping()->GetColorMap();

  m_ScalarReps[std::make_pair(VectorImageWrapperBase::SCALAR_REP_MAGNITUDE, 0)]
      = this->template CreateDerivedWrapper<MagnitudeFunctor>(newImage, cm);

  m_ScalarReps[std::make_pair(VectorImageWrapperBase::SCALAR_REP_MAX, 0)]
      = this->template CreateDerivedWrapper<MaxFunctor>(newImage, cm);

  m_ScalarReps[std::make_pair(VectorImageWrapperBase::SCALAR_REP_AVERAGE, 0)]
      = this->template CreateDerivedWrapper<MeanFunctor>(newImage, cm);

  // Call the parent
  Superclass::UpdateImagePointer(newImage);
}

template <class TTraits, class TBase>
inline ScalarImageWrapperBase *
VectorImageWrapper<TTraits,TBase>
::GetScalarRepresentation(
    VectorImageWrapperBase::ScalarRepresentation type,
    int index)
{
  return m_ScalarReps[std::make_pair(type, index)];
}

template <class TTraits, class TBase>
typename VectorImageWrapper<TTraits,TBase>::ComponentWrapperType *
VectorImageWrapper<TTraits,TBase>
::GetComponentWrapper(unsigned int index)
{
  ScalarRepIndex repidx(VectorImageWrapperBase::SCALAR_REP_COMPONENT, index);
  return static_cast<ComponentWrapperType *>(m_ScalarReps[repidx].GetPointer());
}

template <class TTraits, class TBase>
void
VectorImageWrapper<TTraits,TBase>
::SetSliceIndex(const Vector3ui &cursor)
{
  Superclass::SetSliceIndex(cursor);

  // Propagate to owned scalar wrappers
  for(ScalarRepIterator it = m_ScalarReps.begin(); it != m_ScalarReps.end(); ++it)
    {
    it->second->SetSliceIndex(cursor);
    }
}

template <class TTraits, class TBase>
void
VectorImageWrapper<TTraits,TBase>
::SetImageToDisplayTransform(
    unsigned int iSlice, const ImageCoordinateTransform &transform)
{
  Superclass::SetImageToDisplayTransform(iSlice, transform);

  // Propagate to owned scalar wrappers
  for(ScalarRepIterator it = m_ScalarReps.begin(); it != m_ScalarReps.end(); ++it)
    {
    it->second->SetImageToDisplayTransform(iSlice, transform);
    }
}



/*
template <class TImage, class TBase>
inline double
VectorImageWrapper<TImage,TBase>
::GetVoxelAsDouble(const itk::Index<3> &idx) const
{
  // By default, return the first component
  return (double) this->GetVoxel(idx)[0];
}

template <class TImage, class TBase>
inline double
VectorImageWrapper<TImage,TBase>
::GetVoxelAsDouble(const Vector3ui &x) const
{
  // By default, return the first component
  return (double) this->GetVoxel(x)[0];
}
*/

template class VectorImageWrapper<AnatomicImageWrapperTraits<GreyType> >;

