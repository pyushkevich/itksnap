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
void
ColorLabelTableDisplayMappingPolicy<TWrapperTraits>
::SetLabelColorTable(ColorLabelTable *labels)
{
  // Set the new table
  for(unsigned int i=0;i<3;i++)
    m_RGBAFilter[i]->SetColorTable(labels);
}

template<class TWrapperTraits>
ColorLabelTable *
ColorLabelTableDisplayMappingPolicy<TWrapperTraits>
::GetLabelColorTable() const
{
  return m_RGBAFilter[0]->GetColorTable();
}

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
  m_IntensityCurveVTK = IntensityCurveVTK::New();
  m_IntensityCurveVTK->Initialize();

  // Initialize the colormap
  m_ColorMap = ColorMap::New();

  // Initialize the LUT filter
  m_LookupTableFilter = LookupTableFilterType::New();
  m_LookupTableFilter->SetIntensityCurve(m_IntensityCurveVTK);
  m_LookupTableFilter->SetColorMap(m_ColorMap);

  // Initialize the filters that apply the LUT
  for(unsigned int i=0; i<3; i++)
    {
    m_IntensityFilter[i] = IntensityFilterType::New();
    m_IntensityFilter[i]->SetLookupTable(m_LookupTableFilter->GetOutput());
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
  m_LookupTableFilter->SetImageMinInput(m_Wrapper->GetMinMaxFilter()->GetMinimumOutput());
  m_LookupTableFilter->SetImageMaxInput(m_Wrapper->GetMinMaxFilter()->GetMaximumOutput());

  for(unsigned int i=0; i<3; i++)
    {
    m_IntensityFilter[i]->SetInput(m_Wrapper->GetSlice(i));
    m_IntensityFilter[i]->SetImageMinInput(m_Wrapper->GetMinMaxFilter()->GetMinimumOutput());
    m_IntensityFilter[i]->SetImageMaxInput(m_Wrapper->GetMinMaxFilter()->GetMaximumOutput());
    }
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
    {
    m_IntensityFilter[i]->SetLookupTable(m_LookupTableFilter->GetOutput());
    m_IntensityFilter[i]->SetImageMinInput(m_LookupTableFilter->GetImageMinInput());
    m_IntensityFilter[i]->SetImageMaxInput(m_LookupTableFilter->GetImageMaxInput());
    }

  // Copy the color map and the intensity curve
  m_ColorMap = reference->m_ColorMap;
  m_IntensityCurveVTK = reference->m_IntensityCurveVTK;
}

template<class TWrapperTraits>
void
CachingCurveAndColorMapDisplayMappingPolicy<TWrapperTraits>
::DeriveFromReferenceWrapper(WrapperType *srcWrapper)
{
  // TODO: Figure out how to do this right...

  /*
  SetReferenceIntensityRange(srcWrapper->GetImageMinAsDouble(),
                             srcWrapper->GetImageMaxAsDouble());

  CopyIntensityMap(srcWrapper);

  m_ColorMap->CopyInformation(
        srcWrapper->GetDisplayMapping()->GetColorMap());
        */
}

template<class TWrapperTraits>
void
CachingCurveAndColorMapDisplayMappingPolicy<TWrapperTraits>
::SetReferenceIntensityRange(ComponentObjectType *refMin, ComponentObjectType *refMax)
{
  m_LookupTableFilter->SetImageMinInput(refMin);
  m_LookupTableFilter->SetImageMaxInput(refMax);
}

template<class TWrapperTraits>
void
CachingCurveAndColorMapDisplayMappingPolicy<TWrapperTraits>
::ClearReferenceIntensityRange()
{
  m_LookupTableFilter->SetImageMinInput(m_Wrapper->GetMinMaxFilter()->GetMinimumOutput());
  m_LookupTableFilter->SetImageMaxInput(m_Wrapper->GetMinMaxFilter()->GetMaximumOutput());
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
    float t, x;
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
  m_LookupTableFilter->SetIntensityCurve(m_IntensityCurveVTK);
}

template<class TWrapperTraits>
void
CachingCurveAndColorMapDisplayMappingPolicy<TWrapperTraits>
::SetColorMap(ColorMap *map)
{
  m_ColorMap = map;
  m_LookupTableFilter->SetColorMap(m_ColorMap);
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
void
CachingCurveAndColorMapDisplayMappingPolicy<TWrapperTraits>
::AutoFitContrast()
{
  // Get the histogram
  const ScalarImageHistogram *hist = m_Wrapper->GetHistogram();

  // Integrate the histogram until reaching 0.1%
  PixelType imin = hist->GetBinMin(0);
  PixelType ilow = imin;
  size_t accum = 0;
  size_t accum_goal = m_Wrapper->GetNumberOfVoxels() / 1000;
  for(size_t i = 0; i < hist->GetSize(); i++)
    {
    if(accum + hist->GetFrequency(i) < accum_goal)
      {
      accum += hist->GetFrequency(i);
      ilow = hist->GetBinMax(i);
      }
    else break;
    }

  // Same, but from above
  PixelType imax = hist->GetBinMax(hist->GetSize() - 1);
  PixelType ihigh = imax;
  accum = 0;
  for(int i = (int) hist->GetSize() - 1; i >= 0; i--)
    {
    if(accum + hist->GetFrequency(i) < accum_goal)
      {
      accum += hist->GetFrequency(i);
      ihigh = hist->GetBinMin(i);
      }
    else break;
    }

  // If for some reason the window is off, we set everything to max/min
  if(ilow >= ihigh)
    { ilow = imin; ihigh = imax; }

  // Compute the unit coordinate values that correspond to min and max
  double iAbsMax = m_Wrapper->GetImageMaxAsDouble();
  double iAbsMin = m_Wrapper->GetImageMinAsDouble();
  double factor = 1.0 / (iAbsMax - iAbsMin);
  double t0 = factor * (ilow - iAbsMin);
  double t1 = factor * (ihigh - iAbsMin);

  // Set the window and level
  m_IntensityCurveVTK->ScaleControlPointsToWindow(
        (float) t0, (float) t1);
}


template <class TWrapperTraits>
LinearColorMapDisplayMappingPolicy<TWrapperTraits>
::LinearColorMapDisplayMappingPolicy()
{
  m_ColorMap = ColorMap::New();
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

















MultiChannelDisplayMode::MultiChannelDisplayMode()
{
  UseRGB = false;
  SelectedScalarRep = VectorImageWrapperBase::SCALAR_REP_COMPONENT;
  SelectedComponent = 0;
}

MultiChannelDisplayMode::MultiChannelDisplayMode(
    bool use_rgb, VectorImageWrapperBase::ScalarRepresentation rep,
    int comp)
  : UseRGB(use_rgb), SelectedScalarRep(rep),
    SelectedComponent(comp)
{

}

MultiChannelDisplayMode
MultiChannelDisplayMode::DefaultForRGB()
{
  MultiChannelDisplayMode mode;
  mode.UseRGB = true;
  return mode;
}

void MultiChannelDisplayMode::Save(Registry &reg)
{
  reg["UseRGB"] << UseRGB;
  reg["SelectedScalarRep"].PutEnum(GetScalarRepNames(), SelectedScalarRep);
  reg["SelectedComponent"] << SelectedComponent;
}

MultiChannelDisplayMode
MultiChannelDisplayMode::Load(Registry &reg)
{
  MultiChannelDisplayMode mode;
  mode.UseRGB = reg["UseRGB"][mode.UseRGB];
  mode.SelectedScalarRep = reg["SelectedScalarRep"].GetEnum(
        GetScalarRepNames(), mode.SelectedScalarRep);
  mode.SelectedComponent = reg["SelectedComponent"][mode.SelectedComponent];
  return mode;
}

RegistryEnumMap<MultiChannelDisplayMode::ScalarRepresentation> &
MultiChannelDisplayMode::GetScalarRepNames()
{
  static RegistryEnumMap<ScalarRepresentation> namemap;
  if(namemap.Size() == 0)
    {
    namemap.AddPair(VectorImageWrapperBase::SCALAR_REP_COMPONENT, "Component");
    namemap.AddPair(VectorImageWrapperBase::SCALAR_REP_MAGNITUDE, "Magnitude");
    namemap.AddPair(VectorImageWrapperBase::SCALAR_REP_MAX, "Maximum");
    namemap.AddPair(VectorImageWrapperBase::SCALAR_REP_AVERAGE, "Average");
    }
  return namemap;
}

int MultiChannelDisplayMode::GetHashValue() const
{
  if(UseRGB)
    return 10000;

  if(SelectedScalarRep != VectorImageWrapperBase::SCALAR_REP_COMPONENT)
    return SelectedScalarRep * 100;

  return SelectedComponent;
}

bool operator < (const MultiChannelDisplayMode &a, const MultiChannelDisplayMode &b)
{
  return a.GetHashValue() < b.GetHashValue();
}





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
  if(m_Wrapper->GetNumberOfComponents() == 3)
    {
    m_LUTGenerator = GenerateLUTFilter::New();
    m_LUTGenerator->SetInput(m_Wrapper->GetImage());
    m_LUTGenerator->SetImageMinInput(m_Wrapper->GetImageMinObject());
    m_LUTGenerator->SetImageMaxInput(m_Wrapper->GetImageMaxObject());
    m_LUTGenerator->SetIntensityCurve(
          m_Wrapper->GetComponentWrapper(0)->GetIntensityCurve());

    // Initialize the filters that apply the LUT
    for(unsigned int i=0; i<3; i++)
      {
      m_RGBMapper[i] = ApplyLUTFilter::New();
      m_RGBMapper[i]->SetLookupTable(m_LUTGenerator->GetOutput());

      for(unsigned int j=0; j<3; j++)
        {
        ComponentWrapperType *comp = m_Wrapper->GetComponentWrapper(j);
        m_RGBMapper[i]->SetInput(j, comp->GetSlice(i));
        }

      // Add this filter as the input to the selector
      m_DisplaySliceSelector[i]->AddSelectableInput(
            MultiChannelDisplayMode(true, VectorImageWrapperBase::SCALAR_REP_COMPONENT),
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
  for(int j = 0; j < VectorImageWrapperBase::NUMBER_OF_SCALAR_REPS; j++)
    {
    VectorImageWrapperBase::ScalarRepresentation rep =
        static_cast<VectorImageWrapperBase::ScalarRepresentation>(
          VectorImageWrapperBase::SCALAR_REP_COMPONENT + j);

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
              MultiChannelDisplayMode(false, rep, k),
              sw->GetDisplaySlice(i));
        }
      }
    }

  // Set display mode to default
  SetDisplayMode(MultiChannelDisplayMode());

  // Listen to modified events from all the intensity curves and color maps
  // handled by this object
  // TODO: is it not better to just move the AbstractModel framework into the
  // SNAP logic layer and make these children of abstract model and use
  // Rebroadcast?
  typedef itk::SimpleMemberCommand<Self> Command;
  SmartPtr<Command> cmd = Command::New();
  cmd->SetCallbackFunction(this, &Self::ModifiedEventCallback);
  if(m_Wrapper->GetNumberOfComponents() == 1)
    {
    // When there is only one component, we only need to worry about it, not
    // the derived components
    this->GetIntensityCurve()->AddObserver(itk::ModifiedEvent(), cmd);
    this->GetColorMap()->AddObserver(itk::ModifiedEvent(), cmd);
    }
  else
    {
    // Apply this for all scalar components
    for(int k = 0; k < VectorImageWrapperBase::NUMBER_OF_SCALAR_REPS; k++)
      {
      ScalarImageWrapperBase *s = m_Wrapper->GetScalarRepresentation(
            static_cast<VectorImageWrapperBase::ScalarRepresentation>(k));
      s->GetDisplayMapping()->GetIntensityCurve()->AddObserver(
            itk::ModifiedEvent(), cmd);
      }

    // The colormap is shared among all
    ScalarImageWrapperBase *s0 =
        m_Wrapper->GetScalarRepresentation(VectorImageWrapperBase::SCALAR_REP_COMPONENT);
    s0->GetColorMap()->AddObserver(itk::ModifiedEvent(), cmd);
    }

}


template <class TWrapperTraits>
void
MultiChannelDisplayMappingPolicy<TWrapperTraits>
::ModifiedEventCallback()
{
  this->InvokeEvent(itk::ModifiedEvent());
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
  if(mode.UseRGB)
    {
    if(nc != 3)
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
    return m_ScalarRepresentation->GetIntensityCurve();
    }
  else
    {
    return m_LUTGenerator->GetIntensityCurve();
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

template <class TWrapperTraits>
void
MultiChannelDisplayMappingPolicy<TWrapperTraits>
::SetColorMap(ColorMap *map)
{
  // TODO: do we really need an implementation?
}


template <class TWrapperTraits>
bool
MultiChannelDisplayMappingPolicy<TWrapperTraits>
::IsContrastMultiComponent() const
{
  if(m_DisplayMode.UseRGB || m_Animate)
    return true;

  return false;
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
  if(m_DisplayMode.UseRGB ||
     m_DisplayMode.SelectedScalarRep == VectorImageWrapperBase::SCALAR_REP_COMPONENT)
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
void
MultiChannelDisplayMappingPolicy<TWrapperTraits>
::AutoFitContrast()
{
  if(IsContrastMultiComponent())
    {
    // Get the pooled histogram of all the layers - how?
    // TODO: fix this soon!
    }
  else
    {
    AbstractContinuousImageDisplayMappingPolicy *dp =
        static_cast<AbstractContinuousImageDisplayMappingPolicy *>(
          m_ScalarRepresentation->GetDisplayMapping());
    dp->AutoFitContrast();
    }
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
    for(int i = 0; i < VectorImageWrapperBase::NUMBER_OF_SCALAR_REPS; i++)
      {
      ScalarRep rep = static_cast<ScalarRep>(i);
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
    for(int i = 0; i < VectorImageWrapperBase::NUMBER_OF_SCALAR_REPS; i++)
      {
      ScalarRep rep = static_cast<ScalarRep>(i);
      std::string repname = MultiChannelDisplayMode::GetScalarRepNames()[rep];

      // Get the scalar representation in question
      ScalarImageWrapperBase *scalar = m_Wrapper->GetScalarRepresentation(rep);

      // Save its properties
      scalar->GetDisplayMapping()->Restore(folder.Folder(repname));
      }

    // Restore the display mode
    // this->SetDisplayMode(MultiChannelDisplayMode::Load(folder));
    }
}


template <class TWrapperTraits>
void
MultiChannelDisplayMappingPolicy<TWrapperTraits>
::DeriveFromReferenceWrapper(WrapperType *refwrapper)
{
  // TODO:
  // I guess we have to call this method for each of the scalar wrappers
}








template class ColorLabelTableDisplayMappingPolicy<LabelImageWrapperTraits>;

template class LinearColorMapDisplayMappingPolicy<LevelSetImageWrapperTraits>;
template class CachingCurveAndColorMapDisplayMappingPolicy<SpeedImageWrapperTraits>;

template class MultiChannelDisplayMappingPolicy<AnatomicImageWrapperTraits<GreyType> >;

template class CachingCurveAndColorMapDisplayMappingPolicy<ComponentImageWrapperTraits<GreyType> >;
template class CachingCurveAndColorMapDisplayMappingPolicy<
    VectorDerivedQuantityImageWrapperTraits<GreyVectorToScalarMagnitudeFunctor> >;
template class CachingCurveAndColorMapDisplayMappingPolicy<
    VectorDerivedQuantityImageWrapperTraits<GreyVectorToScalarMaxFunctor> >;
template class CachingCurveAndColorMapDisplayMappingPolicy<
    VectorDerivedQuantityImageWrapperTraits<GreyVectorToScalarMeanFunctor> >;




