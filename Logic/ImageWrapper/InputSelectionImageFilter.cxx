#include "InputSelectionImageFilter.h"
#include "itkUnaryFunctorImageFilter.hxx"
#include "IRISException.h"

#include "itkAddImageFilter.h"

template<class TInputImage, typename TTag>
InputSelectionImageFilter<TInputImage,TTag>
::InputSelectionImageFilter()
{
}

template<class TInputImage, typename TTag>
void
InputSelectionImageFilter<TInputImage,TTag>
::AddSelectableInput(TagType tag, InputImageType *input)
{
  m_TagMap[tag] = input;
  if(m_TagMap.size() == 0)
    SetSelectedInput(tag);
}

template<class TInputImage, typename TTag>
void
InputSelectionImageFilter<TInputImage,TTag>
::RemoveAllSelectableInputs()
{
  m_TagMap.clear();
  this->SetInput(NULL);
}

template<class TInputImage, typename TTag>
void
InputSelectionImageFilter<TInputImage,TTag>
::SetSelectedInput(TagType &tag)
{
  this->SetInput(m_TagMap[tag]);
  m_SelectedInput = tag;
  this->Modified();
}

/**
 * Adopted from itk::InPlaceImageFilter::InternalAllocateOutputs()
 */
template<class TInputImage, typename TTag>
void
InputSelectionImageFilter<TInputImage,TTag>
::GenerateData()
{
  // Just pass the container to the output image
  InputImageType *inputPtr = const_cast<InputImageType *>(this->GetInput());
  OutputImageType *outputPtr = this->GetOutput();

  outputPtr->CopyInformation(inputPtr);
  outputPtr->SetBufferedRegion(inputPtr->GetBufferedRegion());
  outputPtr->SetPixelContainer(inputPtr->GetPixelContainer());
}

#include "DisplayMappingPolicy.h"
#include "itkRGBAPixel.h"
template class InputSelectionImageFilter<ImageWrapperBase::DisplaySliceType, MultiChannelDisplayMode>;
