/*=========================================================================
 *
 *  Copyright Insight Software Consortium
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *         http://www.apache.org/licenses/LICENSE-2.0.txt
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *=========================================================================*/
#ifndef __CPUImageToGPUImageFilter_h
#define __CPUImageToGPUImageFilter_h

#include "itkGPUImage.h"
#include "itkGPUKernelManager.h"
#include "itkImageSource.h"

/** \class CPUImageToGPUImageFilter
 *
 * \brief class to abstract the behaviour of the GPU filters.
 *
 * CPUImageToGPUImageFilter is the GPU version of ImageToImageFilter.
 * This class can accept both CPU and GPU image as input and output,
 * and apply filter accordingly. If GPU is available for use, then
 * GPUGenerateData() is called. Otherwise, GenerateData() in the
 * parent class (i.e., ImageToImageFilter) will be called.
 *
 * \ingroup ITKGPUCommon
 */
template< class TGPUOutputImage >
class CPUImageToGPUImageFilter : public itk::ImageSource< TGPUOutputImage >
{
public:
  /** Standard class typedefs. */
  typedef CPUImageToGPUImageFilter               Self;
  typedef itk::ImageSource< TGPUOutputImage >         Superclass;
  typedef itk::SmartPointer< Self >                   Pointer;
  typedef itk::SmartPointer< const Self >             ConstPointer;

  itkNewMacro(Self);

  /** Run-time type information (and related methods). */
  itkTypeMacro(CPUImageToGPUImageFilter, TParentImageFilter);

  /** Superclass typedefs. */
  typedef typename Superclass::DataObjectIdentifierType DataObjectIdentifierType;
  typedef typename Superclass::OutputImageRegionType    OutputImageRegionType;
  typedef typename Superclass::OutputImagePixelType     OutputImagePixelType;

  /** Some convenient typedefs. */
  typedef TGPUOutputImage                       OutputImageType;
  typedef typename TGPUOutputImage::Pointer     OutputImagePointer;

  typedef typename TGPUOutputImage::Superclass  InputImageType;
  typedef typename InputImageType::Pointer      InputImagePointer;
  typedef typename InputImageType::ConstPointer InputImageConstPointer;
  typedef typename InputImageType::RegionType   InputImageRegionType;
  typedef typename InputImageType::PixelType    InputImagePixelType;

  /** ImageDimension constants */
  itkStaticConstMacro(InputImageDimension, unsigned int, InputImageType::ImageDimension);
  itkStaticConstMacro(OutputImageDimension, unsigned int, OutputImageType::ImageDimension);

  /** Set/Get the image input of this process object.  */
  using Superclass::SetInput;
  virtual void SetInput(const InputImageType *image);

  void GenerateData();

  virtual void GraftOutput(itk::DataObject *output);

  virtual void GraftOutput(const DataObjectIdentifierType & key, itk::DataObject *output);

protected:
  CPUImageToGPUImageFilter();
  ~CPUImageToGPUImageFilter();

  virtual void PrintSelf(std::ostream & os, itk::Indent indent) const;

  //virtual void GPUGenerateData() {  }

  // GPU kernel manager
  typename itk::GPUKernelManager::Pointer m_GPUKernelManager;

  // GPU kernel handle - kernel should be defined in specific filter (not in the
  // base class)
  //int m_KernelHandle;

private:
  CPUImageToGPUImageFilter(const Self &); //purposely not implemented
  void operator=(const Self &);        //purposely not implemented

};

#ifndef ITK_MANUAL_INSTANTIATION
#include "CPUImageToGPUImageFilter.hxx"
#endif

#endif
