/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: ImageCollectionToImageFilter.h,v $
  Language:  C++
  Date:      $Date: 2009/01/24 01:50:21 $
  Version:   $Revision: 1.4 $
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

  -----

  Copyright (c) 2003 Insight Software Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notices for more information.

=========================================================================*/
#ifndef __ImageCollectionToImageFilter_h_
#define __ImageCollectionToImageFilter_h_

#include <itkImageSource.h>
#include <itkImageToImageFilter.h>

#include <itkImageRegionIterator.h>
#include <itkConstNeighborhoodIterator.h>
#include "ImageRegionConstIteratorWithIndexOverride.h"

/**
 * This class is derived from the itk::iterator hierarchy and used to gain
 * access to the protected m_Offset property
 */
template <class TIterator>
class IteratorOffsetAccessor : public TIterator
{
public:
  typedef typename TIterator::ImageType ImageType;
  typedef typename TIterator::RegionType RegionType;
  typedef itk::OffsetValueType OffsetValueType;

  IteratorOffsetAccessor() { }
  IteratorOffsetAccessor(const IteratorOffsetAccessor<TIterator> &copy)
    : TIterator(copy) {}
  IteratorOffsetAccessor(ImageType *image, const RegionType &region)
    : TIterator(image, region) {}

  OffsetValueType GetOffset() const { return this->m_Offset; }
};

/**
 * This class represents an offset table for sampling patches from an image
 * or vector image. Sampling a patch means taking continuous strides of memory
 * from the image. The patch is a list of pairs, each representing a start of
 * a stride and end of a stride; specified as offsets relative to the memory
 * location at the center of the patch
 */
template <unsigned int VDim>
class PatchOffsetTableGenerator
{
public:
  /** A stride in the offset table */
  struct Stride { int start_offset, end_offset; };

  /** The table is a list of strides */
  typedef std::vector<Stride> Table;
  
  /** Computation of the offset table */
  template <class TImage>
  static Table Compute(TImage *image, const itk::Size<VDim> &radius)
    {
    // Create an iterator over the output image
    typedef itk::ImageLinearIteratorWithIndex<TImage> IterBase;
    typedef IteratorExtender<IterBase> IterType;
    Table offset_table;

    // Create the region
    itk::ImageRegion<3> region;
    itk::Index<3> center;
    for(unsigned int i = 0; i < 3; i++)
      {
      region.SetIndex(i, 0);
      region.SetSize(i, 2*radius[i]+1);
      center[i] = radius[i];
      }

    // Get the offset of the center
    IterType it(image, region);
    it.SetIndex(center);
    const auto *p_center = it.GetPixelPointer(image);

    // Iterate over lines in the region
    for(it.GoToBegin(); !it.IsAtEnd(); it.NextLine())
      {
      // The offset at the beginning of the line
      int offset_in_bytes_begin = (int)(it.GetPixelPointer(image) - p_center);

      // The offset at the end of line
      it.GoToEndOfLine();
      int offset_in_bytes_end = (int)(it.GetPixelPointer(image) - p_center);

      // Representation of the line
      Stride stride = { offset_in_bytes_begin, offset_in_bytes_end };
      offset_table.push_back(stride);
      }

    return offset_table;
    }
};

/**
 * An iterator that can be used to iterate over a collection of scalar and
 * vector images of the same type.
 *
 * Example usage:
 *
 * ImageCollectionConstRegionIteratorWithIndex it(out.GetBufferedRegion());
 * it.AddImage(img1);
 * it.AddImage(img2);
 * it.AddImage(img3);
 *
 * int nComp = it.GetTotalComponents();
 * for (; !it.IsAtEnd(); ++it)
 * {
 *   for(int j = 0; j < nComp; j++)
 *   {
 *     GreyType &val = it.Value(j);
 *   }
 * }
 */
template <class TScalarImage, class TVectorImage>
class ImageCollectionConstIteratorWithIndex
{
public:
  /** Standard class typedefs. */
  typedef ImageCollectionConstIteratorWithIndex Self;

  /** Dimension of the image that the iterator walks.  This constant is needed so
   * functions that are templated over image iterator type (as opposed to
   * being templated over pixel type and dimension) can have compile time
   * access to the dimension of the image that the iterator walks. */
  itkStaticConstMacro(ImageDimension, unsigned int, TScalarImage::ImageDimension);

  /** Index typedef support. */
  typedef typename TScalarImage::IndexType         IndexType;
  typedef typename IndexType::IndexValueType IndexValueType;

  /** Size typedef support. */
  typedef typename TScalarImage::SizeType        SizeType;
  typedef typename SizeType::SizeValueType SizeValueType;

  /** Region typedef support. */
  typedef typename TScalarImage::RegionType RegionType;

  /** Image base type */
  typedef itk::ImageBase<ImageDimension> ImageBaseType;

  /** PixelContainer typedef support. Used to refer to the container for
   * the pixel data. While this was already typdef'ed in the superclass,
   * it needs to be redone here for this subclass to compile properly with gcc. */
  typedef typename TScalarImage::PixelContainer  PixelContainer;
  typedef typename PixelContainer::Pointer PixelContainerPointer;

  typedef typename TVectorImage::PixelContainer VectorPixelContainer;
  typedef typename VectorPixelContainer::Pointer VectorPixelContainerPointer;

  /** Internal Pixel Type */
  typedef typename TScalarImage::InternalPixelType InternalPixelType;
  typedef typename TVectorImage::InternalPixelType VectorInternalPixelType;

  /** Offsets */
  typedef typename TScalarImage::OffsetType OffsetType;
  typedef typename TScalarImage::OffsetValueType OffsetValueType;

  /** Constructor is passed a region */
  ImageCollectionConstIteratorWithIndex(const SizeType radius, const RegionType &region)
    : m_Radius(radius), m_Region(region) {}

  /** Add a scalar image to the collection */
  void AddImage(TScalarImage *image)
    {
    typename ScalarImageData::Iterator iter(m_Radius, image, m_Region);
    m_NeighborhoodSize = iter.Size();
    std::vector<int> dest_offset; 
    dest_offset.push_back(m_NeighborhoodSize * m_TotalComponents);
    m_TotalComponents++;
    m_ScalarImageData.push_back({iter, dest_offset});
    }

  /** Add a vector image to the collection */
  void AddImage(TVectorImage *image)
    {
    typename VectorImageData::Iterator iter(m_Radius, image, m_Region);
    m_NeighborhoodSize = iter.Size();
    std::vector<int> dest_offset; 
    for(int i = 0; i < image->GetNumberOfComponents(); i++)
      {
      dest_offset.push_back(m_NeighborhoodSize * m_TotalComponents);
      m_TotalComponents++;
      }
    m_VectorImageData.push_back({iter, dest_offset});
    }

  /** Get the number of components across the collection */
  itkGetMacro(TotalComponents, unsigned int)

  /** Get the optional neighborhood size */
  itkGetMacro(NeighborhoodSize, int)

  /** Standard iterator operations */
  bool IsAtEnd() const
    {
    if(m_ScalarImageData.size())
      return m_ScalarImageData.front().iter.IsAtEnd();
    if(m_VectorImageData.size())
      return m_VectorImageData.front().iter.IsAtEnd();
    return false;
    }

  bool IsAtBegin() const
    {
    if(m_ScalarImageData.size())
      return m_ScalarImageData.front().iter.IsAtBegin();
    if(m_VectorImageData.size())
      return m_VectorImageData.front().iter.IsAtBegin();
    return false;
    }

  Self & operator++()
    {
    for(auto &sid : m_ScalarImageData) ++sid.iter;
    for(auto &vid : m_VectorImageData) ++vid.iter;
    }

  Self & operator--()
    {
    for(auto &sid : m_ScalarImageData) --sid.iter;
    for(auto &vid : m_VectorImageData) --vid.iter;
    }

  void GoToBegin()
    {
    for(auto &sid : m_ScalarImageData) sid.iter.GoToBegin();
    for(auto &vid : m_VectorImageData) vid.iter.GoToBegin();
    }

  void GoToEnd()
    {
    for(auto &sid : m_ScalarImageData) sid.iter.GoToEnd();
    for(auto &vid : m_VectorImageData) vid.iter.GoToEnd();
    }
  
  /** 
   * Sample the neighborhood, placing the values into a flat array of the size
   * TotalComponents * NeighborhoodSize, organized as
   * [component 1 patch][component 2 patch]...[component k patch]
   */
  void GetNeighborhoodValues(InternalPixelType *p)
    {
    for(auto &sid : m_ScalarImageData)
      {
      InternalPixelType q = p + sid.dest_offset[0];
      for(unsigned int i = 0; i < sid.iter.Size(); i++)
        q[i] = sid.iter.GetPixel(i);
      }
    for(auto &vid : m_VectorImageData)
      {
      for(unsigned int i = 0; i < vid.iter.Size(); i++)
        {
        const auto &pixel = vid.iter.GetPixel(i);
        for(unsigned int k = 0; k < vid.dest_offset.size(); k++)
          p[vid.dest_offset[k]+i] = pixel[k];
        }
      }
    }
  
protected:
  
  // Stuff we store for a single image
  template <class T> class ImageData
  {
  public:
    typedef itk::ConstNeighborhoodIterator<T> Iterator;
    Iterator iter;
    std::vector<int> dest_offset;
  };

  typedef typename Self::ImageData<TScalarImage> ScalarImageData;
  typedef typename Self::ImageData<TVectorImage> VectorImageData;

  std::vector<ScalarImageData> m_ScalarImageData;
  std::vector<VectorImageData> m_VectorImageData;

  // Total number of components that have been added
  unsigned int m_TotalComponents;

  // The offset for current iteration
  RegionType m_Region;

  // Radius for the optional neighborhood support
  SizeType m_Radius;

  // Size of the neighborhood
  int m_NeighborhoodSize;
};


#endif
