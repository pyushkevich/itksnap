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
#include "RLEImageRegionIterator.h"
#include "itkImageSliceConstIteratorWithIndex.h"
#include "itkNumericTraits.h"
#include "itkRegionOfInterestImageFilter.h"
#include "itkRescaleIntensityImageFilter.h"
#include "itkIdentityTransform.h"
#include "AdaptiveSlicingPipeline.h"
#include "SNAPSegmentationROISettings.h"
#include "itkCommand.h"
#include "ImageWrapperTraits.h"
#include "itkVectorImageToImageAdaptor.h"
#include "itkMinimumMaximumImageFilter.h"
#include "Rebroadcaster.h"
#include "GuidedNativeImageIO.h"
#include "itkImageFileWriter.h"

#include <iostream>

#include "itkVectorGradientAnisotropicDiffusionImageFilter.h"

template <class TTraits>
VectorImageWrapper<TTraits>
::VectorImageWrapper()
{
}

template <class TTraits>
VectorImageWrapper<TTraits>
::~VectorImageWrapper()
{
}



template <class TTraits>
typename VectorImageWrapper<TTraits>::ImagePointer
VectorImageWrapper<TTraits>
::DeepCopyRegion(const SNAPSegmentationROISettings &roi,
                 itk::Command *progressCommand) const
{
   return Superclass::DeepCopyRegion(roi, progressCommand);
}

template<class TTraits>
void
VectorImageWrapper<TTraits>
::GetRunLengthIntensityStatistics(
    const itk::ImageRegion<3> &region,
    const itk::Index<3> &startIdx, long runlength,
    double *out_nvalid, double *out_sum, double *out_sumsq) const
{
  if(this->IsSlicingOrthogonal())
    {
    ConstIterator it(this->m_Image, region);
    it.SetIndex(startIdx);
    size_t nc = this->GetNumberOfComponents();

    // Perform the integration
    for(long q = 0; q < runlength; q++, ++it)
      {
      PixelType p = it.Get();
      for(size_t c = 0; c < nc; c++)
        {
        double v = (double) p[c];
        if(!std::isnan(v))
          {
          *out_nvalid += 1.0;
          *out_sum += v;
          *out_sumsq += v * v;
          }
        }
      }
    }
  else
    {
    // TODO: implement non-orthogonal statistics
    size_t nc = this->GetNumberOfComponents();
    for(size_t c = 0; c < nc; c++)
      {
      out_sum[c] += nan("");
      out_sumsq[c] += nan("");
      }
    }
}

template<class TTraits>
void
VectorImageWrapper<TTraits>
::GetVoxelUnderCursorDisplayedValueAndAppearance(
    vnl_vector<double> &out_value, DisplayPixelType &out_appearance)
{
  // Get the numerical value
  MultiChannelDisplayMode mode = this->m_DisplayMapping->GetDisplayMode();
  if(mode.UseRGB || mode.RenderAsGrid)
    {
    // Sample the intensity under the cursor for the current time point
    this->SampleIntensityAtReferenceIndex(
          this->m_SliceIndex, this->GetTimePointIndex(), true, out_value);

    // The call above updates the variable m_IntensitySamplingArray, which
    // we use to map intensity
    out_appearance = this->m_DisplayMapping->MapPixel(
                       this->m_IntensitySamplingArray.data_block() +
                       this->GetTimePointIndex() * this->GetNumberOfComponents());
    }
  else
    {
    // Just delegate to the scalar wrapper
    ScalarImageWrapperBase *siw =
        this->GetScalarRepresentation(mode.SelectedScalarRep, mode.SelectedComponent);
    siw->GetVoxelUnderCursorDisplayedValueAndAppearance(out_value, out_appearance);
    }
}

template<class TTraits>
typename VectorImageWrapper<TTraits>::DisplaySlicePointer
VectorImageWrapper<TTraits>
::SampleArbitraryDisplaySlice(const ImageBaseType *ref_space)
{
  // Get the numerical value
  MultiChannelDisplayMode mode = this->m_DisplayMapping->GetDisplayMode();
  if(mode.UseRGB || mode.RenderAsGrid)
    {
    // Create a non-orthogonal slicer for this task - we don't want to interfere with the
    // main slicing pipeline
    using ThumbSlicer = typename SlicerType::NonOrthogonalSlicerType;
    typename ThumbSlicer::Pointer thumb_slicer = ThumbSlicer::New();
    thumb_slicer->SetReferenceImage(ref_space);
    thumb_slicer->SetInput(this->GetImage());

    // The affine transform is set to identity
    typedef itk::IdentityTransform<double, 3> IdTransformType;
    typename IdTransformType::Pointer idTran = IdTransformType::New();
    thumb_slicer->SetTransform(idTran);

    // Perform slicing
    thumb_slicer->Update();
    auto *raw_thumb = thumb_slicer->GetOutput();

    // Allocate the output image
    typename Superclass::DisplaySlicePointer thumb_image = DisplaySliceType::New();
    typename DisplaySliceType::RegionType thumb_region;
    thumb_region.SetSize(0, ref_space->GetBufferedRegion().GetSize()[0]);
    thumb_region.SetSize(1, ref_space->GetBufferedRegion().GetSize()[1]);
    thumb_image->SetRegions(thumb_region);
    thumb_image->Allocate();

    // Now, put this through the display pipeline
    auto *src_pix = raw_thumb->GetBufferPointer(), *p = src_pix;
    auto *dst_pix = thumb_image->GetBufferPointer(), *q = dst_pix;
    unsigned int n_pix = thumb_region.GetNumberOfPixels();
    unsigned int k = raw_thumb->GetNumberOfComponentsPerPixel();
    for(; q < dst_pix + n_pix; p += k, q++)
      *q = this->GetDisplayMapping()->MapPixel(p);

    return thumb_image;
    }
  else
    {
    // Just delegate to the scalar wrapper
    ScalarImageWrapperBase *siw =
        this->GetScalarRepresentation(mode.SelectedScalarRep, mode.SelectedComponent);
    return siw->SampleArbitraryDisplaySlice(ref_space);
    }
}



template <class TTraits>
void
VectorImageWrapper<TTraits>
::SetNativeMapping(NativeIntensityMapping mapping)
{
  Superclass::SetNativeMapping(mapping);

  // Propagate to owned scalar wrappers
  for(ScalarRepIterator it = m_ScalarReps.begin(); it != m_ScalarReps.end(); ++it)
    {
    ScalarRepIndex idx = it->first;
    if(idx.first == SCALAR_REP_COMPONENT)
      {
      // Cast the wrapper the right type
      ComponentWrapperType *cw =
          dynamic_cast<ComponentWrapperType *>(it->second.GetPointer());

      // Pass the native to the component wrapper
      cw->SetNativeMapping(mapping);
      }

    // These are the derived wrappers. They use the identity mapping, but they
    // need to know what the source native mapping is.
    else if(idx.first == SCALAR_REP_MAGNITUDE)
      {
      SetNativeMappingInDerivedWrapper<MagnitudeFunctor>(it->second, mapping);
      }
    else if(idx.first == SCALAR_REP_MAX)
      {
      SetNativeMappingInDerivedWrapper<MaxFunctor>(it->second, mapping);
      }
    else if(idx.first == SCALAR_REP_AVERAGE)
      {
      SetNativeMappingInDerivedWrapper<MeanFunctor>(it->second, mapping);
      }
    }
}

template <class TTraits>
template <class TFunctor>
void
VectorImageWrapper<TTraits>
::SetNativeMappingInDerivedWrapper(
    ScalarImageWrapperBase *w,
    NativeIntensityMapping &mapping)
{
  typedef VectorDerivedQuantityImageWrapperTraits<TFunctor> WrapperTraits;
  typedef typename WrapperTraits::WrapperType DerivedWrapper;
  typedef typename DerivedWrapper::Image4DType AdaptorType;
  typedef typename AdaptorType::AccessorType PixelAccessor;

  // Cast to the right type
  DerivedWrapper *dw = dynamic_cast<DerivedWrapper *>(w);
  dw->SetSourceNativeMapping(mapping.GetScale(), mapping.GetShift());
}

template <class TTraits>
template <class TFunctor>
SmartPtr<ScalarImageWrapperBase>
VectorImageWrapper<TTraits>
::CreateDerivedWrapper(Image4DType *image_4d, ImageBaseType *refSpace, ITKTransformType *transform)
{
  typedef VectorDerivedQuantityImageWrapperTraits<TFunctor> WrapperTraits;
  typedef typename WrapperTraits::WrapperType DerivedWrapper;
  typedef typename DerivedWrapper::Image4DType AdaptorType;

  SmartPtr<AdaptorType> adaptor = AdaptorType::New();
  adaptor->SetImage(image_4d);

  SmartPtr<DerivedWrapper> wrapper = DerivedWrapper::New();
  wrapper->InitializeToWrapper(this, adaptor, refSpace, transform);

  // Assign a parent wrapper to the derived wrapper
  wrapper->SetParentWrapper(this);

  // Pass the display geometry to the component wrapper
  for(int k = 0; k < 3; k++)
    wrapper->SetDisplayViewportGeometry(k, this->GetDisplayViewportGeometry(k));

  SmartPtr<ScalarImageWrapperBase> ptrout = wrapper.GetPointer();

  // When creating derived wrappers, we need to rebroadcast the events from
  // that wrapper as our own events
  Rebroadcaster::RebroadcastAsSourceEvent(wrapper, WrapperChangeEvent(), this);

  return ptrout;
}

template <class TTraits>
void
VectorImageWrapper<TTraits>
::UpdateWrappedImages(Image4DType *image_4d, ImageBaseType *referenceSpace, ITKTransformType *transform)
{
  // Create the component wrappers before calling the parent's method.
  int nc = image_4d->GetNumberOfComponentsPerPixel();

  // The first component image will serve as the reference for the other
  // component images
  ComponentWrapperType *cref = NULL;

  for(int i = 0; i < nc; i++)
    {
    // Create a component image
    typedef itk::VectorImageToImageAdaptor<InternalPixelType,4> ComponentImage;
    SmartPtr<ComponentImage> comp = ComponentImage::New();
    comp->SetImage(image_4d);
    comp->SetSpacing(image_4d->GetSpacing());
    comp->SetOrigin(image_4d->GetOrigin());
    comp->SetDirection(image_4d->GetDirection());
    comp->SetExtractComponentIndex(i);

    // Create a wrapper for this image and assign the component image
    SmartPtr<ComponentWrapperType> cw = ComponentWrapperType::New();

    // Pass the display geometry to the component wrapper
    for(int k = 0; k < 3; k++)
      cw->SetDisplayViewportGeometry(k, this->GetDisplayViewportGeometry(k));

    // Initialize referencing the current wrapper
    cw->InitializeToWrapper(this, comp, referenceSpace, transform);

    // Assign a parent wrapper to the derived wrapper
    cw->SetParentWrapper(this);

    // Store the wrapper
    m_ScalarReps[std::make_pair(SCALAR_REP_COMPONENT, i)] = cw.GetPointer();

    // Rebroadcast the events from that wrapper
    Rebroadcaster::RebroadcastAsSourceEvent(cw, WrapperChangeEvent(), this);
    }

  m_ScalarReps[std::make_pair(SCALAR_REP_MAGNITUDE, 0)]
      = this->template CreateDerivedWrapper<MagnitudeFunctor>(image_4d, referenceSpace, transform);

  m_ScalarReps[std::make_pair(SCALAR_REP_MAX, 0)]
      = this->template CreateDerivedWrapper<MaxFunctor>(image_4d, referenceSpace, transform);

  m_ScalarReps[std::make_pair(SCALAR_REP_AVERAGE, 0)]
      = this->template CreateDerivedWrapper<MeanFunctor>(image_4d, referenceSpace, transform);

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
    m_ScalarReps[std::make_pair(SCALAR_REP_COMPONENT, i)]
        = cw.GetPointer();
    }

  // Initialize the computed derived wrappers
  ColorMap *cm = cref->GetDisplayMapping()->GetColorMap(); */

  // Call the parent's method = this will initialize the display mapping. This should
  // be called after the component/child wrappers have been created
  Superclass::UpdateWrappedImages(image_4d, referenceSpace, transform);
}

template<class TTraits>
void
VectorImageWrapper<TTraits>
::SetITKTransform(ImageBaseType *referenceSpace, ITKTransformType *transform)
{
  Superclass::SetITKTransform(referenceSpace, transform);
  for(auto &it : m_ScalarReps)
    it.second->SetITKTransform(referenceSpace, transform);
}

template<class TTraits>
void
VectorImageWrapper<TTraits>
::SetReferenceSpace(ImageBaseType *referenceSpace)
{
  Superclass::SetReferenceSpace(referenceSpace);
  for(auto &it : m_ScalarReps)
    it.second->SetReferenceSpace(referenceSpace);
}


template <class TTraits>
inline ScalarImageWrapperBase *
VectorImageWrapper<TTraits>
::GetDefaultScalarRepresentation()
{
  ScalarImageWrapperBase *rep =
      this->m_DisplayMapping->GetScalarRepresentation();
  if(rep)
    return rep;

  // TODO: This is somewhat arbitrary! Maybe it should be something the user
  // can change under settings, i.e., "Default scalar representation for RGB images".
  return this->GetScalarRepresentation(SCALAR_REP_MAX);
}

template <class TTraits>
inline ScalarImageWrapperBase *
VectorImageWrapper<TTraits>
::GetScalarRepresentation(
    ScalarRepresentation type,
    int index)
{
  return m_ScalarReps[std::make_pair(type, index)];
}

template <class TTraits>
inline ScalarImageWrapperBase *
VectorImageWrapper<TTraits>
::GetScalarRepresentation(const ScalarRepresentationIterator &it)
{
  assert(!it.IsAtEnd());
  return this->GetScalarRepresentation(it.GetCurrent(), it.GetIndex());
}

template <class TTraits>
bool
VectorImageWrapper<TTraits>
::FindScalarRepresentation(
    ImageWrapperBase *scalar_rep, ScalarRepresentation &type, int &index) const
{
  for(ScalarRepConstIterator it = m_ScalarReps.begin(); it != m_ScalarReps.end(); ++it)
    {
    if(it->second.GetPointer() == scalar_rep)
      {
      type = it->first.first;
      index = it->first.second;
      return true;
      }
    }

  return false;
}

template <class TTraits>
typename VectorImageWrapper<TTraits>::ComponentWrapperType *
VectorImageWrapper<TTraits>
::GetComponentWrapper(unsigned int index)
{
  ScalarRepIndex repidx(SCALAR_REP_COMPONENT, index);
  return static_cast<ComponentWrapperType *>(m_ScalarReps[repidx].GetPointer());
}

template <class TTraits>
void
VectorImageWrapper<TTraits>
::SetSliceIndex(const IndexType &cursor)
{
  Superclass::SetSliceIndex(cursor);

  // Propagate to owned scalar wrappers
  for(ScalarRepIterator it = m_ScalarReps.begin(); it != m_ScalarReps.end(); ++it)
    {
    it->second->SetSliceIndex(cursor);
    }
}

template <class TTraits>
void
VectorImageWrapper<TTraits>
::SetDisplayViewportGeometry(
    unsigned int index,
    const ImageBaseType *viewport_image)
{
  Superclass::SetDisplayViewportGeometry(index, viewport_image);

  // Propagate to owned scalar wrappers
  for(ScalarRepIterator it = m_ScalarReps.begin(); it != m_ScalarReps.end(); ++it)
    {
    it->second->SetDisplayViewportGeometry(index, viewport_image);
    }
}

template <class TTraits>
void
VectorImageWrapper<TTraits>
::SetSticky(bool value)
{
  Superclass::SetSticky(value);

  // Propagate to owned scalar wrappers
  for(ScalarRepIterator it = m_ScalarReps.begin(); it != m_ScalarReps.end(); ++it)
    it->second->SetSticky(value);
}

template <class TTraits>
void
VectorImageWrapper<TTraits>
::SetDisplayGeometry(const IRISDisplayGeometry &dispGeom)
{
  Superclass::SetDisplayGeometry(dispGeom);
  for(ScalarRepIterator it = m_ScalarReps.begin(); it != m_ScalarReps.end(); ++it)
    it->second->SetDisplayGeometry(dispGeom);
}

template <class TTraits>
void
VectorImageWrapper<TTraits>
::SetDirectionMatrix(const vnl_matrix<double> &direction)
{
  Superclass::SetDirectionMatrix(direction);
  for(ScalarRepIterator it = m_ScalarReps.begin(); it != m_ScalarReps.end(); ++it)
    it->second->SetDirectionMatrix(direction);
}

template<class TTraits>
void
VectorImageWrapper<TTraits>
::SetSlicingInterpolationMode(InterpolationMode mode)
{
    Superclass::SetSlicingInterpolationMode(mode);
    for(ScalarRepIterator it = m_ScalarReps.begin(); it != m_ScalarReps.end(); ++it)
        it->second->SetSlicingInterpolationMode(mode);
}

template <class TTraits>
void
VectorImageWrapper<TTraits>
::CopyImageCoordinateTransform(const ImageWrapperBase *source)
{
  Superclass::CopyImageCoordinateTransform(source);
  for(ScalarRepIterator it = m_ScalarReps.begin(); it != m_ScalarReps.end(); ++it)
    it->second->CopyImageCoordinateTransform(source);
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


template<class TTraits>
void
VectorImageWrapper<TTraits>
::WriteToFileAsFloat(const char *fname, Registry &hints)
{
  SmartPtr<GuidedNativeImageIO> io = GuidedNativeImageIO::New();
  io->CreateImageIO(fname, hints, false);
  itk::ImageIOBase *base = io->GetIOBase();

  // Create a pipeline that casts the image to floating type
  auto *float_img = this->CreateCastToFloatVectorPipeline("WriteToFileAsFloat");

  typedef itk::ImageFileWriter<typename ImageWrapperBase::FloatVectorImageType> WriterType;
  SmartPtr<WriterType> writer = WriterType::New();
  writer->SetFileName(fname);
  if(base)
    writer->SetImageIO(base);
  writer->SetInput(float_img);
  writer->Update();

  // Release the pipeline (what a pain)
  this->ReleaseInternalPipeline("WriteToFileAsFloat");
}

// --------------------------------------------
// Explicit template instantiation
#define VectorImageWrapperInstantiateMacro(type) \
  template class VectorImageWrapper<typename ImageWrapperTraits<type>::VectorTraits>;

VectorImageWrapperInstantiateMacro(unsigned char)
VectorImageWrapperInstantiateMacro(char)
VectorImageWrapperInstantiateMacro(unsigned short)
VectorImageWrapperInstantiateMacro(short)
VectorImageWrapperInstantiateMacro(float)
VectorImageWrapperInstantiateMacro(double)
