#ifndef NATIVEINTENSITYMAPPING_H
#define NATIVEINTENSITYMAPPING_H

/**
 * @brief The AbstractNativeIntensityMapping class
 * This class is the parent in a hierarchy of classes describing a mapping from
 * the internal datatype to the 'native' datatype. These mappings are used
 * in cases where the image is represented internally as a short type, although
 * the underlying data is in the reals.
 */
class AbstractNativeIntensityMapping
{
public:
  virtual double operator() (double g) const = 0;
  virtual double MapGradientMagnitudeToNative(double g) const = 0;
  virtual double MapInternalToNative(double g) const = 0;
  virtual double MapNativeToInternal(double g) const = 0;
  virtual double GetScale() const = 0;
  virtual double GetShift() const = 0;
};

class LinearInternalToNativeIntensityMapping : public AbstractNativeIntensityMapping
{
public:
  typedef LinearInternalToNativeIntensityMapping Self;

  double operator() (double g) const
    { return MapInternalToNative(g); }

  double MapGradientMagnitudeToNative(double internalGM) const
    { return internalGM * scale; }

  double MapInternalToNative(double internal) const
    { return internal * scale + shift; }

  double MapNativeToInternal(double native) const
    { return (native - shift) / scale; }

  virtual double GetScale() const { return scale; }
  virtual double GetShift() const { return shift; }

  LinearInternalToNativeIntensityMapping() : scale(1.0), shift(0.0) {}
  LinearInternalToNativeIntensityMapping(double a, double b) : scale(a), shift(b) {}

  bool operator != (const Self &other) const
    { return scale != other.scale || shift != other.shift; }

protected:
  double scale;
  double shift;
};

class SpeedImageInternalToNativeIntensityMapping
    : public LinearInternalToNativeIntensityMapping
{
public:

  typedef SpeedImageInternalToNativeIntensityMapping Self;

  SpeedImageInternalToNativeIntensityMapping()
  {
    // Map the range of short to -1 : 1
    short smin = -0x7fff, smax = 0x7fff;
    this->scale = 2.0 / ((int) smax - (int) smin);
    this->shift = 0.0;
  }

  bool operator != (const Self &other) const { return false; }

};

class IdentityInternalToNativeIntensityMapping : public AbstractNativeIntensityMapping
{
public:

  typedef IdentityInternalToNativeIntensityMapping Self;

  double operator() (double g) const
    { return g; }

  double MapGradientMagnitudeToNative(double internalGM) const
    { return internalGM; }

  double MapInternalToNative(double internal) const
    { return internal; }

  double MapNativeToInternal(double native) const
    { return native; }

  virtual double GetScale() const { return 1; }
  virtual double GetShift() const { return 0; }

  bool operator != (const Self &other) const { return false; }
};



#endif // NATIVEINTENSITYMAPPING_H
