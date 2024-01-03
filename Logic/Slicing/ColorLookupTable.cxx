#include "ColorLookupTable.h"
#include "itkRGBAPixel.h"

template<class TInputPixel, class TDisplayPixel>
void ColorLookupTable<TInputPixel, TDisplayPixel>
::Initialize(TInputPixel image_min, TInputPixel image_max, double t0, double t1)
{
  if constexpr(std::is_floating_point<TInputPixel>::value)
  {
    m_LUT.resize(1 + FLOAT_LUT_MAX);
    m_StartValue = (TInputPixel) (t0 * (image_max - image_min) + image_min);
    m_EndValue = (TInputPixel) (t1 * (image_max - image_min) + image_min);
    m_IntensityToLUTIndexScaleFactor = (m_EndValue == m_StartValue) ?
                                       1.0 : FLOAT_LUT_MAX / (m_EndValue - m_StartValue);
    m_LUTIndexToCurveDomainScale = (t1 - t0) / FLOAT_LUT_MAX;
    m_LUTIndexToCurveDomainShift = t0;
  }
  else
  {
    m_LUT.resize(1 + image_max - image_min);
    m_StartValue = image_min;
    m_EndValue = image_max;
    m_IntensityToLUTIndexScaleFactor = 1.0;
    m_LUTIndexToCurveDomainScale = (image_max == image_min) ? 1.0 : 1.0 / (image_max - image_min);
    m_LUTIndexToCurveDomainShift = 0;
  }
}

// Template instantiation
#define ColorLookupTableInstantiateMacro(type) \
  template class ColorLookupTable<type, itk::RGBAPixel<unsigned char> >; \
  template class ColorLookupTable<type, unsigned char>;

ColorLookupTableInstantiateMacro(unsigned char)
ColorLookupTableInstantiateMacro(char)
ColorLookupTableInstantiateMacro(unsigned short)
ColorLookupTableInstantiateMacro(short)
ColorLookupTableInstantiateMacro(float)
ColorLookupTableInstantiateMacro(double)
