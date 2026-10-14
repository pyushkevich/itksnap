/*=========================================================================

  Program:   ITK-SNAP
  Language:  C++
  Copyright (c) 2007 Paul A. Yushkevich

  This file is part of ITK-SNAP

  ITK-SNAP is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

=========================================================================*/
#ifndef NONORTHOGONALSLICERWORKERTRAITS_TXX
#define NONORTHOGONALSLICERWORKERTRAITS_TXX

#include "NonOrthogonalSlicerWorkerTraits.h"
#include "FastLinearInterpolator.h"
#include <cmath>

/* ===========================================================================================
 * DefaultNonOrthogonalSlicerWorkerTraits traits for itk::Image or itk::VectorImage
 * ===========================================================================================  */

template <class TInputImage, class TOutputImage>
DefaultNonOrthogonalSlicerWorkerTraits<TInputImage, TOutputImage>
::DefaultNonOrthogonalSlicerWorkerTraits(TInputImage *image)
  : m_Interpolator(image)
{
  m_NumComponents = m_Interpolator.GetPointerIncrement();
  m_Buffer = new double[m_NumComponents];
}

template <class TInputImage, class TOutputImage>
DefaultNonOrthogonalSlicerWorkerTraits<TInputImage, TOutputImage>
::~DefaultNonOrthogonalSlicerWorkerTraits()
{
  delete m_Buffer;
}

template <class TInputImage, class TOutputImage>
void
DefaultNonOrthogonalSlicerWorkerTraits<TInputImage, TOutputImage>
::ProcessVoxel(double *cix, bool use_nn, OutputComponentType **out_ptr)
{
  // Perform the interpolation
  typename Interpolator::InOut status =
      use_nn
      ? m_Interpolator.InterpolateNearestNeighbor(cix,  m_Buffer)
      : m_Interpolator.Interpolate(cix, m_Buffer);

  if(status == Interpolator::INSIDE || status == Interpolator::BORDER)
    {
    for(int k = 0; k < m_NumComponents; k++)
      *(*out_ptr)++ = static_cast<OutputComponentType>(m_Buffer[k]);
    }
  else
    {
    SkipVoxels(1, out_ptr);
    }
}

template <class TInputImage, class TOutputImage>
void
DefaultNonOrthogonalSlicerWorkerTraits<TInputImage, TOutputImage>::CollectStatistics(double *cix,
                                                                                     bool use_nn,
                                                                                     double &out_nvalid,
                                                                                     double &out_sum,
                                                                                     double &out_sumsq)
{
  // Perform the interpolation
  typename Interpolator::InOut status = use_nn
                                          ? m_Interpolator.InterpolateNearestNeighbor(cix, m_Buffer)
                                          : m_Interpolator.Interpolate(cix, m_Buffer);

  if (status == Interpolator::INSIDE)
  {
    for (int k = 0; k < m_NumComponents; k++)
    {
      double value = m_Buffer[k];
      if (std::isnan(value))
        continue;
      out_nvalid += 1.0;
      out_sum += value;
      out_sumsq += value * value;
    }
  }
  else if (status == Interpolator::BORDER)
  {
    // On the border, value is the weighted sum over the voxels inside the
    // image and mask is the sum of their weights, so the sample counts with
    // weight mask at the value value / mask
    double mask = m_Interpolator.GetMask();
    if (mask <= 0.0)
      return;
    for (int k = 0; k < m_NumComponents; k++)
    {
      double value = m_Buffer[k];
      if (std::isnan(value))
        continue;
      out_nvalid += mask;
      out_sum += value;
      out_sumsq += value * value / mask;
    }
  }
}

template <class TInputImage, class TOutputImage>
void
DefaultNonOrthogonalSlicerWorkerTraits<TInputImage, TOutputImage>
::SkipVoxels(int n, OutputComponentType **out_ptr)
{
  // TODO: this is problematic, why are we using zero? !!!!
  int n_total = n * m_NumComponents;
  for(int k = 0; k < n_total; k++)
    *(*out_ptr)++ = 0; //itk::NumericTraits<OutputComponentType>::Zero;
}

/* ===========================================================================================
 * DefaultNonOrthogonalSlicerWorkerTraits traits for itk::VectorImageToImageAdaptor
 * ===========================================================================================  */

/*
 * Traits for the component extracting image adaptor. Note that in the call to the
 * constructor for the interpolator, we are passing the buffer pointer offset by
 * the component index, and only requesting a single component to be sampled
 */
template <typename TPixelType, unsigned int Dimension, typename TOutputImage>
DefaultNonOrthogonalSlicerWorkerTraits<
  itk::VectorImageToImageAdaptor<TPixelType, Dimension>,
  TOutputImage>
::DefaultNonOrthogonalSlicerWorkerTraits(AdaptorType *adaptor)
  : m_Interpolator(adaptor,
                   adaptor->GetBufferPointer() + adaptor->GetPixelAccessor().GetExtractComponentIdx(),
                   adaptor->GetPixelAccessor().GetVectorLength(), 1)
{
  m_NumComponents = m_Interpolator.GetPointerIncrement();
  m_ExtractComponent = adaptor->GetPixelAccessor().GetExtractComponentIdx();
}

template <typename TPixelType, unsigned int Dimension, typename TOutputImage>
DefaultNonOrthogonalSlicerWorkerTraits<
  itk::VectorImageToImageAdaptor<TPixelType, Dimension>,
  TOutputImage>
::~DefaultNonOrthogonalSlicerWorkerTraits()
{
}

template <typename TPixelType, unsigned int Dimension, typename TOutputImage>
void
DefaultNonOrthogonalSlicerWorkerTraits<
  itk::VectorImageToImageAdaptor<TPixelType, Dimension>,
  TOutputImage>
::ProcessVoxel(double *cix, bool use_nn, OutputComponentType **out_ptr)
{
  // Perform the interpolation
  typename Interpolator::InOut status =
      use_nn
      ? m_Interpolator.InterpolateNearestNeighbor(cix, &m_BufferValue)
      : m_Interpolator.Interpolate(cix, &m_BufferValue);

  if(status == Interpolator::INSIDE || status == Interpolator::BORDER)
    *(*out_ptr)++ = static_cast<OutputComponentType>(m_BufferValue);
  else
    *(*out_ptr)++ = 0;
}

template <typename TPixelType, unsigned int Dimension, typename TOutputImage>
void
DefaultNonOrthogonalSlicerWorkerTraits<
  itk::VectorImageToImageAdaptor<TPixelType, Dimension>,
  TOutputImage>
::CollectStatistics(double *cix, bool use_nn, double &out_nvalid, double &out_sum, double &out_sumsq)
{
  // Perform the interpolation of the single extracted component
  typename Interpolator::InOut status =
      use_nn
      ? m_Interpolator.InterpolateNearestNeighbor(cix, &m_BufferValue)
      : m_Interpolator.Interpolate(cix, &m_BufferValue);

  // Samples outside the image do not contribute. On the border, the
  // interpolated value is the weighted sum over the voxels inside the image,
  // and the mask is the sum of their weights, so the sample counts with that
  // weight at the value m_BufferValue / mask
  double weight = (status == Interpolator::INSIDE) ? 1.0
                  : (status == Interpolator::BORDER) ? m_Interpolator.GetMask()
                  : 0.0;
  double value = m_BufferValue;
  if(weight > 0.0 && !std::isnan(value))
    {
    out_nvalid += weight;
    out_sum += value;
    out_sumsq += value * value / weight;
    }
}

template <typename TPixelType, unsigned int Dimension, typename TOutputImage>
void
DefaultNonOrthogonalSlicerWorkerTraits<
  itk::VectorImageToImageAdaptor<TPixelType, Dimension>,
  TOutputImage>
::SkipVoxels(int n, OutputComponentType **out_ptr)
{
  for(int i = 0; i < n; i++)
    *(*out_ptr)++ = 0;
}

/* ===========================================================================================
 * DefaultNonOrthogonalSlicerWorkerTraits traits for generic itk::ImageAdaptor
 * ===========================================================================================  */

template <typename TPixelType, unsigned int Dimension, typename TAccessor, typename TOutputImage>
DefaultNonOrthogonalSlicerWorkerTraits<
  itk::ImageAdaptor<itk::VectorImage<TPixelType, Dimension>, TAccessor>,
  TOutputImage>
::DefaultNonOrthogonalSlicerWorkerTraits(AdaptorType *adaptor)
  : m_Interpolator(adaptor, adaptor->GetBufferPointer(), adaptor->GetPixelAccessor().GetVectorLength()),
    m_VectorPixel(adaptor->GetPixelAccessor().GetVectorLength()),
    m_Adaptor(adaptor)
{
  m_NumComponents = m_Interpolator.GetPointerIncrement();
  m_Buffer = new double[m_NumComponents];
}

template <typename TPixelType, unsigned int Dimension, typename TAccessor, typename TOutputImage>
DefaultNonOrthogonalSlicerWorkerTraits<
  itk::ImageAdaptor<itk::VectorImage<TPixelType, Dimension>, TAccessor>,
  TOutputImage>
::~DefaultNonOrthogonalSlicerWorkerTraits()
{
  delete m_Buffer;
}

template <typename TPixelType, unsigned int Dimension, typename TAccessor, typename TOutputImage>
void
DefaultNonOrthogonalSlicerWorkerTraits<
  itk::ImageAdaptor<itk::VectorImage<TPixelType, Dimension>, TAccessor>,
  TOutputImage>
::ProcessVoxel(double *cix, bool use_nn, OutputComponentType **out_ptr)
{
  // Perform the interpolation
  typename Interpolator::InOut status =
      use_nn
      ? m_Interpolator.InterpolateNearestNeighbor(cix, m_Buffer)
      : m_Interpolator.Interpolate(cix, m_Buffer);

  const typename InternalImageType::PixelType &vpref = m_VectorPixel;

  if(status == Interpolator::INSIDE || status == Interpolator::BORDER)
    {
    // Cast to the vector pixel type
    for(int i = 0; i < m_NumComponents; i++)
      m_VectorPixel[i] = static_cast<OutputComponentType>(m_Buffer[i]);

    // Apply the accessor
    *(*out_ptr)++ =
        m_Adaptor->GetPixelAccessor().Get(m_VectorPixel.GetDataPointer());
    }
  else
    {
    *(*out_ptr)++ = 0;
    }
}

template <typename TPixelType, unsigned int Dimension, typename TAccessor, typename TOutputImage>
void
DefaultNonOrthogonalSlicerWorkerTraits<
  itk::ImageAdaptor<itk::VectorImage<TPixelType, Dimension>, TAccessor>,
  TOutputImage>
::CollectStatistics(double *cix, bool use_nn, double &out_nvalid, double &out_sum, double &out_sumsq)
{
  // Perform the interpolation of all the components
  typename Interpolator::InOut status =
      use_nn
      ? m_Interpolator.InterpolateNearestNeighbor(cix, m_Buffer)
      : m_Interpolator.Interpolate(cix, m_Buffer);

  // Samples outside the image do not contribute. On the border, the
  // interpolated components are weighted sums over the voxels inside the
  // image, with the mask being the sum of the weights
  double weight = (status == Interpolator::INSIDE) ? 1.0
                  : (status == Interpolator::BORDER) ? m_Interpolator.GetMask()
                  : 0.0;
  if(weight <= 0.0)
    return;

  // The accessor (e.g. magnitude, maximum) is not linear, so it is applied to
  // the components normalized by the weight, and the result counted with that
  // weight
  for(int i = 0; i < m_NumComponents; i++)
    m_VectorPixel[i] = static_cast<TPixelType>(m_Buffer[i] / weight);
  double value = m_Adaptor->GetPixelAccessor().Get(m_VectorPixel.GetDataPointer());

  if(!std::isnan(value))
    {
    out_nvalid += weight;
    out_sum += weight * value;
    out_sumsq += weight * value * value;
    }
}

template <typename TPixelType, unsigned int Dimension, typename TAccessor, typename TOutputImage>
void
DefaultNonOrthogonalSlicerWorkerTraits<
  itk::ImageAdaptor<itk::VectorImage<TPixelType, Dimension>, TAccessor>,
  TOutputImage>
::SkipVoxels(int n, OutputComponentType **out_ptr)
{
  for(int i = 0; i < n; i++)
    *(*out_ptr)++ = 0;
}

/* ===========================================================================================
 * DefaultNonOrthogonalSlicerWorkerTraits traits for RLEImage
 * ===========================================================================================  */

template <typename TPixel, unsigned int Dimension, typename TCounter, typename TOutputImage>
DefaultNonOrthogonalSlicerWorkerTraits<RLEImage<TPixel, Dimension, TCounter>, TOutputImage>
::DefaultNonOrthogonalSlicerWorkerTraits(InputImageType *image)
  : m_Image(image)
{
}

template <typename TPixel, unsigned int Dimension, typename TCounter, typename TOutputImage>
DefaultNonOrthogonalSlicerWorkerTraits<RLEImage<TPixel, Dimension, TCounter>, TOutputImage>
::~DefaultNonOrthogonalSlicerWorkerTraits()
{
}

template <typename TPixel, unsigned int Dimension, typename TCounter, typename TOutputImage>
void
DefaultNonOrthogonalSlicerWorkerTraits<RLEImage<TPixel, Dimension, TCounter>, TOutputImage>
::ProcessVoxel(double *cix, bool itkNotUsed(use_nn), OutputComponentType **out_ptr)
{
  // We simply have to round cix to the closest index, check if it's inside the buffer
  // and call GetPixel(). There is not a faster way. Round with floor: a cast
  // truncates toward zero, which maps the voxel before the first (-1) to 0.
  itk::Index<Dimension> idx;
  for(unsigned int k = 0; k < Dimension; k++)
    idx[k] = (int) std::floor(cix[k] + 0.5);

  if(m_Image->GetBufferedRegion().IsInside(idx))
    {
    *(*out_ptr)++ = m_Image->GetPixel(idx);
    }
  else
    {
    *(*out_ptr)++ = 0;
    }
}

template <typename TPixel, unsigned int Dimension, typename TCounter, typename TOutputImage>
void
DefaultNonOrthogonalSlicerWorkerTraits<RLEImage<TPixel, Dimension, TCounter>, TOutputImage>
::CollectStatistics(double *cix, bool itkNotUsed(use_nn),
                    double &out_nvalid, double &out_sum, double &out_sumsq)
{
  // As in ProcessVoxel, always use the nearest voxel. Samples outside the
  // image do not contribute.
  itk::Index<Dimension> idx;
  for(unsigned int k = 0; k < Dimension; k++)
    idx[k] = (int) std::floor(cix[k] + 0.5);

  if(m_Image->GetBufferedRegion().IsInside(idx))
    {
    double value = m_Image->GetPixel(idx);
    out_nvalid += 1.0;
    out_sum += value;
    out_sumsq += value * value;
    }
}

template <typename TPixel, unsigned int Dimension, typename TCounter, typename TOutputImage>
void
DefaultNonOrthogonalSlicerWorkerTraits<RLEImage<TPixel, Dimension, TCounter>, TOutputImage>
::SkipVoxels(int n, OutputComponentType **out_ptr)
{
  for(int i = 0; i < n; i++)
    *(*out_ptr)++ = 0;
}

#endif // NONORTHOGONALSLICERWORKERTRAITS_TXX
