#ifndef COLORLOOKUPTABLE_H
#define COLORLOOKUPTABLE_H

#include "SNAPCommon.h"
#include "itkDataObject.h"
#include <type_traits>
#include <cmath>

/**
 * This class defines a lookup table that is used to map from raw intensity to
 * a color value. It is just a vector of RGBA pixels with some extra metadata
 * to specify range and color for NaN values.
 */
template <class TInputPixel, class TDisplayPixel> class ColorLookupTable
    : public itk::DataObject
{
public:
  irisITKObjectMacro(ColorLookupTable, itk::DataObject)

  // Maximum size of the LUT for floating point data
  static constexpr int FLOAT_LUT_MAX = 10000;

  /**
   * Initialize LUT storage and compute the intensity values corresponding
   * to the ends of the LUT. This method behaves differently depending on
   * the input pixel type. For short/char, the color maps starts at image
   * minimum and goes through the image maximum. For float, the color map
   * starts at the first control point of the intensity mapping curve and
   * ends at the last control point (because the whole intensity range can
   * be huge and the user might want to zoom in on a part of it). For float,
   * the size of the map is fixed.
   */
  void Initialize(TInputPixel image_min, TInputPixel image_max, double t0, double t1);

  /** Map an intensity value from the image to the display type, no range check for char/short */
  inline TDisplayPixel MapIntensityToDisplay(const TInputPixel &x) const
    {
    if constexpr(std::is_floating_point<TInputPixel>::value)
      {
      if(std::isnan(x))
        return m_ColorNaN;
      else if(x < m_StartValue)
        return m_ColorBelow;
      else if(x > m_EndValue)
        return m_ColorAbove;
      else
        return m_LUT[(int)((x - m_StartValue) * m_IntensityToLUTIndexScaleFactor)];
      }
    else
      {
      return m_LUT[x - m_StartValue];
      }
    }

  /** Perform a range check (is intensity in the mapped range) - normally not required */
  bool CheckRange(const TInputPixel &x) const
    {
    return x >= m_StartValue && x <= m_EndValue;
    }

  /** Get the size of the LUT */
  unsigned int GetSize() const { return m_LUT.size(); }

  /** Get the size of the LUT */
  double GetIntensityCurveDomainValueForIndex(unsigned int index) const
    {
    return index * m_LUTIndexToCurveDomainScale + m_LUTIndexToCurveDomainShift;
    }

  /** Set the value of the LUT entry */
  void SetLUTValue(unsigned int index, const TDisplayPixel &value) { m_LUT[index] = value; }

  /** Color used for intensities below the covered range */
  itkGetConstMacro(ColorBelow, TDisplayPixel)

  /** Color used for intensities below the covered range */
  itkSetMacro(ColorBelow, TDisplayPixel)

  /** Color used for intensities above the covered range */
  itkGetConstMacro(ColorAbove, TDisplayPixel)

  /** Color used for intensities above the covered range */
  itkSetMacro(ColorAbove, TDisplayPixel)

  /** Color used for NAN */
  itkGetConstMacro(ColorNaN, TDisplayPixel)

  /** Color used for NAN */
  itkSetMacro(ColorNaN, TDisplayPixel)


protected:
  ColorLookupTable() {}
  virtual ~ColorLookupTable() {}

  // The table itself
  std::vector<TDisplayPixel> m_LUT;

  // Colors for the values outside of the specified range
  TDisplayPixel m_ColorBelow, m_ColorAbove, m_ColorNaN;

  // Specification for the ends of the table
  TInputPixel m_StartValue, m_EndValue;

  // Scale factor used during fast computation
  double m_IntensityToLUTIndexScaleFactor;

  // Linear transform between LUT index and the intensity curve domain
  double m_LUTIndexToCurveDomainScale, m_LUTIndexToCurveDomainShift;
};


#endif // COLORLOOKUPTABLE_H
