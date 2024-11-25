#include "DisplayMappingPolicy.h"
#include "ImageWrapperTraits.h"
#include "ColorLabelTable.h"
#include "LabelToRGBAFilter.h"
#include "IntensityCurveVTK.h"
#include "IntensityToColorLookupTableImageFilter.h"
#include "LookupTableIntensityMappingFilter.h"
#include "RGBALookupTableIntensityMappingFilter.h"
#include "ColorMap.h"
#include "ScalarImageHistogram.h"
#include "itkMinimumMaximumImageFilter.h"
#include "itkVectorImageToImageAdaptor.h"
#include "IRISException.h"
#include "itkCommand.h"
#include "itkUnaryFunctorImageFilter.h"
#include "InputSelectionImageFilter.h"
#include "Rebroadcaster.h"
#include "TDigestImageFilter.h"
#include "ColorLookupTable.h"

/* ===============================================================
    ColorLabelTableDisplayMappingPolicy implementation
   =============================================================== */


template<class TWrapperTraits>
ColorLabelTableDisplayMappingPolicy<TWrapperTraits>
::ColorLabelTableDisplayMappingPolicy()
{
  m_Wrapper = NULL;
}

template<class TWrapperTraits>
ColorLabelTableDisplayMappingPolicy<TWrapperTraits>
::~ColorLabelTableDisplayMappingPolicy()
{

}

template<class TWrapperTraits>
void
ColorLabelTableDisplayMappingPolicy<TWrapperTraits>
::Initialize(WrapperType *wrapper)
{
  // Initialize the wrapper
  m_Wrapper = wrapper;

  // Initialize the filters
  for(unsigned int i=0; i<3; i++)
    {
    m_RGBAFilter[i] = RGBAFilterType::New();
    m_RGBAFilter[i]->SetInput(wrapper->GetSlice(i));
    m_RGBAFilter[i]->SetColorTable(NULL);
    }

}

template <class TWrapperTraits>
void
ColorLabelTableDisplayMappingPolicy<TWrapperTraits>
::UpdateImagePointer(ImageType *image)
{
  // Nothing to do here, since we are connected to the slices?
}

template<class TWrapperTraits>
typename ColorLabelTableDisplayMappingPolicy<TWrapperTraits>::DisplaySlicePointer
ColorLabelTableDisplayMappingPolicy<TWrapperTraits>
::GetDisplaySlice(unsigned int slice)
{
  return m_RGBAFilter[slice]->GetOutput();
}

template<class TWrapperTraits>
typename ColorLabelTableDisplayMappingPolicy<TWrapperTraits>::DisplayPixelType
ColorLabelTableDisplayMappingPolicy<TWrapperTraits>
::MapPixel(const InputComponentType *val)
{
  DisplayPixelType pix;
  ColorLabelTable *table = this->m_RGBAFilter[0]->GetColorTable();
  table->GetColorLabel(*val).GetRGBAVector(pix.GetDataPointer());
  return pix;
}

template<class TWrapperTraits>
void
ColorLabelTableDisplayMappingPolicy<TWrapperTraits>
::SetLabelColorTable(ColorLabelTable *labels)
{
  // Set the new table
  for(unsigned int i=0;i<3;i++)
    m_RGBAFilter[i]->SetColorTable(labels);

  // Propagate the events from to color label table to the wrapper
  Rebroadcaster::Rebroadcast(labels, SegmentationLabelChangeEvent(),
                             m_Wrapper, WrapperDisplayMappingChangeEvent());

  Rebroadcaster::Rebroadcast(labels, SegmentationLabelConfigurationChangeEvent(),
                             m_Wrapper, WrapperDisplayMappingChangeEvent());
}

template<class TWrapperTraits>
ColorLabelTable *
ColorLabelTableDisplayMappingPolicy<TWrapperTraits>
::GetLabelColorTable() const
{
  return m_RGBAFilter[0]->GetColorTable();
}


/* ===============================================================
    CachingCurveAndColorMapDisplayMappingPolicy implementation
   =============================================================== */

template<class TWrapperTraits>
CachingCurveAndColorMapDisplayMappingPolicy<TWrapperTraits>
::CachingCurveAndColorMapDisplayMappingPolicy()
{
  m_Wrapper = NULL;
}

template<class TWrapperTraits>
CachingCurveAndColorMapDisplayMappingPolicy<TWrapperTraits>
::~CachingCurveAndColorMapDisplayMappingPolicy()
{

}

template<class TWrapperTraits>
void
CachingCurveAndColorMapDisplayMappingPolicy<TWrapperTraits>
::Initialize(WrapperType *wrapper)
{
  // Initialize the intensity curve
  m_Wrapper = wrapper;

  // Initialize the LUT filter
  m_LookupTableFilter = LookupTableFilterType::New();
  m_LookupTableFilter->SetIgnoreAlpha(!wrapper->IsSticky());

  // Initialize the colormap
  m_ColorMap = ColorMap::New();
  m_ColorMap->SetToSystemPreset(
        static_cast<ColorMap::SystemPreset>(TWrapperTraits::DefaultColorMap));
  this->SetColorMap(m_ColorMap);

  // Initialize the intensity curve
  m_IntensityCurveVTK = IntensityCurveVTK::New();
  m_IntensityCurveVTK->Initialize();
  this->SetIntensityCurve(m_IntensityCurveVTK);

  // Initialize the filters that apply the LUT
  for(unsigned int i=0; i<3; i++)
    {
    m_IntensityFilter[i] = IntensityFilterType::New();
    m_IntensityFilter[i]->SetLookupTable(m_LookupTableFilter->GetLookupTable());
    }
}

template <class TWrapperTraits>
void
CachingCurveAndColorMapDisplayMappingPolicy<TWrapperTraits>
::UpdateImagePointer(ImageType *image)
{
  // Hook up the image to the filter
  m_LookupTableFilter->SetInput(m_Wrapper->GetImage());

  // Hook up the min/max filters
  m_LookupTableFilter->SetImageMinInput(m_Wrapper->GetImageMinObject());
  m_LookupTableFilter->SetImageMaxInput(m_Wrapper->GetImageMaxObject());

  for(unsigned int i=0; i<3; i++)
    m_IntensityFilter[i]->SetInput(m_Wrapper->GetSlice(i));
}

template<class TWrapperTraits>
void
CachingCurveAndColorMapDisplayMappingPolicy<TWrapperTraits>
::CopyDisplayPipeline(const Self *reference)
{
  // Copy the lookup table filter. Not sure we need to do this, or just set
  // it to NULL.
  m_LookupTableFilter = reference->m_LookupTableFilter;

  // Configure the per-slice filters
  for(unsigned int i=0; i<3; i++)
    m_IntensityFilter[i]->SetLookupTable(m_LookupTableFilter->GetLookupTable());

  // Copy the color map and the intensity curve
  this->SetColorMap(reference->m_ColorMap);
  this->SetIntensityCurve(reference->m_IntensityCurveVTK);
}

template<class TWrapperTraits>
void
CachingCurveAndColorMapDisplayMappingPolicy<TWrapperTraits>
::SetReferenceIntensityRange(const ComponentObjectType *refMin, const ComponentObjectType *refMax)
{
  m_LookupTableFilter->SetImageMinInput(refMin);
  m_LookupTableFilter->SetImageMaxInput(refMax);
}

template<class TWrapperTraits>
void
CachingCurveAndColorMapDisplayMappingPolicy<TWrapperTraits>
::ClearReferenceIntensityRange()
{
  m_LookupTableFilter->SetImageMinInput(m_Wrapper->GetImageMinObject());
  m_LookupTableFilter->SetImageMaxInput(m_Wrapper->GetImageMaxObject());
}

template<class TWrapperTraits>
void
CachingCurveAndColorMapDisplayMappingPolicy<TWrapperTraits>
::DeepCopyIntensityMap(WrapperType *srcWrapper)
{
  Self *s = srcWrapper->GetDisplayMapping();
  const IntensityCurveInterface *ici = s->m_IntensityCurveVTK;
  m_IntensityCurveVTK->Initialize(ici->GetControlPointCount());
  for(size_t i = 0; i < m_IntensityCurveVTK->GetControlPointCount(); i++)
    {
    double t, x;
    ici->GetControlPoint(i, t, x);
    m_IntensityCurveVTK->UpdateControlPoint(i, t, x);
    }
}

template<class TWrapperTraits>
Vector2d
CachingCurveAndColorMapDisplayMappingPolicy<TWrapperTraits>
::GetNativeImageRangeForCurve()
{
  return Vector2d(m_Wrapper->GetImageMinNative(), m_Wrapper->GetImageMaxNative());
}

template<class TWrapperTraits>
typename CachingCurveAndColorMapDisplayMappingPolicy<TWrapperTraits>::TDigest *
CachingCurveAndColorMapDisplayMappingPolicy<TWrapperTraits>
::GetTDigest()
{
  return m_Wrapper->GetTDigest();
}

template<class TWrapperTraits>
typename CachingCurveAndColorMapDisplayMappingPolicy<TWrapperTraits>::DisplaySlicePointer
CachingCurveAndColorMapDisplayMappingPolicy<TWrapperTraits>
::GetDisplaySlice(unsigned int dim)
{
  return m_IntensityFilter[dim]->GetOutput();
}

template<class TWrapperTraits>
ColorMap *
CachingCurveAndColorMapDisplayMappingPolicy<TWrapperTraits>
::GetColorMap() const
{
  return m_ColorMap;
}

template<class TWrapperTraits>
IntensityCurveInterface *
CachingCurveAndColorMapDisplayMappingPolicy<TWrapperTraits>
::GetIntensityCurve() const
{
  return m_IntensityCurveVTK;
}

template<class TWrapperTraits>
void
CachingCurveAndColorMapDisplayMappingPolicy<TWrapperTraits>
::SetIntensityCurve(IntensityCurveInterface *curve)
{
  m_IntensityCurveVTK = static_cast<IntensityCurveVTK *>(curve);

  // Connect the curve to the LUT filter
  m_LookupTableFilter->SetIntensityCurve(m_IntensityCurveVTK);

  // Connect modified events from the color map to appropriate events
  // from the image wrapper
  Rebroadcaster::Rebroadcast(m_IntensityCurveVTK, itk::ModifiedEvent(),
                             m_Wrapper, WrapperDisplayMappingChangeEvent());
}

template<class TWrapperTraits>
void
CachingCurveAndColorMapDisplayMappingPolicy<TWrapperTraits>
::SetColorMap(ColorMap *map)
{
  m_ColorMap = map;

  // Attach the color map to the LUT filter
  m_LookupTableFilter->SetColorMap(m_ColorMap);

  // Connect modified events from the color map to appropriate events
  // from the image wrapper
  Rebroadcaster::Rebroadcast(m_ColorMap, itk::ModifiedEvent(),
                             m_Wrapper, WrapperDisplayMappingChangeEvent());
}

template<class TWrapperTraits>
void
CachingCurveAndColorMapDisplayMappingPolicy<TWrapperTraits>
::Save(Registry &reg)
{
  m_IntensityCurveVTK->SaveToRegistry(reg.Folder("Curve"));
  m_ColorMap->SaveToRegistry(reg.Folder("ColorMap"));
}

template<class TWrapperTraits>
void
CachingCurveAndColorMapDisplayMappingPolicy<TWrapperTraits>
::Restore(Registry &reg)
{
  m_IntensityCurveVTK->LoadFromRegistry(reg.Folder("Curve"));
  m_ColorMap->LoadFromRegistry(reg.Folder("ColorMap"));
}


template<class TWrapperTraits>
typename CachingCurveAndColorMapDisplayMappingPolicy<TWrapperTraits>::DisplayPixelType
CachingCurveAndColorMapDisplayMappingPolicy<TWrapperTraits>
::MapPixel(const InputComponentType *val)
{
  DisplayPixelType pix = m_IntensityFilter[0]->MapPixel(*val);
  return pix;
}

template<class TWrapperTraits>
void CachingCurveAndColorMapDisplayMappingPolicy<TWrapperTraits>::SetSticky(bool sticky)
{
  m_LookupTableFilter->SetIgnoreAlpha(!sticky);
}





/* ===============================================================
    AbstractContinuousImageDisplayMappingPolicy implementation
   =============================================================== */

void
AbstractContinuousImageDisplayMappingPolicy
::AutoFitContrast()
{
  // Get the quantiles to fit to on the bottom and top
  this->GetTDigest()->Update();
  double imin = this->GetTDigest()->GetImageMinimum();
  double imax = this->GetTDigest()->GetImageMaximum();
  double ilow = this->GetTDigest()->GetImageQuantile(0.005);
  double ihigh = this->GetTDigest()->GetImageQuantile(0.995);

  // If for some reason the window is off, we set everything to max/min
  // TODO: check for infinity values
  if(ilow >= ihigh)
    { ilow = imin; ihigh = imax; }

  // Compute the unit coordinate values that correspond to min and max
  Vector2d irange = this->GetNativeImageRangeForCurve();
  double factor = 1.0 / (irange[1] - irange[0]);
  double t0 = factor * (ilow - irange[0]);
  double t1 = factor * (ihigh - irange[0]);

  // Set the window and level
  this->GetIntensityCurve()->ScaleControlPointsToWindow(t0, t1);
}

bool AbstractContinuousImageDisplayMappingPolicy::IsContrastInDefaultState()
{
  return this->GetIntensityCurve()->IsInDefaultState();
}

Vector2d AbstractContinuousImageDisplayMappingPolicy::GetCurveMinMaxNative()
{
  IntensityCurveInterface *curve = this->GetIntensityCurve();
  assert(curve);

  // Get the control point range
  double t0, y0, t1, y1;
  curve->GetControlPoint(0, t0, y0);
  curve->GetControlPoint(curve->GetControlPointCount() - 1, t1, y1);

  // Get the reference intensity range
  Vector2d range = this->GetNativeImageRangeForCurve();

  // Map the extents of the control points to image units
  Vector2d outRange;
  outRange[0] = range[0] * (1 - t0) + range[1] * t0;
  outRange[1] = range[0] * (1 - t1) + range[1] * t1;
  return outRange;
}


/* ===============================================================
    LinearColorMapDisplayMappingPolicy implementation
   =============================================================== */

template <class TWrapperTraits>
LinearColorMapDisplayMappingPolicy<TWrapperTraits>
::LinearColorMapDisplayMappingPolicy()
{
  m_ColorMap = ColorMap::New();
  m_ColorMap->SetToSystemPreset(
        static_cast<ColorMap::SystemPreset>(TWrapperTraits::DefaultColorMap));

  m_Wrapper = NULL;

  // Initialize the functor - based on the hard-coded range of the
  // intensity values encoded in the traits
  float imin, imax;
  TWrapperTraits::GetFixedIntensityRange(imin, imax);
  m_Functor.m_Shift = imin;
  m_Functor.m_Scale = 1.0 / (imax - imin);
  m_Functor.m_ColorMap = m_ColorMap;

  for(int i = 0; i < 3; i++)
    {
    m_Filter[i] = IntensityFilterType::New();
    m_Filter[i]->SetFunctor(m_Functor);

    // The color map is added as a 'named' input of the filter. This ensures
    // that as the colormap is modified, the filter will be updated
    m_Filter[i]->SetInput("colormap", m_ColorMap);
    }

}

template <class TWrapperTraits>
LinearColorMapDisplayMappingPolicy<TWrapperTraits>
::~LinearColorMapDisplayMappingPolicy()
{

}


template <class TWrapperTraits>
void
LinearColorMapDisplayMappingPolicy<TWrapperTraits>
::Initialize(WrapperType *wrapper)
{
  m_Wrapper = wrapper;

  for(int i = 0; i < 3; i++)
    {
    m_Filter[i]->SetInput(wrapper->GetSlice(i));
    }

  Rebroadcaster::Rebroadcast(m_ColorMap, itk::ModifiedEvent(),
                             m_Wrapper, WrapperDisplayMappingChangeEvent());
}

template <class TWrapperTraits>
void
LinearColorMapDisplayMappingPolicy<TWrapperTraits>
::UpdateImagePointer(ImageType *image)
{
  // Nothing to do here, since we are connected to the slices?
}


template <class TWrapperTraits>
typename LinearColorMapDisplayMappingPolicy<TWrapperTraits>::DisplaySlicePointer
LinearColorMapDisplayMappingPolicy<TWrapperTraits>
::GetDisplaySlice(unsigned int slice)
{
  return m_Filter[slice]->GetOutput();
}


template <class TWrapperTraits>
inline typename LinearColorMapDisplayMappingPolicy<TWrapperTraits>::DisplayPixelType
LinearColorMapDisplayMappingPolicy<TWrapperTraits>::MappingFunctor
::operator()(PixelType in)
{
  double v = (in - m_Shift) * m_Scale;
  return m_ColorMap->MapIndexToRGBA(v);
}

template <class TWrapperTraits>
bool
LinearColorMapDisplayMappingPolicy<TWrapperTraits>::MappingFunctor
::operator!=(const MappingFunctor &comp)
{
  return (comp.m_ColorMap != m_ColorMap)
      || (comp.m_Shift != m_Shift)
      || (comp.m_Scale != m_Scale);
}



template <class TWrapperTraits>
void
LinearColorMapDisplayMappingPolicy<TWrapperTraits>
::Save(Registry &reg)
{
  m_ColorMap->SaveToRegistry(reg.Folder("ColorMap"));
}

template <class TWrapperTraits>
void
LinearColorMapDisplayMappingPolicy<TWrapperTraits>
::Restore(Registry &reg)
{
  m_ColorMap->LoadFromRegistry(reg.Folder("ColorMap"));
}




template<class TWrapperTraits>
typename LinearColorMapDisplayMappingPolicy<TWrapperTraits>::DisplayPixelType
LinearColorMapDisplayMappingPolicy<TWrapperTraits>
::MapPixel(const InputComponentType *val)
{
  DisplayPixelType pix = m_Functor(*val);
  return pix;
}












/* ===============================================================
    MultiChannelDisplayMappingPolicy implementation
   =============================================================== */

template <class TWrapperTraits>
MultiChannelDisplayMappingPolicy<TWrapperTraits>
::MultiChannelDisplayMappingPolicy()
{
  m_Animate = false;
}

template <class TWrapperTraits>
MultiChannelDisplayMappingPolicy<TWrapperTraits>
::~MultiChannelDisplayMappingPolicy()
{
}




template <class TWrapperTraits>
void
MultiChannelDisplayMappingPolicy<TWrapperTraits>
::Initialize(WrapperType *wrapper)
{
  // Save the wrapper pointer
  m_Wrapper = wrapper;

  // Modified events from the display policy fire as modification events
  // for the wrapper
  Rebroadcaster::Rebroadcast(this, itk::ModifiedEvent(),
                             wrapper, WrapperMetadataChangeEvent());
  Rebroadcaster::Rebroadcast(this, itk::ModifiedEvent(),
                             wrapper, WrapperDisplayMappingChangeEvent());
}

template <class TWrapperTraits>
void
MultiChannelDisplayMappingPolicy<TWrapperTraits>
::UpdateImagePointer(ImageType *image)
{
  // Component wrappers
  typedef typename WrapperType::ComponentWrapperType ComponentWrapperType;

  // Initialize the display slice selectors
  for(unsigned int i=0; i<3; i++)
    m_DisplaySliceSelector[i] = DisplaySliceSelector::New();

  // If the number of components is 3, set up the RGB pipeline
  int nc = m_Wrapper->GetNumberOfComponents();
  bool is_2d = m_Wrapper->GetImage()->GetLargestPossibleRegion().GetSize()[2] == 1;
  if(nc == 2 || nc == 3)
    {
    m_LUTGenerator = GenerateLUTFilter::New();
    m_LUTGenerator->SetInput(m_Wrapper->GetImage());
    m_LUTGenerator->SetImageMinInput(m_Wrapper->GetImageMinObject());
    m_LUTGenerator->SetImageMaxInput(m_Wrapper->GetImageMaxObject());
    m_LUTGenerator->SetIntensityCurve(
          m_Wrapper->GetComponentWrapper(0)->GetIntensityCurve());
    m_LUTGenerator->SetIgnoreAlpha(!m_Wrapper->IsSticky());

    // Initialize the filters that apply the LUT
    for(unsigned int i=0; i<3; i++)
      {
      m_RGBMapper[i] = ApplyLUTFilter::New();
      m_RGBMapper[i]->SetLookupTable(m_LUTGenerator->GetLookupTable());

      // For two components, use the hue/value mapper (experimental)
      if(nc == 2)
          m_RGBMapper[i]->SetMappingModeToTwoChannelHueValue();
      for(unsigned int j=0; j<nc; j++)
          m_RGBMapper[i]->SetInput(j, m_Wrapper->GetComponentWrapper(j)->GetSlice(i));

      // Add this filter as the input to the selector
      m_DisplaySliceSelector[i]->AddSelectableInput(
            MultiChannelDisplayMode(true, false, SCALAR_REP_COMPONENT),
            m_RGBMapper[i]->GetOutput());

      if(nc == 3 || (nc == 2 && is_2d))
          m_DisplaySliceSelector[i]->AddSelectableInput(
                MultiChannelDisplayMode(false, true, SCALAR_REP_COMPONENT),
                m_RGBMapper[i]->GetOutput());
      }
    }
  else
    {
    m_LUTGenerator = NULL;
    for(unsigned int j=0; j<3; j++)
      m_RGBMapper[j] = NULL;
    }

  // Get the reference component wrapper whose properties will be shared
  // with the other components
  typedef typename WrapperType::ComponentWrapperType ComponentWrapper;
  ComponentWrapper *first_comp = static_cast<ComponentWrapper *>(
        m_Wrapper->GetComponentWrapper(0));

  // The min/max for this LUT should be the global min/max, overriding
  // the default, which is component-wise min/max.
  first_comp->GetDisplayMapping()->SetReferenceIntensityRange(
        m_Wrapper->GetImageMinObject(), m_Wrapper->GetImageMaxObject());

  // Configure all the component wrappers display mappings
  for(int j = 0; j < NUMBER_OF_SCALAR_REPS; j++)
    {
    ScalarRepresentation rep =
        static_cast<ScalarRepresentation>(
          SCALAR_REP_COMPONENT + j);

    int nc = (j == 0) ? m_Wrapper->GetNumberOfComponents() : 1;
    for(int k = 0; k < nc; k++)
      {
      // Get the component/derived wrapper
      ScalarImageWrapperBase *sw = m_Wrapper->GetScalarRepresentation(rep, k);

      // Try casting to the component type
      ComponentWrapper *cw = dynamic_cast<ComponentWrapper *>(sw);
      if(cw && cw != first_comp)
        {
        // Copy the LUT from the first comp to the current component.
        cw->GetDisplayMapping()->CopyDisplayPipeline(first_comp->GetDisplayMapping());
        }

      else if(cw != first_comp)
        {
        AbstractContinuousImageDisplayMappingPolicy *dp =
            static_cast<AbstractContinuousImageDisplayMappingPolicy *>(
              sw->GetDisplayMapping());

        // Copy the LUT from the first comp to the current component.
        dp->SetColorMap(first_comp->GetColorMap());
        }

      // Pass inputs to the slice selector
      for(int i = 0; i < 3; i++)
        {
        m_DisplaySliceSelector[i]->AddSelectableInput(
              MultiChannelDisplayMode(false, false, rep, k),
              sw->GetDisplaySlice(i));
        }
      }
    }

  // Set display mode to default
  SetDisplayMode(MultiChannelDisplayMode());
}

template <class TWrapperTraits>
void
MultiChannelDisplayMappingPolicy<TWrapperTraits>
::SetDisplayMode(MultiChannelDisplayMode mode)
{
  // Store the mode
  m_DisplayMode = mode;

  // Select the proper output in the selection filters
  for(int i = 0; i < 3; i++)
    m_DisplaySliceSelector[i]->SetSelectedInput(mode);

  // Point to the selected scalar representation
  int nc = m_Wrapper->GetNumberOfComponents();
  if(mode.UseRGB || mode.RenderAsGrid)
    {
    if(nc != 3 && nc != 2)
      throw IRISException("RGB mode requested for %d component image", nc);
    m_ScalarRepresentation = NULL;
    }
  else
    {
    if(mode.SelectedComponent >= nc || mode.SelectedComponent < 0)
      throw IRISException("Requested component for display %d "
                          "is not in valid range [0, %d]",
                          mode.SelectedComponent, nc);
    m_ScalarRepresentation =
        m_Wrapper->GetScalarRepresentation(
          mode.SelectedScalarRep, mode.SelectedComponent);
    if(m_ScalarRepresentation == NULL)
      std::cerr << "NULL!!!" << std::endl;
    }

  // Invoke the modified event
  this->InvokeEvent(itk::ModifiedEvent());
}


template <class TWrapperTraits>
typename MultiChannelDisplayMappingPolicy<TWrapperTraits>::DisplaySlicePointer
MultiChannelDisplayMappingPolicy<TWrapperTraits>
::GetDisplaySlice(unsigned int slice)
{
  return m_DisplaySliceSelector[slice]->GetOutput();
}

template <class TWrapperTraits>
IntensityCurveInterface *
MultiChannelDisplayMappingPolicy<TWrapperTraits>
::GetIntensityCurve() const
{
  if(m_ScalarRepresentation)
    {
    return const_cast<IntensityCurveInterface *>(m_ScalarRepresentation->GetIntensityCurve());
    }
  else
    {
    return const_cast<IntensityCurveInterface *>(m_LUTGenerator->GetIntensityCurve());
    }
}

template <class TWrapperTraits>
ColorMap *
MultiChannelDisplayMappingPolicy<TWrapperTraits>
::GetColorMap() const
{
  if(m_ScalarRepresentation)
    {
    return m_ScalarRepresentation->GetColorMap();
    }
  else return NULL;
}

template<class TWrapperTraits>
typename MultiChannelDisplayMappingPolicy<TWrapperTraits>::DisplayPixelType
MultiChannelDisplayMappingPolicy<TWrapperTraits>
::MapPixel(const InputComponentType *val)
{
  // This method should never be called directly for scalar modes (component, max, etc)
  // because VectorImageWrapper should delegate calling this function to the
  // appropriate scalar image wrapper.
  assert(!m_ScalarRepresentation);

  // Use the LUT
  if(m_Wrapper->GetNumberOfComponents() == 3)
    return m_RGBMapper[0]->MapPixel(val[0], val[1], val[2]);
  else
    return m_RGBMapper[0]->MapPixel(val[0], val[1], val[1]);
  }

template<class TWrapperTraits>
void MultiChannelDisplayMappingPolicy<TWrapperTraits>::SetSticky(bool sticky)
{
  if(m_LUTGenerator)
    m_LUTGenerator->SetIgnoreAlpha(!sticky);
}



template <class TWrapperTraits>
void
MultiChannelDisplayMappingPolicy<TWrapperTraits>
::SetColorMap(ColorMap *)
{
  // TODO: do we really need an implementation?
}


template <class TWrapperTraits>
bool
MultiChannelDisplayMappingPolicy<TWrapperTraits>
::IsContrastMultiComponent() const
{
  return m_DisplayMode.UseRGB || m_DisplayMode.RenderAsGrid || m_Animate;
}


template<class TWrapperTraits>
Vector2d
MultiChannelDisplayMappingPolicy<TWrapperTraits>
::GetNativeImageRangeForCurve()
{
  double cmin, cmax;

  // The native range is global componentwise max/min when we are in RGB mode
  // or when we are in single component mode (because the curves are shared
  // between these display modes).
  if(m_DisplayMode.UseRGB || m_DisplayMode.RenderAsGrid ||
     m_DisplayMode.SelectedScalarRep == SCALAR_REP_COMPONENT)
    {
    cmin = m_Wrapper->GetImageMinNative();
    cmax = m_Wrapper->GetImageMaxNative();
    }

  // Otherwise, when displaying a derived component, the image range is specific
  // to that component (the component has its own curve).
  else
    {
    cmin = m_ScalarRepresentation->GetImageMinNative();
    cmax = m_ScalarRepresentation->GetImageMaxNative();
    }

  return Vector2d(cmin, cmax);
}

template<class TWrapperTraits>
typename MultiChannelDisplayMappingPolicy<TWrapperTraits>::TDigest *
MultiChannelDisplayMappingPolicy<TWrapperTraits>
::GetTDigest()
{
  if(m_DisplayMode.UseRGB || m_DisplayMode.RenderAsGrid)
    return m_Wrapper->GetTDigest();
  else
    return m_ScalarRepresentation->GetTDigest();
}


template<class TWrapperTraits>
void
MultiChannelDisplayMappingPolicy<TWrapperTraits>
::AutoFitContrast()
{
  // It's safe to just call the parent's method
  Superclass::AutoFitContrast();
}


template <class TWrapperTraits>
void
MultiChannelDisplayMappingPolicy<TWrapperTraits>
::Save(Registry &folder)
{
  // If the image has only one component, use the scalar representation
  if(m_Wrapper->GetNumberOfComponents() == 1)
    {
    m_ScalarRepresentation->GetDisplayMapping()->Save(folder);
    }
  else
    {
    // We need to save the properties for each of the relevant scalar
    // representations.
    for(int i = 0; i < NUMBER_OF_SCALAR_REPS; i++)
      {
      ScalarRepresentation rep = static_cast<ScalarRepresentation>(i);
      std::string repname = MultiChannelDisplayMode::GetScalarRepNames()[rep];

      // Get the scalar representation in question
      ScalarImageWrapperBase *scalar = m_Wrapper->GetScalarRepresentation(rep);

      // Save its properties
      scalar->GetDisplayMapping()->Save(folder.Folder(repname));
      }

    // We also need to state what the current representation is and whether
    // we are using RGB mode
    m_DisplayMode.Save(folder);
    }
}

template <class TWrapperTraits>
void
MultiChannelDisplayMappingPolicy<TWrapperTraits>
::Restore(Registry &folder)
{
  // If the image has only one component, use the scalar representation
  if(m_Wrapper->GetNumberOfComponents() == 1)
    {
    m_ScalarRepresentation->GetDisplayMapping()->Restore(folder);
    }
  else
    {
    // We need to restore the properties for each of the relevant scalar
    // representations.
    for(int i = 0; i < NUMBER_OF_SCALAR_REPS; i++)
      {
      ScalarRepresentation rep = static_cast<ScalarRepresentation>(i);
      std::string repname = MultiChannelDisplayMode::GetScalarRepNames()[rep];

      // Get the scalar representation in question
      ScalarImageWrapperBase *scalar = m_Wrapper->GetScalarRepresentation(rep);

      // Save its properties
      scalar->GetDisplayMapping()->Restore(folder.Folder(repname));
      }

    // Restore the display mode
    MultiChannelDisplayMode mode = MultiChannelDisplayMode::Load(folder);
    int nc = m_Wrapper->GetNumberOfComponents();

    // Make sure the display mode is compatible
    if(m_Wrapper && (mode.UseRGB || mode.RenderAsGrid) && nc != 3 && nc != 2)
      mode = MultiChannelDisplayMode();

    if(m_Wrapper && mode.SelectedComponent >= nc)
      mode = MultiChannelDisplayMode();

    this->SetDisplayMode(mode);
    }
}

// Template instantiation
//----------------------------------------
template class ColorLabelTableDisplayMappingPolicy<LabelImageWrapperTraits>;
template class LinearColorMapDisplayMappingPolicy<LevelSetImageWrapperTraits>;
template class LinearColorMapDisplayMappingPolicy<SpeedImageWrapperTraits>;

/** Helper class for template instantiation */
template <class TPixel> class DisplayMappingPolicyTypes
{
public:
  typedef AnatomicImageWrapperTraits<TPixel, false> VectorWrapperTraits;
  typedef AnatomicScalarImageWrapperTraits<TPixel, false> ScalarWrapperTraits;
  typedef ComponentImageWrapperTraits<TPixel, false> ComponentWrapperTraits;
  typedef typename VectorToScalarImageAccessorTypes<TPixel>::MagnitudeFunctor MagnitudeFunctor;
  typedef VectorDerivedQuantityImageWrapperTraits<MagnitudeFunctor> MagnitudeWrapperTraits;
  typedef typename VectorToScalarImageAccessorTypes<TPixel>::MaxFunctor MaxFunctor;
  typedef VectorDerivedQuantityImageWrapperTraits<MaxFunctor> MaxWrapperTraits;
  typedef typename VectorToScalarImageAccessorTypes<TPixel>::MeanFunctor MeanFunctor;
  typedef VectorDerivedQuantityImageWrapperTraits<MeanFunctor> MeanWrapperTraits;
};

#define DisplayMappingPolicyInstantiateMacro(type) \
  template class MultiChannelDisplayMappingPolicy<typename DisplayMappingPolicyTypes<type>::VectorWrapperTraits>; \
  template class CachingCurveAndColorMapDisplayMappingPolicy<typename DisplayMappingPolicyTypes<type>::ScalarWrapperTraits>; \
  template class CachingCurveAndColorMapDisplayMappingPolicy<typename DisplayMappingPolicyTypes<type>::ComponentWrapperTraits>; \
  template class CachingCurveAndColorMapDisplayMappingPolicy<typename DisplayMappingPolicyTypes<type>::MagnitudeWrapperTraits>; \
  template class CachingCurveAndColorMapDisplayMappingPolicy<typename DisplayMappingPolicyTypes<type>::MaxWrapperTraits>; \
  template class CachingCurveAndColorMapDisplayMappingPolicy<typename DisplayMappingPolicyTypes<type>::MeanWrapperTraits>;

DisplayMappingPolicyInstantiateMacro(unsigned char)
DisplayMappingPolicyInstantiateMacro(char)
DisplayMappingPolicyInstantiateMacro(unsigned short)
DisplayMappingPolicyInstantiateMacro(short)
DisplayMappingPolicyInstantiateMacro(float)
DisplayMappingPolicyInstantiateMacro(double)
