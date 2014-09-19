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

#include <itkImageRegionIteratorWithIndex.h>

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
template <class TImage, class TVectorImage>
class ImageCollectionConstRegionIteratorWithIndex
{
public:
  /** Standard class typedefs. */
  typedef ImageCollectionConstRegionIteratorWithIndex Self;

  /** Dimension of the image that the iterator walks.  This constant is needed so
   * functions that are templated over image iterator type (as opposed to
   * being templated over pixel type and dimension) can have compile time
   * access to the dimension of the image that the iterator walks. */
  itkStaticConstMacro(ImageDimension, unsigned int, TImage::ImageDimension);

  /** Index typedef support. */
  typedef typename TImage::IndexType         IndexType;
  typedef typename IndexType::IndexValueType IndexValueType;

  /** Size typedef support. */
  typedef typename TImage::SizeType        SizeType;
  typedef typename SizeType::SizeValueType SizeValueType;

  /** Region typedef support. */
  typedef typename TImage::RegionType RegionType;

  /** Image typedef support. */
  typedef TImage ImageType;

  /** Image base type */
  typedef itk::ImageBase<ImageDimension> ImageBaseType;

  /** PixelContainer typedef support. Used to refer to the container for
   * the pixel data. While this was already typdef'ed in the superclass,
   * it needs to be redone here for this subclass to compile properly with gcc. */
  typedef typename TImage::PixelContainer  PixelContainer;
  typedef typename PixelContainer::Pointer PixelContainerPointer;

  typedef typename TVectorImage::PixelContainer VectorPixelContainer;
  typedef typename VectorPixelContainer::Pointer VectorPixelContainerPointer;

  /** Internal Pixel Type */
  typedef typename TImage::InternalPixelType InternalPixelType;
  typedef typename TVectorImage::InternalPixelType VectorInternalPixelType;

  /** Offsets */
  typedef typename TImage::OffsetType OffsetType;
  typedef typename TImage::OffsetValueType OffsetValueType;

  /** Constructor is passed a region */
  ImageCollectionConstRegionIteratorWithIndex(const RegionType &region);

  /** Add a scalar image to the collection */
  void AddScalarImage(TImage *image);

  /** Add a vector image to the collection */
  void AddVectorImage(TVectorImage *image);

  /** Add an image that must be dynamically castable to either TImage or TVectorImage */
  void AddImage(itk::DataObject *image);

  /** Get the number of components across the collection */
  itkGetMacro(TotalComponents, unsigned int)

  /** Standard iterator operations */
  bool IsAtEnd() const { return m_InternalIter.IsAtEnd(); }
  bool IsAtBegin() const { return m_InternalIter.IsAtBegin(); }
  Self & operator++() { ++m_InternalIter; return *this; }
  Self & operator--() { --m_InternalIter; return *this; }
  void GoToBegin() { m_InternalIter.GoToBegin(); }
  void GoToEnd() { m_InternalIter.GoToEnd(); }

  /** Get a pointer to a component */
  InternalPixelType &Value(unsigned int comp)
  {
    OffsetValueType offset = m_InternalIter.GetOffset();
    InternalPixelType *dataPtr = m_Start[comp] + offset * m_OffsetScaling[comp];
    return *(dataPtr);
  }

protected:

  // Collection of scalar images
  std::vector<const TImage *> m_ScalarImages;

  // Collection of vector images
  std::vector<const TVectorImage *> m_VectorImages;

  // An array of start pointers for each component
  std::vector<InternalPixelType *> m_Start;

  // The scaling of offsets for each component
  std::vector<int> m_OffsetScaling;

  // A dummy image used to create an internal iterator. Instead of replicating
  // all of the iterator mechanics code in this class, we defer to the internal
  // iterator for all of that.
  typename ImageType::Pointer m_DummyImage;

  // Internal iterator
  typedef itk::ImageRegionIterator<ImageType> WrappedIteratorType;
  typedef IteratorOffsetAccessor<WrappedIteratorType> InternalIteratorType;
  InternalIteratorType m_InternalIter;

  // Total number of components that have been added
  unsigned int m_TotalComponents;

  // The offset for current iteration
  RegionType m_Region;

  // This method initializes all the iteration mechanics
  void ComputeMechanics(ImageBaseType *image);
};


template <class TImage, class TVectorImage>
ImageCollectionConstRegionIteratorWithIndex<TImage, TVectorImage>
::ImageCollectionConstRegionIteratorWithIndex(const RegionType &region)
{
  m_Region = region;
  m_TotalComponents = 0;
}

template <class TImage, class TVectorImage>
void
ImageCollectionConstRegionIteratorWithIndex<TImage, TVectorImage>
::AddScalarImage(TImage *image)
{
  // Initialize the iteration mechanics
  if(m_TotalComponents == 0)
    ComputeMechanics(image);
  else
    assert(m_DummyImage->GetBufferedRegion() == image->GetBufferedRegion());

  // Add the image to the list
  m_ScalarImages.push_back(image);
  m_TotalComponents++;

  m_Start.push_back(image->GetBufferPointer());
  m_OffsetScaling.push_back(1);
}

template <class TImage, class TVectorImage>
void
ImageCollectionConstRegionIteratorWithIndex<TImage, TVectorImage>
::AddVectorImage(TVectorImage *image)
{
  // Initialize the iteration mechanics
  if(m_TotalComponents == 0)
    ComputeMechanics(image);
  else
    assert(m_DummyImage->GetBufferedRegion() == image->GetBufferedRegion());

  m_VectorImages.push_back(image);
  m_TotalComponents += image->GetVectorLength();

  for(int i = 0; i < image->GetVectorLength(); i++)
    {
    m_Start.push_back(image->GetBufferPointer() + i);
    m_OffsetScaling.push_back(image->GetVectorLength());
    }
}

template <class TImage, class TVectorImage>
void
ImageCollectionConstRegionIteratorWithIndex<TImage, TVectorImage>
::AddImage(itk::DataObject *dobj)
{
  TImage *image = dynamic_cast<TImage *>(dobj);
  if(image)
    {
    this->AddScalarImage(image);
    }
  else
    {
    TVectorImage *vecImage = dynamic_cast<TVectorImage *>(dobj);
    if(vecImage)
      {
      this->AddVectorImage(vecImage);
      }
    else
      {
      itkAssertInDebugOrThrowInReleaseMacro(
            "Wrong input type to ImageCollectionConstRegionIteratorWithIndex");
      }
    }
}

template <class TImage, class TVectorImage>
void
ImageCollectionConstRegionIteratorWithIndex<TImage, TVectorImage>
::ComputeMechanics(ImageBaseType *image)
{
  m_DummyImage = ImageType::New();
  m_DummyImage->SetRegions(image->GetBufferedRegion());

  m_InternalIter = InternalIteratorType(m_DummyImage, m_Region);
}



/* example usage:



  */

/*
template <class TScalarImage, class TVectorImage, class TOutputImage>
class ImageCollectionToImageFilter
: public itk::ImageSource<TOutputImage>
{
public:

protected:
};

*/

#endif
