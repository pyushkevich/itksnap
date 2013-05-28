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
#include "itkMinimumMaximumImageFilter.h"
#include "ScalarImageHistogram.h"
#include "Rebroadcaster.h"

#include <iostream>

#include "itkVectorGradientAnisotropicDiffusionImageFilter.h"


template <class TTraits, class TBase>
VectorImageWrapper<TTraits,TBase>
::VectorImageWrapper()
{
  // Initialize the flattened image
  m_FlatImage = NULL;

  // Initialize the filters
  m_MinMaxFilter = MinMaxFilterType::New();

}

template <class TTraits, class TBase>
VectorImageWrapper<TTraits,TBase>
::~VectorImageWrapper()
{
}



template <class TTraits, class TBase>
typename VectorImageWrapper<TTraits,TBase>::ImagePointer
VectorImageWrapper<TTraits,TBase>
::DeepCopyRegion(const SNAPSegmentationROISettings &roi,
                 itk::Command *progressCommand) const
{
   return Superclass::DeepCopyRegion(roi, progressCommand);
}

template <class TTraits, class TBase>
vnl_vector<double>
VectorImageWrapper<TTraits,TBase>
::GetVoxelUnderCursorDisplayedValue()
{
  // TODO: erase this
  for(ScalarRepIterator it = m_ScalarReps.begin(); it != m_ScalarReps.end(); ++it)
    {
    ScalarImageWrapperBase *s = it->second;
    ScalarRepIndex idx = it->first;
    }



  MultiChannelDisplayMode mode = this->m_DisplayMapping->GetDisplayMode();
  if(mode.UseRGB)
    {
    vnl_vector<double> v(3);
    this->GetVoxelMappedToNative(this->m_SliceIndex, v.data_block());
    return v;
    }
  else
    {
    ScalarImageWrapperBase *siw =
        this->GetScalarRepresentation(mode.SelectedScalarRep, mode.SelectedComponent);
    vnl_vector<double> v(1);
    v[0] = siw->GetVoxelMappedToNative(this->m_SliceIndex);
    return v;
    }
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
    ScalarRepIndex idx = it->first;
    if(idx.first == VectorImageWrapperBase::SCALAR_REP_COMPONENT)
      {
      // Get the mapping cast to proper type
      AbstractNativeIntensityMapping *abstract_mapping =
          const_cast<AbstractNativeIntensityMapping *>(
            it->second->GetNativeIntensityMapping());

      NativeIntensityMapping *real_mapping =
          dynamic_cast<NativeIntensityMapping *>(abstract_mapping);

      // Assign the native mapping to the component
      (*real_mapping) = mapping;
      }

    // These are the derived wrappers. They use the identity mapping, but they
    // need to know what the source native mapping is.
    else if(idx.first == VectorImageWrapperBase::SCALAR_REP_MAGNITUDE)
      {
      SetNativeMappingInDerivedWrapper<MagnitudeFunctor>(it->second, mapping);
      }
    else if(idx.first == VectorImageWrapperBase::SCALAR_REP_MAX)
      {
      SetNativeMappingInDerivedWrapper<MaxFunctor>(it->second, mapping);
      }
    else if(idx.first == VectorImageWrapperBase::SCALAR_REP_AVERAGE)
      {
      SetNativeMappingInDerivedWrapper<MeanFunctor>(it->second, mapping);
      }
    }
}

template <class TTraits, class TBase>
template <class TFunctor>
void
VectorImageWrapper<TTraits,TBase>
::SetNativeMappingInDerivedWrapper(
    ScalarImageWrapperBase *w,
    NativeIntensityMapping &mapping)
{
  typedef VectorDerivedQuantityImageWrapperTraits<TFunctor> WrapperTraits;
  typedef typename WrapperTraits::WrapperType DerivedWrapper;
  typedef typename DerivedWrapper::ImageType AdaptorType;
  typedef typename AdaptorType::AccessorType PixelAccessor;

  // Cast to the right type
  DerivedWrapper *dw = dynamic_cast<DerivedWrapper *>(w);

  // Get the accessor
  PixelAccessor &accessor = dw->GetImage()->GetPixelAccessor();
  accessor.SetSourceNativeMapping(mapping.GetScale(), mapping.GetShift());
}

template <class TTraits, class TBase>
template <class TFunctor>
SmartPtr<ScalarImageWrapperBase>
VectorImageWrapper<TTraits,TBase>
::CreateDerivedWrapper(ImageType *image)
{
  typedef VectorDerivedQuantityImageWrapperTraits<TFunctor> WrapperTraits;
  typedef typename WrapperTraits::WrapperType DerivedWrapper;
  typedef typename DerivedWrapper::ImageType AdaptorType;

  SmartPtr<AdaptorType> adaptor = AdaptorType::New();
  adaptor->SetImage(image);

  SmartPtr<DerivedWrapper> wrapper = DerivedWrapper::New();
  wrapper->InitializeToWrapper(this, adaptor);

  SmartPtr<ScalarImageWrapperBase> ptrout = wrapper.GetPointer();

  // When creating derived wrappers, we need to rebroadcast the events from
  // that wrapper as our own events
  Rebroadcaster::RebroadcastAsSourceEvent(wrapper, WrapperChangeEvent(), this);

  return ptrout;
}

template <class TTraits, class TBase>
void
VectorImageWrapper<TTraits,TBase>
::UpdateImagePointer(ImageType *newImage)
{
  // Create the component wrappers before calling the parent's method.
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

    // Store the wrapper
    m_ScalarReps[std::make_pair(
          VectorImageWrapperBase::SCALAR_REP_COMPONENT, i)] = cw.GetPointer();

    // Rebroadcast the events from that wrapper
    Rebroadcaster::RebroadcastAsSourceEvent(cw, WrapperChangeEvent(), this);
    }

  m_ScalarReps[std::make_pair(VectorImageWrapperBase::SCALAR_REP_MAGNITUDE, 0)]
      = this->template CreateDerivedWrapper<MagnitudeFunctor>(newImage);

  m_ScalarReps[std::make_pair(VectorImageWrapperBase::SCALAR_REP_MAX, 0)]
      = this->template CreateDerivedWrapper<MaxFunctor>(newImage);

  m_ScalarReps[std::make_pair(VectorImageWrapperBase::SCALAR_REP_AVERAGE, 0)]
      = this->template CreateDerivedWrapper<MeanFunctor>(newImage);

  // Create a flat representation of the image
  m_FlatImage = FlatImageType::New();
  typename FlatImageType::SizeType flatsize;
  flatsize[0] = newImage->GetPixelContainer()->Size();
  m_FlatImage->SetRegions(flatsize);
  m_FlatImage->SetPixelContainer(newImage->GetPixelContainer());

  // Connect the flat image to the min/max computer
  m_MinMaxFilter->SetInput(m_FlatImage);


  /*

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
  ColorMap *cm = cref->GetDisplayMapping()->GetColorMap(); */

  // Call the parent's method = this will initialize the display mapping
  Superclass::UpdateImagePointer(newImage);

}

template <class TTraits, class TBase>
inline ScalarImageWrapperBase *
VectorImageWrapper<TTraits,TBase>
::GetDefaultScalarRepresentation()
{
  ScalarImageWrapperBase *rep =
      this->m_DisplayMapping->GetScalarRepresentation();
  if(rep)
    return rep;

  // TODO: This is somewhat arbitrary! Maybe it should be something the user
  // can change under settings, i.e., "Default scalar representation for RGB images".
  return this->GetScalarRepresentation(VectorImageWrapperBase::SCALAR_REP_MAX);
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

template<class TTraits, class TBase>
typename VectorImageWrapper<TTraits,TBase>::ComponentTypeObject *
VectorImageWrapper<TTraits,TBase>
::GetImageMinObject() const
{
  return m_MinMaxFilter->GetMinimumOutput();
}

template<class TTraits, class TBase>
typename VectorImageWrapper<TTraits,TBase>::ComponentTypeObject *
VectorImageWrapper<TTraits,TBase>
::GetImageMaxObject() const
{
  return m_MinMaxFilter->GetMaximumOutput();
}


template<class TTraits, class TBase>
void
VectorImageWrapper<TTraits,TBase>
::AddSamplesToHistogram()
{
  typedef itk::ImageRegionConstIterator<FlatImageType> FlatIterator;
  FlatIterator it(m_FlatImage, m_FlatImage->GetBufferedRegion());
  for(; !it.IsAtEnd(); ++it)
    {
    this->m_Histogram->AddSample(this->m_NativeMapping(it.Get()));
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

