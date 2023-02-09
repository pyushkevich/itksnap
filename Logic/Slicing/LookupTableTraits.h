#ifndef LOOKUPTABLETRAITS_H
#define LOOKUPTABLETRAITS_H

#include <algorithm>
#include <itkMacro.h>

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

  static unsigned int GetLUTSize(TPixel imin, TPixel imax)
  {
    return 1 + imax - imin;
  }

  static void GetLUTIntensityRange(
      TPixel imin, TPixel imax, double itkNotUsed(t0), double itkNotUsed(t1),
      TPixel &i0, TPixel &i1)
  {
    i0 = imin; i1 = imax;
  }

  static void ComputeLinearMappingToUnitInterval(
      TPixel imin, TPixel imax, double itkNotUsed(t0), double itkNotUsed(t1),
      double &scale, double &shift)
  {
    shift = imin;
    scale = 1.0f / (imax - imin);
  }

  static double ComputeIntensityToLUTIndexScaleFactor(const TPixel &itkNotUsed(lut_start), const TPixel &itkNotUsed(lut_end))
    {
    return 1.0;
    }


  // Compute offset into the LUT for an input value
  static int ComputeLUTOffset(double itkNotUsed(scale), int shift, int value)
  {
    return value - shift;
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

  static unsigned int GetLUTSize(TPixel itkNotUsed(imin), TPixel itkNotUsed(imax))
  {
    return 1 + LUT_MAX - LUT_MIN;
  }

  static void GetLUTIntensityRange(
      TPixel imin, TPixel imax, double t0, double t1,
      TPixel &i0, TPixel &i1)
  {
    i0 = (TPixel) (t0 * (imax - imin) + imin);
    i1 = (TPixel) (t1 * (imax - imin) + imin);
  }

  static void ComputeLinearMappingToUnitInterval(
      TPixel itkNotUsed(imin), TPixel itkNotUsed(imax), double t0, double t1,
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

  static double ComputeIntensityToLUTIndexScaleFactor(const TPixel &lut_start, const TPixel &lut_end)
    {
    return (LUT_MAX - LUT_MIN) * 1.0 / (lut_end - lut_start);
    }

  static int ComputeLUTOffset(double scale, TPixel shift, TPixel value)
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
