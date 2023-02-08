#ifndef LOOKUPTABLETRAITS_H
#define LOOKUPTABLETRAITS_H

#include <algorithm>

/**
 * This is a traits class that modifies the behavior of this filter for
 * integral and non-integral data types. For data types whose range is
 * small (char/short/uchar/ushort), the lookup table assigns a color to
 * each possible value of the input image. For data types whose range is
 * large (int, float, double, etc), the lookup table allocates 2^16 values
 * into which the image intensities are scaled.
 */
template <class TPixel>
class SmallIntegralTypeLookupTableTraits
{
public:
  typedef itk::ImageRegion<1> LUTRange;
  static LUTRange ComputeLUTRange(TPixel imin, TPixel imax)
  {
    LUTRange range;
    range.SetIndex(0, imin);
    range.SetSize(0, 1 + imax - imin);
    return range;
  }

  static void ComputeLinearMappingToUnitInterval(
      TPixel imin, TPixel imax, double itkNotUsed(t0), double itkNotUsed(t1),
      double &scale, double &shift)
  {
    shift = imin;
    scale = 1.0f / (imax - imin);
  }

  static void ComputeLinearMappingToLUT(
      TPixel imin, TPixel imax,
      float &scale, TPixel &shift)
  {
    scale = 1.0;
    shift = 0;
  }

  // Compute offset into the LUT for an input value
  static int ComputeLUTOffset(float itkNotUsed(scale),
                              TPixel itkNotUsed(shift),
                              TPixel value)
  {
    return value;
  }

protected:

};

/**
 * This is the version of the above used for types float and double. These
 * types are always mapped to some fixed range of values. We fix this range
 * of values at 10000 - something completely arbitrary.
 */
template <class TPixel>
class RealTypeLookupTableTraits
{
public:
  static constexpr int LUT_MIN = 0, LUT_MAX = 10000;
  typedef itk::ImageRegion<1> LUTRange;
  static LUTRange ComputeLUTRange(TPixel imin, TPixel imax)
  {
    LUTRange range;
    range.SetIndex(0, LUT_MIN);
    range.SetSize(0, 1 + LUT_MAX - LUT_MIN);
    return range;
  }

  static void ComputeLinearMappingToUnitInterval(
      TPixel imin, TPixel imax, double t0, double t1,
      double &scale, double &shift)
  {
    // t = t0 + (t1 - t0) q, where q = (x - LUT_MIN) / (LUT_MAX - LUT_MIN)
    // t = t0 + (t1 - t0) / (LUT_MAX - LUT_MIN) x - (t1 - t0) LUT_MIN / (LUT_MAX - LUT_MIN)
    scale = (t1 - t0) / (LUT_MAX - LUT_MIN);
    shift = t0 - scale * LUT_MIN;


    // OLD:
    // scale = (t1 - t0) / (LUT_MAX - LUT_MIN);
    // shift = LUT_MIN;
  }

  static void ComputeLinearMappingToLUT(
      TPixel imin, TPixel imax,
      float &scale, TPixel &shift)
  {
    scale = (LUT_MAX - LUT_MIN) * 1.0 / (imax - imin);
    shift = imin;
  }

  // Compute offset into the LUT for an input value
  static int ComputeLUTOffset(float scale, TPixel shift, TPixel value)
  {
    int pos = static_cast<int>((value - shift) * scale);
    return std::clamp(pos, LUT_MIN, LUT_MAX);
  }


};

/**
 * Link each C type to the corresponding traits
 */
template <typename T> class LookupTableTraits
{
};

#define DECL_LUT_TRAITS(type,traits) \
  template<> class LookupTableTraits<type> : public traits<type> {};

DECL_LUT_TRAITS(float,          RealTypeLookupTableTraits)
DECL_LUT_TRAITS(double,         RealTypeLookupTableTraits)
DECL_LUT_TRAITS(short,          SmallIntegralTypeLookupTableTraits)
DECL_LUT_TRAITS(unsigned short, SmallIntegralTypeLookupTableTraits)
DECL_LUT_TRAITS(char,           SmallIntegralTypeLookupTableTraits)
DECL_LUT_TRAITS(unsigned char,  SmallIntegralTypeLookupTableTraits)


#endif // LOOKUPTABLETRAITS_H
