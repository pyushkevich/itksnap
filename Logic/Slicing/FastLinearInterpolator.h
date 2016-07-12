/*=========================================================================

  Program:   ALFABIS fast medical image registration programs
  Language:  C++
  Website:   github.com/pyushkevich/greedy
  Copyright (c) Paul Yushkevich, University of Pennsylvania. All rights reserved.

  This program is part of ALFABIS: Adaptive Large-Scale Framework for
  Automatic Biomedical Image Segmentation.

  ALFABIS development is funded by the NIH grant R01 EB017255.

  ALFABIS is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  ALFABIS is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with ALFABIS.  If not, see <http://www.gnu.org/licenses/>.

=========================================================================*/
#ifndef __FastLinearInterpolator_h_
#define __FastLinearInterpolator_h_

#include "itkVectorImage.h"
#include "itkNumericTraits.h"

template <class TFloat, class TInputComponentType>
struct FastLinearInterpolatorOutputTraits
{
};

template <class TInputComponentType>
struct FastLinearInterpolatorOutputTraits<float, TInputComponentType>
{
  typedef typename itk::NumericTraits<TInputComponentType>::FloatType OutputComponentType;
};

template <class TInputComponentType>
struct FastLinearInterpolatorOutputTraits<double, TInputComponentType>
{
  typedef typename itk::NumericTraits<TInputComponentType>::RealType OutputComponentType;
};

template <class TImageType>
struct FastWarpCompositeImageFilterInputImageTraits
{
};

template <class TPixel, unsigned int VDim>
struct FastWarpCompositeImageFilterInputImageTraits< itk::Image<TPixel, VDim> >
{
  static int GetPointerIncrementSize(const itk::Image<TPixel, VDim> *) { return 1; }
};

template <class TPixel, unsigned int VDim>
struct FastWarpCompositeImageFilterInputImageTraits< itk::VectorImage<TPixel, VDim> >
{
  static int GetPointerIncrementSize(const itk::VectorImage<TPixel, VDim> *image)
  {
    return image->GetNumberOfComponentsPerPixel();
  }
};


/**
 * Base class for the fast linear interpolators
 */
template<class TImage, class TFloat, unsigned int VDim>
class FastLinearInterpolatorBase
{
public:
  typedef TImage                                                  ImageType;
  typedef TFloat                                                  RealType;
  typedef typename ImageType::InternalPixelType                   InputComponentType;
  typedef FastLinearInterpolatorOutputTraits<TFloat, InputComponentType>  OutputTraits;
  typedef typename OutputTraits::OutputComponentType              OutputComponentType;

  /** Determine the image dimension. */
  itkStaticConstMacro(ImageDimension, unsigned int, ImageType::ImageDimension );

  enum InOut { INSIDE, OUTSIDE, BORDER };

  /**
   * Get the number that should be added to the input pointer when parsing the input and
   * output images. This will be 1 for itk::Image and Ncomp for itk::VectorImage
   */
  int GetPointerIncrement() const { return nComp; }

  FastLinearInterpolatorBase(ImageType *image)
  {
    buffer = image->GetBufferPointer();
    nComp = FastWarpCompositeImageFilterInputImageTraits<TImage>::GetPointerIncrementSize(image);
    def_value_store = new InputComponentType[nComp];
    for(int i = 0; i < nComp; i++)
      def_value_store[i] = itk::NumericTraits<InputComponentType>::Zero;
    def_value = def_value_store;
  }

  ~FastLinearInterpolatorBase()
  {
    delete [] def_value_store;
  }

protected:


  int nComp;
  const InputComponentType *buffer;

  // Default value - for interpolation outside of the image bounds
  const InputComponentType *def_value;
  InputComponentType *def_value_store;

  InOut status;


  template <class TInput>
  inline OutputComponentType lerp(RealType a, const TInput &l, const TInput &h)
  {
    return l+((h-l)*a);
  }
};


/**
 * Arbitrary dimension fast linear interpolator - meant to be slow
 */
template<class TImage, class TFloat, unsigned int VDim>
class FastLinearInterpolator : public FastLinearInterpolatorBase<TImage, TFloat, VDim>
{
public:
  typedef FastLinearInterpolatorBase<TImage, TFloat, VDim>   Superclass;
  typedef typename Superclass::ImageType               ImageType;
  typedef typename Superclass::InputComponentType      InputComponentType;
  typedef typename Superclass::OutputComponentType     OutputComponentType;
  typedef typename Superclass::RealType                RealType;
  typedef typename Superclass::InOut                   InOut;

  FastLinearInterpolator(ImageType *image) : Superclass(image) {}

  InOut InterpolateWithGradient(RealType *cix, OutputComponentType *out, OutputComponentType **grad)
    { return Superclass::INSIDE; }

  InOut Interpolate(RealType *cix, OutputComponentType *out)
    { return Superclass::INSIDE; }

  InOut InterpolateNearestNeighbor(RealType *cix, OutputComponentType *out)
    { return Superclass::INSIDE; }

  TFloat GetMask() { return 0.0; }

  TFloat GetMaskAndGradient(RealType *mask_gradient) { return 0.0; }

  template <class THistContainer>
  void PartialVolumeHistogramSample(RealType *cix, const InputComponentType *fixptr, THistContainer &hist) {}

  template <class THistContainer>
  void PartialVolumeHistogramGradientSample(RealType *cix, const InputComponentType *fix_ptr, const THistContainer &hist_w, RealType *out_grad) {}


protected:
};

/**
 * 3D fast linear interpolator - optimized for speed
 */
template <class TImage, class TFloat>
class FastLinearInterpolator<TImage, TFloat, 3>
    : public FastLinearInterpolatorBase<TImage, TFloat, 3>
{
public:
  typedef TImage                                             ImageType;
  typedef FastLinearInterpolatorBase<ImageType, TFloat, 3>   Superclass;
  typedef typename Superclass::InputComponentType            InputComponentType;
  typedef typename Superclass::OutputComponentType           OutputComponentType;
  typedef typename Superclass::RealType                      RealType;
  typedef typename Superclass::InOut                         InOut;

  FastLinearInterpolator(ImageType *image) : Superclass(image)
  {
    xsize = image->GetLargestPossibleRegion().GetSize()[0];
    ysize = image->GetLargestPossibleRegion().GetSize()[1];
    zsize = image->GetLargestPossibleRegion().GetSize()[2];
  }

  /**
   * Compute the pointers to the eight corners of the interpolating cube
   */
  InOut ComputeCorners(RealType *cix)
  {
    const InputComponentType *dp;

    x0 = (int) floor(cix[0]); fx = cix[0] - x0;
    y0 = (int) floor(cix[1]); fy = cix[1] - y0;
    z0 = (int) floor(cix[2]); fz = cix[2] - z0;

    x1 = x0 + 1;
    y1 = y0 + 1;
    z1 = z0 + 1;

    if (x0 >= 0 && x1 < xsize &&
        y0 >= 0 && y1 < ysize &&
        z0 >= 0 && z1 < zsize)
      {
      // The sample point is completely inside
      dp = dens(x0, y0, z0);
      d000 = dp;
      d100 = dp+this->nComp;
      dp += xsize*this->nComp;
      d010 = dp;
      d110 = dp+this->nComp;
      dp += xsize*ysize*this->nComp;
      d011 = dp;
      d111 = dp+this->nComp;
      dp -= xsize*this->nComp;
      d001 = dp;
      d101 = dp+this->nComp;

      // The mask is one
      this->status = Superclass::INSIDE;
      }
    else if (x0 >= -1 && x1 <= xsize &&
             y0 >= -1 && y1 <= ysize &&
             z0 >= -1 && z1 <= zsize)
      {
      // The sample point is on the border region
      d000 = border_check(x0, y0, z0, m000);
      d001 = border_check(x0, y0, z1, m001);
      d010 = border_check(x0, y1, z0, m010);
      d011 = border_check(x0, y1, z1, m011);
      d100 = border_check(x1, y0, z0, m100);
      d101 = border_check(x1, y0, z1, m101);
      d110 = border_check(x1, y1, z0, m110);
      d111 = border_check(x1, y1, z1, m111);

      // The mask is between 0 and 1
      this->status = Superclass::BORDER;
      }
    else
      {
      // The mask is zero
      this->status = Superclass::OUTSIDE;
      }

    return this->status;
  }

  /**
   * Interpolate at position cix, placing the intensity values in out and gradient
   * values in grad (in strides of VDim)
   */
  InOut InterpolateWithGradient(RealType *cix, OutputComponentType *out, OutputComponentType **grad)
  {
    RealType dx00, dx01, dx10, dx11, dxy0, dxy1;
    RealType dx00_x, dx01_x, dx10_x, dx11_x, dxy0_x, dxy1_x;
    RealType dxy0_y, dxy1_y;

    // Compute the corners
    this->ComputeCorners(cix);

    if(this->status != Superclass::OUTSIDE)
      {
      // Loop over the components
      for(int iComp = 0; iComp < this->nComp; iComp++, grad++,
          d000++, d001++, d010++, d011++,
          d100++, d101++, d110++, d111++)
        {
        // Interpolate the image intensity
        dx00 = Superclass::lerp(fx, *d000, *d100);
        dx01 = Superclass::lerp(fx, *d001, *d101);
        dx10 = Superclass::lerp(fx, *d010, *d110);
        dx11 = Superclass::lerp(fx, *d011, *d111);
        dxy0 = Superclass::lerp(fy, dx00, dx10);
        dxy1 = Superclass::lerp(fy, dx01, dx11);
        *(out++) = Superclass::lerp(fz, dxy0, dxy1);

        // Interpolate the gradient in x
        dx00_x = *d100 - *d000;
        dx01_x = *d101 - *d001;
        dx10_x = *d110 - *d010;
        dx11_x = *d111 - *d011;
        dxy0_x = this->lerp(fy, dx00_x, dx10_x);
        dxy1_x = this->lerp(fy, dx01_x, dx11_x);
        (*grad)[0] = this->lerp(fz, dxy0_x, dxy1_x);

        // Interpolate the gradient in y
        dxy0_y = dx10 - dx00;
        dxy1_y = dx11 - dx01;
        (*grad)[1] = this->lerp(fz, dxy0_y, dxy1_y);

        // Interpolate the gradient in z
        (*grad)[2] = dxy1 - dxy0;
        }
      }

    return this->status;
  }

  InOut Interpolate(RealType *cix, OutputComponentType *out)
  {
    OutputComponentType dx00, dx01, dx10, dx11, dxy0, dxy1;

    // Compute the corners
    this->ComputeCorners(cix);

    if(this->status != Superclass::OUTSIDE)
      {
      // Loop over the components
      for(int iComp = 0; iComp < this->nComp; iComp++,
          d000++, d001++, d010++, d011++,
          d100++, d101++, d110++, d111++)
        {
        // Interpolate the image intensity
        dx00 = Superclass::lerp(fx, *d000, *d100);
        dx01 = Superclass::lerp(fx, *d001, *d101);
        dx10 = Superclass::lerp(fx, *d010, *d110);
        dx11 = Superclass::lerp(fx, *d011, *d111);
        dxy0 = Superclass::lerp(fy, dx00, dx10);
        dxy1 = Superclass::lerp(fy, dx01, dx11);
        *(out++) = Superclass::lerp(fz, dxy0, dxy1);
        }
      }

    return this->status;
  }

  InOut InterpolateNearestNeighbor(RealType *cix, OutputComponentType *out)
  {
    x0 = (int) floor(cix[0] + 0.5);
    y0 = (int) floor(cix[1] + 0.5);
    z0 = (int) floor(cix[2] + 0.5);

    if (x0 >= 0 && x0 < xsize &&
        y0 >= 0 && y0 < ysize &&
        z0 >= 0 && z0 < zsize)
      {
      const InputComponentType *dp = dens(x0, y0, z0);
      for(int iComp = 0; iComp < this->nComp; iComp++)
        {
        out[iComp] = dp[iComp];
        }
      return Superclass::INSIDE;
      }
    else return Superclass::OUTSIDE;
  }


  template <class THistContainer>
  void PartialVolumeHistogramSample(RealType *cix, const InputComponentType *fixptr, THistContainer &hist)
  {
    // Compute the corners
    this->ComputeCorners(cix);

    if(this->status != Superclass::OUTSIDE)
      {
      // Compute the corner weights using 4 multiplications (not 16)
      RealType fxy = fx * fy, fyz = fy * fz, fxz = fx * fz, fxyz = fxy * fz;

      RealType w111 = fxyz;
      RealType w011 = fyz - fxyz;
      RealType w101 = fxz - fxyz;
      RealType w110 = fxy - fxyz;
      RealType w001 = fz - fxz - w011;
      RealType w010 = fy - fyz - w110;
      RealType w100 = fx - fxy - w101;
      RealType w000 = 1.0 - fx - fy + fxy - w001;

      // Loop over the components
      for(int iComp = 0; iComp < this->nComp; iComp++,
          d000++, d001++, d010++, d011++,
          d100++, d101++, d110++, d111++, fixptr++)
        {
        // Just this line in the histogram
        RealType *hist_line = hist[iComp][*fixptr];

        // Assign the appropriate weight to each part of the histogram
        hist_line[*d000] += w000;
        hist_line[*d001] += w001;
        hist_line[*d010] += w010;
        hist_line[*d011] += w011;
        hist_line[*d100] += w100;
        hist_line[*d101] += w101;
        hist_line[*d110] += w110;
        hist_line[*d111] += w111;
        }
      }
    else
      {
      for(int iComp = 0; iComp < this->nComp; iComp++, fixptr++)
        {
        // Just this line in the histogram
        RealType *hist_line = hist[iComp][*fixptr];
        hist_line[0] += 1.0;
        }
      }
  }

  template <class THistContainer>
  void PartialVolumeHistogramGradientSample(RealType *cix, const InputComponentType *fixptr, const THistContainer &hist_w, RealType *out_grad)
  {
    // Compute the corners
    this->ComputeCorners(cix);

    // Outside values do not contribute to the gradient
    if(this->status != Superclass::OUTSIDE)
      {
      // Compute the corner weights using 4 multiplications (not 16)
      RealType fxy = fx * fy, fyz = fy * fz, fxz = fx * fz;

      // Some horrendous derivatives here! Wow!
      RealType w111x = fyz,             w111y = fxz,             w111z = fxy;
      RealType w011x = -fyz,            w011y = fz - fxz,        w011z = fy - fxy;
      RealType w101x = fz - fyz,        w101y = -fxz,            w101z = fx - fxy;
      RealType w110x = fy - fyz,        w110y = fx - fxz,        w110z = -fxy;
      RealType w001x = -fz - w011x,     w001y = -w011y,          w001z = 1 - fx - w011z;
      RealType w010x = -w110x,          w010y = 1 - fz - w110y,  w010z = -fy - w110z;
      RealType w100x = 1 - fy - w101x,  w100y = -fx - w101y,     w100z = -w101z;
      RealType w000x = -1 + fy - w001x, w000y = -1 + fx - w001y, w000z = -w001z;

      // Initialize gradient to zero
      out_grad[0] = 0.0;
      out_grad[1] = 0.0;
      out_grad[2] = 0.0;

      // Loop over the components
      for(int iComp = 0; iComp < this->nComp; iComp++,
          d000++, d001++, d010++, d011++,
          d100++, d101++, d110++, d111++, fixptr++)
        {
        // Just this line in the histogram
        const RealType *f = hist_w[iComp][*fixptr];

        // Take the weighted sum
        RealType f000 = f[*d000], f001 = f[*d001], f010 = f[*d010], f011 = f[*d011];
        RealType f100 = f[*d100], f101 = f[*d101], f110 = f[*d110], f111 = f[*d111];

        out_grad[0] += w000x * f000 + w001x * f001 + w010x * f010 + w011x * f011 +
                       w100x * f100 + w101x * f101 + w110x * f110 + w111x * f111;

        out_grad[1] += w000y * f000 + w001y * f001 + w010y * f010 + w011y * f011 +
                       w100y * f100 + w101y * f101 + w110y * f110 + w111y * f111;

        out_grad[2] += w000z * f000 + w001z * f001 + w010z * f010 + w011z * f011 +
                       w100z * f100 + w101z * f101 + w110z * f110 + w111z * f111;
        }
      }
    else
      {
      out_grad[0] = 0.0;
      out_grad[1] = 0.0;
      out_grad[2] = 0.0;
      }
  }

  RealType GetMask()
  {
    // Interpolate the mask
    RealType dx00, dx01, dx10, dx11, dxy0, dxy1;
    dx00 = this->lerp(fx, m000, m100);
    dx01 = this->lerp(fx, m001, m101);
    dx10 = this->lerp(fx, m010, m110);
    dx11 = this->lerp(fx, m011, m111);
    dxy0 = this->lerp(fy, dx00, dx10);
    dxy1 = this->lerp(fy, dx01, dx11);
    return this->lerp(fz, dxy0, dxy1);
  }

  RealType GetMaskAndGradient(RealType *mask_gradient)
  {
    // Interpolate the mask
    RealType dx00, dx01, dx10, dx11, dxy0, dxy1;
    dx00 = this->lerp(fx, m000, m100);
    dx01 = this->lerp(fx, m001, m101);
    dx10 = this->lerp(fx, m010, m110);
    dx11 = this->lerp(fx, m011, m111);
    dxy0 = this->lerp(fy, dx00, dx10);
    dxy1 = this->lerp(fy, dx01, dx11);
    RealType mask = this->lerp(fz, dxy0, dxy1);

    // Compute the gradient of the mask
    RealType dx00_x, dx01_x, dx10_x, dx11_x, dxy0_x, dxy1_x;
    dx00_x = m100 - m000;
    dx01_x = m101 - m001;
    dx10_x = m110 - m010;
    dx11_x = m111 - m011;
    dxy0_x = this->lerp(fy, dx00_x, dx10_x);
    dxy1_x = this->lerp(fy, dx01_x, dx11_x);
    mask_gradient[0] = this->lerp(fz, dxy0_x, dxy1_x);

    RealType dxy0_y, dxy1_y;
    dxy0_y = dx10 - dx00;
    dxy1_y = dx11 - dx01;
    mask_gradient[1] = this->lerp(fz, dxy0_y, dxy1_y);

    mask_gradient[2] = dxy1 - dxy0;

    return mask;
  }

protected:

  inline const InputComponentType *border_check(int X, int Y, int Z, RealType &mask)
  {
    if(X >= 0 && X < xsize && Y >= 0 && Y < ysize && Z >= 0 && Z < zsize)
      {
      mask = 1.0;
      return dens(X,Y,Z);
      }
    else
      {
      mask = 0.0;
      return this->def_value;
      }
   }

  inline const InputComponentType *dens(int X, int Y, int Z)
  {
    return this->buffer + this->nComp * (X+xsize*(Y+ysize*Z));
  }

  // Image size
  int xsize, ysize, zsize;

  // State of current interpolation
  const InputComponentType *d000, *d001, *d010, *d011, *d100, *d101, *d110, *d111;
  RealType m000, m001, m010, m011, m100, m101, m110, m111;

  RealType fx, fy, fz;
  int	 x0, y0, z0, x1, y1, z1;

};



/**
 * 2D fast linear interpolator - optimized for speed
 */
template <class TImage, class TFloat>
class FastLinearInterpolator<TImage, TFloat, 2>
    : public FastLinearInterpolatorBase<TImage, TFloat, 2>
{
public:
  typedef TImage                                             ImageType;
  typedef FastLinearInterpolatorBase<ImageType, TFloat, 2>   Superclass;
  typedef typename Superclass::InputComponentType            InputComponentType;
  typedef typename Superclass::OutputComponentType           OutputComponentType;
  typedef typename Superclass::RealType                      RealType;
  typedef typename Superclass::InOut                         InOut;

  FastLinearInterpolator(ImageType *image) : Superclass(image)
  {
    xsize = image->GetLargestPossibleRegion().GetSize()[0];
    ysize = image->GetLargestPossibleRegion().GetSize()[1];
  }

  /**
   * Compute the pointers to the eight corners of the interpolating cube
   */
  InOut ComputeCorners(RealType *cix)
  {
    const InputComponentType *dp;

    x0 = (int) floor(cix[0]); fx = cix[0] - x0;
    y0 = (int) floor(cix[1]); fy = cix[1] - y0;

    x1 = x0 + 1;
    y1 = y0 + 1;

    if (x0 >= 0 && x1 < xsize &&
        y0 >= 0 && y1 < ysize)
      {
      // The sample point is completely inside
      dp = dens(x0, y0);
      d00 = dp;
      d10 = dp+this->nComp;
      dp += xsize*this->nComp;
      d01 = dp;
      d11 = dp+this->nComp;

      // The mask is one
      this->status = Superclass::INSIDE;
      }
    else if (x0 >= -1 && x1 <= xsize &&
             y0 >= -1 && y1 <= ysize)
      {
      // The sample point is on the border region
      d00 = border_check(x0, y0, m00);
      d01 = border_check(x0, y1, m01);
      d10 = border_check(x1, y0, m10);
      d11 = border_check(x1, y1, m11);

      // The mask is between 0 and 1
      this->status = Superclass::BORDER;
      }
    else
      {
      // The mask is zero
      this->status = Superclass::OUTSIDE;
      }

    return this->status;
  }

  /**
   * Interpolate at position cix, placing the intensity values in out and gradient
   * values in grad (in strides of VDim)
   */
  InOut InterpolateWithGradient(RealType *cix, OutputComponentType *out, OutputComponentType **grad)
  {
    RealType dx0, dx1;

    // Compute the corners
    this->ComputeCorners(cix);

    if(this->status != Superclass::OUTSIDE)
      {
      // Loop over the components
      for(int iComp = 0; iComp < this->nComp; iComp++, grad++,
          d00++, d01++, d10++, d11++)
        {
        // Interpolate the image intensity
        dx0 = Superclass::lerp(fx, *d00, *d10);
        dx1 = Superclass::lerp(fx, *d01, *d11);
        *(out++) = Superclass::lerp(fy, dx0, dx1);

        // Interpolate the gradient in x
        (*grad)[0] = this->lerp(fy, *d10 - *d00, *d11 - *d01);

        // Interpolate the gradient in y
        (*grad)[1] = dx1 - dx0;
        }
      }

    return this->status;
  }

  InOut Interpolate(RealType *cix, OutputComponentType *out)
  {
    OutputComponentType dx0, dx1;

    // Compute the corners
    this->ComputeCorners(cix);

    if(this->status != Superclass::OUTSIDE)
      {
      // Loop over the components
      for(int iComp = 0; iComp < this->nComp; iComp++,
          d00++, d01++, d10++, d11++)
        {
        // Interpolate the image intensity
        dx0 = Superclass::lerp(fx, *d00, *d10);
        dx1 = Superclass::lerp(fx, *d01, *d11);
        *(out++) = Superclass::lerp(fy, dx0, dx1);
        }
      }

    return this->status;
  }

  InOut InterpolateNearestNeighbor(RealType *cix, OutputComponentType *out)
  {
    x0 = (int) floor(cix[0] + 0.5);
    y0 = (int) floor(cix[1] + 0.5);

    if (x0 >= 0 && x0 < xsize && y0 >= 0 && y0 < ysize)
      {
      const InputComponentType *dp = dens(x0, y0);
      for(int iComp = 0; iComp < this->nComp; iComp++)
        {
        out[iComp] = dp[iComp];
        }
      return Superclass::INSIDE;
      }
    else return Superclass::OUTSIDE;
  }


  template <class THistContainer>
  void PartialVolumeHistogramSample(RealType *cix, const InputComponentType *fixptr, THistContainer &hist)
  {
    // Compute the corners
    this->ComputeCorners(cix);

    if(this->status != Superclass::OUTSIDE)
      {
      // Compute the corner weights using 4 multiplications (not 16)
      RealType fxy = fx * fy;

      RealType w11 = fxy;
      RealType w01 = fy - fxy;
      RealType w10 = fx - fxy;
      RealType w00 = 1.0 - fx - fy + fxy;

      // Loop over the components
      for(int iComp = 0; iComp < this->nComp; iComp++,
          d00++, d01++, d10++, d11++, fixptr++)
        {
        // Just this line in the histogram
        RealType *hist_line = hist[iComp][*fixptr];

        // Assign the appropriate weight to each part of the histogram
        hist_line[*d00] += w00;
        hist_line[*d01] += w01;
        hist_line[*d10] += w10;
        hist_line[*d11] += w11;
        }
      }
    else
      {
      for(int iComp = 0; iComp < this->nComp; iComp++, fixptr++)
        {
        // Just this line in the histogram
        RealType *hist_line = hist[iComp][*fixptr];
        hist_line[0] += 1.0;
        }
      }
  }

  template <class THistContainer>
  void PartialVolumeHistogramGradientSample(RealType *cix, const InputComponentType *fixptr, const THistContainer &hist_w, RealType *out_grad)
  {
    // Compute the corners
    this->ComputeCorners(cix);

    // Outside values do not contribute to the gradient
    if(this->status != Superclass::OUTSIDE)
      {
      // Some horrendous derivatives here! Wow!
      RealType w11x = fy,               w11y = fx;
      RealType w01x = -fy,              w01y = 1 - fx;
      RealType w10x = 1 - fy,           w10y = -fx;
      RealType w00x = fy - 1,           w00y = fx - 1;

      // Initialize gradient to zero
      out_grad[0] = 0.0;
      out_grad[1] = 0.0;

      // Loop over the components
      for(int iComp = 0; iComp < this->nComp; iComp++,
          d00++, d01++, d10++, d11++, fixptr++)
        {
        // Just this line in the histogram
        const RealType *f = hist_w[iComp][*fixptr];

        // Take the weighted sum
        RealType f00 = f[*d00], f01 = f[*d01];
        RealType f10 = f[*d10], f11 = f[*d11];

        out_grad[0] += w00x * f00 + w01x * f01 + w10x * f10 + w11x * f11;
        out_grad[1] += w00y * f00 + w01y * f01 + w10y * f10 + w11y * f11;
        }
      }
    else
      {
      out_grad[0] = 0.0;
      out_grad[1] = 0.0;
      }
  }

  RealType GetMask()
  {
    // Interpolate the mask
    RealType dx0, dx1;
    dx0 = this->lerp(fx, m00, m10);
    dx1 = this->lerp(fx, m01, m11);
    return this->lerp(fy, dx0, dx1);
  }

  RealType GetMaskAndGradient(RealType *mask_gradient)
  {
    // Interpolate the mask
    RealType dx0, dx1;
    dx0 = this->lerp(fx, m00, m10);
    dx1 = this->lerp(fx, m01, m11);
    RealType mask = this->lerp(fy, dx0, dx1);

    // Compute the gradient of the mask
    mask_gradient[0] = this->lerp(fy, m10 - m00, m11 - m01);
    mask_gradient[1] = dx1 - dx0;

    return mask;
  }

protected:

  inline const InputComponentType *border_check(int X, int Y, RealType &mask)
  {
    if(X >= 0 && X < xsize && Y >= 0 && Y < ysize)
      {
      mask = 1.0;
      return dens(X,Y);
      }
    else
      {
      mask = 0.0;
      return this->def_value;
      }
   }

  inline const InputComponentType *dens(int X, int Y)
  {
    return this->buffer + this->nComp * (X+xsize*Y);
  }

  // Image size
  int xsize, ysize;

  // State of current interpolation
  const InputComponentType *d00, *d01, *d10, *d11;
  RealType m00, m01, m10, m11;

  RealType fx, fy;
  int	 x0, y0, x1, y1;
};


#endif
