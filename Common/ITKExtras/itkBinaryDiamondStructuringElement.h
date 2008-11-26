/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    $RCSfile: itkBinaryDiamondStructuringElement.h,v $
  Language:  C++
  Date:      $Date: 2008/11/26 03:10:15 $
  Version:   $Revision: 1.1 $

  Copyright (c) Insight Software Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notices for more information.

=========================================================================*/
#ifndef __itkBinaryDiamondStructuringElement_h
#define __itkBinaryDiamondStructuringElement_h

#include "itkNeighborhood.h"

namespace itk {
  
/** \class BinaryDiamondStructuringElement
 * \brief A Neighborhood that represents a box structuring element 
 *        with binary elements.
 *
 *
 * \sa Neighborhood
 * \sa MorphologyImageFilter
 * \sa BinaryDilateImageFilter
 * \sa BinaryErodeImageFilter
 * 
 * \ingroup Operators
 * \ingroup ImageIterators
 */

template<class TPixel, unsigned int VDimension = 2,
         class TAllocator = NeighborhoodAllocator<TPixel> >
class ITK_EXPORT BinaryDiamondStructuringElement
  : public Neighborhood<TPixel, VDimension, TAllocator>
{
public:
  /** Standard class typedefs. */
  typedef BinaryDiamondStructuringElement                Self;
  typedef Neighborhood<TPixel, VDimension, TAllocator>   Superclass;

  /** External support for allocator type. */
  typedef TAllocator AllocatorType;

  /** External support for dimensionality. */
  itkStaticConstMacro(NeighborhoodDimension, unsigned int, VDimension);
  
  /** External support for pixel type. */
  typedef TPixel PixelType;
  
  /** Iterator typedef support. Note the naming is intentional, i.e.,
  * ::iterator and ::const_iterator, because the allocator may be a
  * vnl object or other type, which uses this form. */
  typedef typename AllocatorType::iterator       Iterator;
  typedef typename AllocatorType::const_iterator ConstIterator;
  
  /** Size and value typedef support. */
  typedef typename Superclass::SizeType      SizeType;
  typedef typename Superclass::SizeValueType SizeValueType;

  /** Offset and value typedef support. */
  typedef typename Superclass::OffsetType      OffsetType;
  typedef typename OffsetType::OffsetValueType OffsetValueType;
  
  /** Radius typedef support. */
  typedef typename Superclass::RadiusType RadiusType;

  /** External slice iterator type typedef support. */
  typedef SliceIterator<TPixel, Self> SliceIteratorType;
  
  /** Default constructor. */
  BinaryDiamondStructuringElement() {}

  /** Default destructor. */
  virtual ~BinaryDiamondStructuringElement() {}
    
  /** Copy constructor. */
  BinaryDiamondStructuringElement(const Self& other)
    : Neighborhood<TPixel, VDimension, TAllocator>(other)
    {
    }

  /** Assignment operator. */
  Self &operator=(const Self& other)
    {
    Superclass::operator=(other);
    return *this;
    }

  /** Build the structuring element */
  void CreateStructuringElement();   
  
protected:
  
private:

};

} // namespace itk

// Define instantiation macro for this template.
#define ITK_TEMPLATE_BinaryDiamondStructuringElement(_, EXPORT, x, y) namespace itk { \
  _(2(class EXPORT BinaryDiamondStructuringElement< ITK_TEMPLATE_2 x >)) \
  namespace Templates { typedef BinaryDiamondStructuringElement< ITK_TEMPLATE_2 x > \
                                                  BinaryDiamondStructuringElement##y; } \
  }

#if ITK_TEMPLATE_EXPLICIT
# include "Templates/itkBinaryDiamondStructuringElement+-.h"
#endif

#if ITK_TEMPLATE_TXX
# include "itkBinaryDiamondStructuringElement.txx"
#endif

#endif
