/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    $RCSfile: SNAPLevelSetStopAndGoFilter.h,v $
  Language:  C++
  Date:      $Date: 2006/12/02 04:22:14 $
  Version:   $Revision: 1.1 $
  Copyright (c) 2003 Insight Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notices for more information.
=========================================================================*/
#ifndef __SNAPLevelSetStopAndGoFilter_h_
#define __SNAPLevelSetStopAndGoFilter_h_

#include "itkSparseFieldLevelSetImageFilter.h"

/**
 * \class SNAPLevelSetStopAndGoFilter
 * \brief An extension of the ITK SparseFieldLevelSetImageFilter that allows
 * users to execute one iteration at a time.
 *
 * This class will no longer be necessary if the functionality is added to the
 * FiniteDifferenceImageFilter class in ITK.
 */
template <class TInputImage, class TOutputImage>
class ITK_EXPORT SNAPLevelSetStopAndGoFilter
  : public itk::SparseFieldLevelSetImageFilter<TInputImage, TOutputImage>
{
public:
  /** Standard class typedefs. */
  typedef SNAPLevelSetStopAndGoFilter Self;
  typedef itk::SparseFieldLevelSetImageFilter<TInputImage, TOutputImage>
   Superclass;
  typedef itk::SmartPointer<Self> Pointer;
  typedef itk::SmartPointer<const Self> ConstPointer;

  /** Some typedefs from the parent class */
  typedef typename Superclass::TimeStepType TimeStepType;

  /** Run-time type information. */
  itkTypeMacro(SNAPLevelSetStopAndGoFilter,
               itk::SparseFieldLevelSetImageFilter);

  /** New object of this type */
  itkNewMacro(SNAPLevelSetStopAndGoFilter);

  /** Initialize the filter before calling Run */
  void Start();

  /** Method that runs the filter for a number of iterations */
  void Run(unsigned int nIterations);

  /** Override the generate data method to do nothing */
  void GenerateData();

protected:
  SNAPLevelSetStopAndGoFilter();
  ~SNAPLevelSetStopAndGoFilter() {}
  void PrintSelf(std::ostream &s, itk::Indent indent) const;
  
  /** Dummy implementation.  Since the filter does nothing in Update(), this 
   * method should never be called anyway */
  virtual bool Halt() { return true; }

private:
  SNAPLevelSetStopAndGoFilter(const Self&); //purposely not implemented
  void operator=(const Self&); //purposely not implemented
  
  /** Whether or not the stop and go is going */
  bool     m_Started;
};

#ifndef ITK_MANUAL_INSTANTIATION
#include "SNAPLevelSetStopAndGoFilter.txx"
#endif

#endif // __SNAPLevelSetStopAndGoFilter_h_
