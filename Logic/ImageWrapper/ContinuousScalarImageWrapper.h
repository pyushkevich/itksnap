#ifndef CONTINUOUSSCALARIMAGEWRAPPER_H
#define CONTINUOUSSCALARIMAGEWRAPPER_H

#ifdef GOOGOOOGOO

#include <ScalarImageWrapper.h>
#include <itkUnaryFunctorImageFilter.h>
#include <UnaryFunctorCache.h>

/**
  This is a base class for traits used to convert between native intensity
  values in an image wrapper and RGBA values displayed to the user. This
  mapping may involve color maps, intensity curves, color pallettes, etc.
  The traits are used to parameterize a ScalarImageWrapper
  */
template <class TNative>
class AbstractNativeToDisplayTraits
{
public:



};

template <class TNative>
class ColorMapNativeToDisplayFunctor
{
public:
  ImageWrapperBase::DisplayPixelType operator()(const TNative &in) const
  {
    // Map the input value to range of 0 to 1
    double inZeroOne = (in - m_IntensityMin) * m_IntensityFactor;

    // Map the output to a RGBA pixel
    return m_Parent->m_ColorMap->MapIndexToRGBA(inZeroOne);
  }

  double m_IntensityMin, m_IntensityFactor;
  ColorMap *m_ColorMap;
};

/**
What are the behaviors we want to support?

 - Cache + Curve + ColorMap
 - Cache + ColorMap
 - ColorMap
*/

template<class TNative>
class ColorMapNativeToDisplayTraits
{
public:

  typedef ColorMapNativeToDisplayTraits Self;
  typedef ColorMapNativeToDisplayFunctor<TNative> IntensityFunctor;
  typedef itk::Image<TNative, 2> NativeImageType;
  typedef itk::UnaryFunctorImageFilter<
    NativeImageType, NativeImageType, IntensityFunctor> IntensityFilter;

  void CopyInformation(const Self &ref);

  IntensityFunctor GetFunctor();


protected:
  SmartPtr<ColorMap> m_ColorMap;
};


template<class TNative>
class CachingColorMapNativeToDisplayTraits
{
public:
  typedef ImageWrapperBase::DisplayPixelType DisplayPixelType;
  typedef ColorMapNativeToDisplayFunctor<TNative> WrappedIntensityFunctor;
  typedef itk::Image<TNative, 2> NativeImageType;
  typedef UnaryFunctorCache<TNative,DisplayPixelType,WrappedIntensityFunctor> CacheType;
  typedef typename CacheType::CachingFunctor IntensityFunctor;

  typedef itk::UnaryFunctorImageFilter<
    NativeImageType, NativeImageType, IntensityFunctor> IntensityFilter;

protected:
  SmartPtr<ColorMap> m_ColorMap;
  SmartPtr<CacheType> m_Cache;
};


template <class TInputImage, class TOutputImage>
class NativeToDisplayPixelFilter : public itk::InPlaceImageFilter<TInputImage, TOutputImage>
{
public:



  itkSetMacro(UseCaching, bool)

  itkSetMacro(IntensityCurve, IntensityCurveInterface *)

  itkSetMacro(ColorMap, ColorMap *)

  void GenerateData();
};





template <class TPixel, class TNativeToDisplayTraits>
class ContinuousScalarImageWrapper :
  public ScalarImageWrapper<TPixel>, public ContinuousScalarImageWrapperBase
{
public:
  ContinuousScalarImageWrapper();


  typedef TNativeToDisplayTraits TraitsType;
  typedef typename TraitsType::IntensityFunctor IntensityFunctor;

  typedef itk::UnaryFunctorImageFilter<
    SliceType, DisplaySliceType, IntensityFunctor> IntensityFilter;

  /**
    Make sure that the display slice in i-th direction is up-to-date. This
    checks the m-time of all the things that the display slice depends on
    and updates accordingly.
    */
  void UpdateDisplaySlice(unsigned int dim);

  /**
   * Get the display slice in a given direction.  To change the
   * display slice, call parent's MoveToSlice() method
   */
  DisplaySlicePointer GetDisplaySlice(unsigned int dim);


  /** Get voxel at the position as an RGBA object. This returns the appearance
    of the voxel for display (applying all intensity transformations) */
  void GetVoxelDisplayAppearance(const Vector3ui &x, DisplayPixelType &out);


protected:

  // The traits object
  TraitsType m_Traits;

  // The intensity filters
  SmartPtr<IntensityFilter> m_IntensityFilter[3];

};






#endif










#endif // CONTINUOUSSCALARIMAGEWRAPPER_H
