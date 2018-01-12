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
#include "ThreadedHistogramImageFilter.h"
#include "ScalarImageHistogram.h"
#include "Rebroadcaster.h"
#include "UnaryFunctorVectorImageFilter.h"
#include "GuidedNativeImageIO.h"
#include "itkImageFileWriter.h"

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
  m_HistogramFilter = HistogramFilterType::New();
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

template<class TTraits, class TBase>
void
VectorImageWrapper<TTraits, TBase>
::GetRunLengthIntensityStatistics(
    const itk::ImageRegion<3> &region,
    const itk::Index<3> &startIdx, long runlength,
    double *out_sum, double *out_sumsq) const
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
        out_sum[c] += v;
        out_sumsq[c] += v * v;
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

template<class TTraits, class TBase>
void
VectorImageWrapper<TTraits,TBase>
::GetVoxelUnderCursorDisplayedValueAndAppearance(
    vnl_vector<double> &out_value, DisplayPixelType &out_appearance)
{
  // Get the numerical value
  MultiChannelDisplayMode mode = this->m_DisplayMapping->GetDisplayMode();
  if(mode.UseRGB || mode.RenderAsGrid)
    {
    // Look up the actual intensity of the voxel from the slicer
    PixelType pixel_value = this->m_Slicer[0]->LookupIntensityAtSliceIndex(this->m_ReferenceSpace);

    // Set the output value
    out_value.set_size(this->GetNumberOfComponents());
    for(int i = 0; i < this->GetNumberOfComponents(); i++)
      out_value[i] = this->m_NativeMapping(pixel_value[i]);

    // Use the display mapping to map to display pixel
    out_appearance = this->m_DisplayMapping->MapPixel(pixel_value);
    }
  else
    {
    // Just delegate to the scalar wrapper
    ScalarImageWrapperBase *siw =
        this->GetScalarRepresentation(mode.SelectedScalarRep, mode.SelectedComponent);
    siw->GetVoxelUnderCursorDisplayedValueAndAppearance(out_value, out_appearance);
    }
}


template <class TTraits, class TBase>
void
VectorImageWrapper<TTraits,TBase>
::SetNativeMapping(NativeIntensityMapping mapping)
{
  Superclass::SetNativeMapping(mapping);

  // Propagate the mapping to the histogram
  m_HistogramFilter->SetIntensityTransform(mapping.GetScale(), mapping.GetShift());

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
::CreateDerivedWrapper(ImageType *image, ImageBaseType *refSpace, ITKTransformType *transform)
{
  typedef VectorDerivedQuantityImageWrapperTraits<TFunctor> WrapperTraits;
  typedef typename WrapperTraits::WrapperType DerivedWrapper;
  typedef typename DerivedWrapper::ImageType AdaptorType;

  SmartPtr<AdaptorType> adaptor = AdaptorType::New();
  adaptor->SetImage(image);

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

template <class TTraits, class TBase>
void
VectorImageWrapper<TTraits,TBase>
::UpdateImagePointer(ImageType *newImage, ImageBaseType *referenceSpace, ITKTransformType *transform)
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

    // Pass the display geometry to the component wrapper
    for(int k = 0; k < 3; k++)
      cw->SetDisplayViewportGeometry(k, this->GetDisplayViewportGeometry(k));

    // Initialize referencing the current wrapper
    cw->InitializeToWrapper(this, comp, referenceSpace, transform);

    // Assign a parent wrapper to the derived wrapper
    cw->SetParentWrapper(this);

    // Store the wrapper
    m_ScalarReps[std::make_pair(
          SCALAR_REP_COMPONENT, i)] = cw.GetPointer();

    // Rebroadcast the events from that wrapper
    Rebroadcaster::RebroadcastAsSourceEvent(cw, WrapperChangeEvent(), this);
    }

  m_ScalarReps[std::make_pair(SCALAR_REP_MAGNITUDE, 0)]
      = this->template CreateDerivedWrapper<MagnitudeFunctor>(newImage, referenceSpace, transform);

  m_ScalarReps[std::make_pair(SCALAR_REP_MAX, 0)]
      = this->template CreateDerivedWrapper<MaxFunctor>(newImage, referenceSpace, transform);

  m_ScalarReps[std::make_pair(SCALAR_REP_AVERAGE, 0)]
      = this->template CreateDerivedWrapper<MeanFunctor>(newImage, referenceSpace, transform);

  // Create a flat representation of the image
  m_FlatImage = FlatImageType::New();
  typename FlatImageType::SizeType flatsize;
  flatsize[0] = newImage->GetPixelContainer()->Size();
  m_FlatImage->SetRegions(flatsize);
  m_FlatImage->SetPixelContainer(newImage->GetPixelContainer());

  // Connect the flat image to the min/max computer
  m_MinMaxFilter->SetInput(m_FlatImage);

  // Hook up the histogram computer to the flat image and min/max filter
  m_HistogramFilter->SetInput(m_FlatImage);
  m_HistogramFilter->SetRangeInputs(m_MinMaxFilter->GetMinimumOutput(),
                                    m_MinMaxFilter->GetMaximumOutput());

  // Set the number of bins (TODO - how to do this smartly?)
  m_HistogramFilter->SetNumberOfBins(DEFAULT_HISTOGRAM_BINS);

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

  // Call the parent's method = this will initialize the display mapping
  Superclass::UpdateImagePointer(newImage, referenceSpace, transform);

}

template<class TTraits, class TBase>
void
VectorImageWrapper<TTraits,TBase>
::SetITKTransform(ImageBaseType *referenceSpace, ITKTransformType *transform)
{
  Superclass::SetITKTransform(referenceSpace, transform);
  for(ScalarRepIterator it = m_ScalarReps.begin(); it != m_ScalarReps.end(); ++it)
    {
    it->second->SetITKTransform(referenceSpace, transform);
    }
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
  return this->GetScalarRepresentation(SCALAR_REP_MAX);
}

template<class TTraits, class TBase>
const ScalarImageHistogram *
VectorImageWrapper<TTraits,TBase>
::GetHistogram(size_t nBins)
{
  // If the user passes in a non-zero number of bins, we pass that as a
  // parameter to the filter
  if(nBins > 0)
    m_HistogramFilter->SetNumberOfBins(nBins);

  m_HistogramFilter->Update();
  return m_HistogramFilter->GetHistogramOutput();
}


template <class TTraits, class TBase>
inline ScalarImageWrapperBase *
VectorImageWrapper<TTraits,TBase>
::GetScalarRepresentation(
    ScalarRepresentation type,
    int index)
{
  return m_ScalarReps[std::make_pair(type, index)];
}

template <class TTraits, class TBase>
inline ScalarImageWrapperBase *
VectorImageWrapper<TTraits,TBase>
::GetScalarRepresentation(const ScalarRepresentationIterator &it)
{
  assert(!it.IsAtEnd());
  return this->GetScalarRepresentation(it.GetCurrent(), it.GetIndex());
}

template <class TTraits, class TBase>
bool
VectorImageWrapper<TTraits,TBase>
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

template <class TTraits, class TBase>
typename VectorImageWrapper<TTraits,TBase>::ComponentWrapperType *
VectorImageWrapper<TTraits,TBase>
::GetComponentWrapper(unsigned int index)
{
  ScalarRepIndex repidx(SCALAR_REP_COMPONENT, index);
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
::SetDisplayViewportGeometry(
    unsigned int index,
    ImageBaseType *viewport_image)
{
  Superclass::SetDisplayViewportGeometry(index, viewport_image);

  // Propagate to owned scalar wrappers
  for(ScalarRepIterator it = m_ScalarReps.begin(); it != m_ScalarReps.end(); ++it)
    {
    it->second->SetDisplayViewportGeometry(index, viewport_image);
    }
}

template <class TTraits, class TBase>
void
VectorImageWrapper<TTraits,TBase>
::SetDisplayGeometry(const IRISDisplayGeometry &dispGeom)
{
  Superclass::SetDisplayGeometry(dispGeom);
  for(ScalarRepIterator it = m_ScalarReps.begin(); it != m_ScalarReps.end(); ++it)
    it->second->SetDisplayGeometry(dispGeom);
}

template <class TTraits, class TBase>
void
VectorImageWrapper<TTraits,TBase>
::SetDirectionMatrix(const vnl_matrix<double> &direction)
{
  Superclass::SetDirectionMatrix(direction);
  for(ScalarRepIterator it = m_ScalarReps.begin(); it != m_ScalarReps.end(); ++it)
    it->second->SetDirectionMatrix(direction);
}

template <class TTraits, class TBase>
void
VectorImageWrapper<TTraits,TBase>
::CopyImageCoordinateTransform(const ImageWrapperBase *source)
{
  Superclass::CopyImageCoordinateTransform(source);
  for(ScalarRepIterator it = m_ScalarReps.begin(); it != m_ScalarReps.end(); ++it)
    it->second->CopyImageCoordinateTransform(source);
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


template<class TTraits, class TBase>
SmartPtr<typename VectorImageWrapper<TTraits, TBase>::FloatImageSource>
VectorImageWrapper<TTraits, TBase>::CreateCastToFloatPipeline() const
{
  // Just use the default representation
  return NULL;
}

template<class TTraits, class TBase>
SmartPtr<typename VectorImageWrapper<TTraits, TBase>::DoubleImageSource>
VectorImageWrapper<TTraits, TBase>::CreateCastToDoublePipeline() const
{
  // Just use the default representation
  return NULL;
}

template <class TInputPixel, class TOutputComponent, class TScalarFunctor>
class MultiComponentChannelWiseFunctor
{
public:
  typedef MultiComponentChannelWiseFunctor<TInputPixel,TOutputComponent,TScalarFunctor> Self;

  void SetNumberOfComponentsPerPixel(unsigned int n) { m_Components = n; }
  unsigned int GetNumberOfComponentsPerPixel() const { return m_Components; }

  void SetFunctor(const TScalarFunctor &functor) { m_Functor = functor; }

  void operator() (const TInputPixel &x, TOutputComponent *y) const
  {
    for(unsigned int k = 0; k < m_Components; k++)
      y[k] = m_Functor(x[k]);
  }

  bool operator != (const Self &other) const
  {
    return m_Components != other.m_Components || m_Functor != other.m_Functor;
  }

protected:
  unsigned int m_Components;
  TScalarFunctor m_Functor;
};

template<class TTraits, class TBase>
SmartPtr<typename VectorImageWrapper<TTraits, TBase>::FloatVectorImageSource>
VectorImageWrapper<TTraits, TBase>::CreateCastToFloatVectorPipeline() const
{
  typedef MultiComponentChannelWiseFunctor<PixelType, float, NativeIntensityMapping> FunctorType;
  typedef UnaryFunctorVectorImageFilter<ImageType, FloatVectorImageType, FunctorType> FilterType;
  FunctorType functor;
  functor.SetNumberOfComponentsPerPixel(this->GetNumberOfComponents());
  functor.SetFunctor(this->m_NativeMapping);
  SmartPtr<FilterType> filter = FilterType::New();
  filter->SetInput(this->m_Image);
  filter->SetFunctor(functor);

  SmartPtr<FloatVectorImageSource> output = filter.GetPointer();
  return output;
}

template<class TTraits, class TBase>
SmartPtr<typename VectorImageWrapper<TTraits, TBase>::DoubleVectorImageSource>
VectorImageWrapper<TTraits, TBase>::CreateCastToDoubleVectorPipeline() const
{
  typedef MultiComponentChannelWiseFunctor<PixelType, double, NativeIntensityMapping> FunctorType;
  typedef UnaryFunctorVectorImageFilter<ImageType, DoubleVectorImageType, FunctorType> FilterType;
  FunctorType functor;
  functor.SetNumberOfComponentsPerPixel(this->GetNumberOfComponents());
  functor.SetFunctor(this->m_NativeMapping);
  SmartPtr<FilterType> filter = FilterType::New();
  filter->SetInput(this->m_Image);
  filter->SetFunctor(functor);

  SmartPtr<DoubleVectorImageSource> output = filter.GetPointer();
  return output;
}

template<class TTraits, class TBase>
void
VectorImageWrapper<TTraits, TBase>
::WriteToFileAsFloat(const char *fname, Registry &hints)
{
  SmartPtr<GuidedNativeImageIO> io = GuidedNativeImageIO::New();
  io->CreateImageIO(fname, hints, false);
  itk::ImageIOBase *base = io->GetIOBase();

  SmartPtr<FloatVectorImageSource> pipeline = this->CreateCastToFloatVectorPipeline();
  typedef itk::ImageFileWriter<FloatVectorImageType> WriterType;
  SmartPtr<WriterType> writer = WriterType::New();
  writer->SetFileName(fname);
  if(base)
    writer->SetImageIO(base);
  writer->SetInput(pipeline->GetOutput());
  writer->Update();
}


template class VectorImageWrapper<AnatomicImageWrapperTraits<GreyType> >;

