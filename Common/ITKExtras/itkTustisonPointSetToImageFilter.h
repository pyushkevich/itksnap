/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: itkTustisonPointSetToImageFilter.h,v $
  Language:  C++
  Date:      $Date: 2007/12/30 04:05:12 $
  Version:   $Revision: 1.2 $

  Copyright (c) Insight Software Consortium. All rights reserved.
  
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
#ifndef __itkTustisonPointSetToImageFilter_h
#define __itkTustisonPointSetToImageFilter_h

#include "itkImageSource.h"
#include "itkConceptChecking.h"

namespace itk
{
  
/** \class TustisonPointSetToImageFilter
 * \brief Base class for filters that take a PointSet 
 *        as input and produce an image as output.
 *  By default, if the user does not specify the size of the output image,
 *  the maximum size of the point-set's bounding box is used. 
 */
template <class TInputPointSet, class TOutputImage>
class ITK_EXPORT TustisonPointSetToImageFilter : public ImageSource<TOutputImage>
{
public:
  /** Standard class typedefs. */
  typedef TustisonPointSetToImageFilter                Self;
  typedef ImageSource<TOutputImage>            Superclass;
  typedef SmartPointer<Self>                   Pointer;
  typedef SmartPointer<const Self>             ConstPointer;
  typedef typename TOutputImage::SizeType      SizeType;
  typedef TOutputImage                         OutputImageType;
  typedef typename OutputImageType::Pointer    OutputImagePointer;
  typedef typename OutputImageType::ValueType  ValueType;
  
  /** Method for creation through the object factory. */
  itkNewMacro(Self);
  
  /** Run-time type information (and related methods). */
  itkTypeMacro(TustisonPointSetToImageFilter,ImageSource);

  /** Superclass typedefs. */
  typedef typename Superclass::OutputImageRegionType OutputImageRegionType;

  /** Some convenient typedefs. */
  typedef TInputPointSet InputPointSetType;
  typedef typename InputPointSetType::Pointer        InputPointSetPointer;
  typedef typename InputPointSetType::ConstPointer   InputPointSetConstPointer;

  /** Dimension constants */
  itkStaticConstMacro(InputPointSetDimension, unsigned int,
                      InputPointSetType::PointDimension);
  itkStaticConstMacro(OutputImageDimension, unsigned int,
                      TOutputImage::ImageDimension);

  /** Image spacing and origin typedefs */
  typedef typename TOutputImage::SpacingType SpacingType;
  typedef typename TOutputImage::PointType   PointType;

  /** Set/Get the input point-set of this process object.  */
  virtual void SetInput( const InputPointSetType *pointset);
  virtual void SetInput( unsigned int, const InputPointSetType * pointset);
  const InputPointSetType * GetInput(void);
  const InputPointSetType * GetInput(unsigned int idx);

  /** Set the spacing (size of a pixel) of the image. The
   * spacing is the geometric distance between image samples.
   * It is stored internally as double, but may be set from
   * float. \sa GetSpacing() */
  itkSetMacro(Spacing,SpacingType);
  virtual void SetSpacing( const double* spacing);
  virtual void SetSpacing( const float* spacing);

  /** Get the spacing (size of a pixel) of the image. The
   * spacing is the geometric distance between image samples.
   * The value returned is a pointer to a double array.
   * For ImageBase and Image, the default data spacing is unity. */
  itkGetConstReferenceMacro(Spacing,SpacingType);

  /** Set the origin of the image. The origin is the geometric
   * coordinates of the image origin.  It is stored internally
   * as double but may be set from float.
   * \sa GetOrigin() */
  itkSetMacro(Origin,PointType);
  virtual void SetOrigin( const double* origin);
  virtual void SetOrigin( const float* origin);
 
 /** Get the origin of the image. The origin is the geometric
   * coordinates of the index (0,0).  The value returned is a pointer
   * to a double array.  For ImageBase and Image, the default origin is 
   * 0. */
  itkGetConstReferenceMacro(Origin,PointType);

  /** Set/Get the value for pixels in the point-set. 
  * By default, this filter will return an image
  * that contains values from the point-set specified as input. 
  * If this "inside" value is changed to a non-null value,
  * the output produced by this filter will be a mask with inside/outside values 
  * specified by the user. */
  itkSetMacro(InsideValue, ValueType);
  itkGetMacro(InsideValue, ValueType);

  /** Set/Get the value for pixels outside the point-set.
  * By default, this filter will return an image
  * that contains values from the point specified as input. 
  * If this "outside" value is changed to a non-null value,
  * the output produced by this filter will be a mask with inside/outside values
  * specified by the user. */
  itkSetMacro(OutsideValue, ValueType);
  itkGetMacro(OutsideValue, ValueType);

  /** Set/Get Size */
  itkSetMacro(Size,SizeType);
  itkGetMacro(Size,SizeType);

protected:
  TustisonPointSetToImageFilter();
  ~TustisonPointSetToImageFilter();

  virtual void GenerateOutputInformation(){}; // do nothing
  virtual void GenerateData();

  SizeType     m_Size;
  SpacingType  m_Spacing;
  PointType    m_Origin;
  ValueType    m_InsideValue;
  ValueType    m_OutsideValue;

  virtual void PrintSelf(std::ostream& os, Indent indent) const;

private:
  TustisonPointSetToImageFilter(const Self&); //purposely not implemented
  void operator=(const Self&); //purposely not implemented


};

} // end namespace itk

#ifndef ITK_MANUAL_INSTANTIATION
#include "itkTustisonPointSetToImageFilter.txx"
#endif

#endif
