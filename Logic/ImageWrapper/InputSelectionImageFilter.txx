#ifndef INPUTSELECTIONIMAGEFILTER_TXX
#define INPUTSELECTIONIMAGEFILTER_TXX

#include "InputSelectionImageFilter.h"
#include "itkUnaryFunctorImageFilter.hxx"
#include "IRISException.h"

#include "itkImageAdaptor.h"
#include "itkVectorImageToImageAdaptor.h"
#include "MultiChannelDisplayMode.h"

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
  if(m_TagMap.size() == 0)
    {
    this->SetInput(input);
    this->Modified();
    m_SelectedInput = tag;
    }
  m_TagMap[tag] = input;
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
  if(m_SelectedInput != tag)
    {
    this->SetInput(m_TagMap[tag]);
    this->Modified();
    m_SelectedInput = tag;
    }
}

/**
 * Default implementation of selecting an input is to assign the pixel container from
 * the source to the target.
 */
template <class TInputImage, class TOutputImage>
class InputSelectionImageFilter_Specialization
{
public:
  static void GenerateData(TInputImage *inputPtr, TOutputImage *outputPtr)
  {
    outputPtr->CopyInformation(inputPtr);
    outputPtr->SetBufferedRegion(inputPtr->GetBufferedRegion());
    outputPtr->SetPixelContainer(inputPtr->GetPixelContainer());
  }
};

template <class TInputImage, class TOutputImage>
class InputSelectionImageFilter_Specialization_Adaptor
{
public:
  static void GenerateData(TInputImage *inputPtr, TOutputImage *outputPtr)
  {
    // Configure the internal pixel container for the output image
    typedef typename TOutputImage::InternalImageType OutputInternalImage;
    typename OutputInternalImage::Pointer tp_internals = OutputInternalImage::New();
    tp_internals->CopyInformation(inputPtr);
    tp_internals->SetBufferedRegion(inputPtr->GetBufferedRegion());
    tp_internals->SetNumberOfComponentsPerPixel(inputPtr->GetPixelAccessor().GetVectorLength());
    tp_internals->GetPixelContainer()->SetImportPointer(inputPtr->GetBufferPointer(),
                                                        inputPtr->GetPixelContainer()->Size());

    // Set up the output itself
    outputPtr->CopyInformation(tp_internals);
    outputPtr->SetImage(tp_internals);
    outputPtr->SetPixelAccessor(inputPtr->GetPixelAccessor());
  }
};

/**
 * Specialized implementation for image adaptor outputs requres special handling because
 * the internal image of the image adaptor also needs to be updated
 */
template <class TInputImage, class TOutputInternalImage, class TOutputAccesor>
class InputSelectionImageFilter_Specialization<TInputImage,
    itk::ImageAdaptor<TOutputInternalImage, TOutputAccesor> >
    : public InputSelectionImageFilter_Specialization_Adaptor<
    TInputImage,
    itk::ImageAdaptor<TOutputInternalImage, TOutputAccesor> >
{
};

template <class TInputImage, class TPixel, unsigned int VDim>
class InputSelectionImageFilter_Specialization<TInputImage,
    itk::VectorImageToImageAdaptor<TPixel, VDim> >
    : public InputSelectionImageFilter_Specialization_Adaptor<
    TInputImage,
    itk::VectorImageToImageAdaptor<TPixel, VDim> >
{
};


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

  typedef InputSelectionImageFilter_Specialization<InputImageType, OutputImageType> Specialization;
  Specialization::GenerateData(inputPtr, outputPtr);
}

#endif // #ifndef INPUTSELECTIONIMAGEFILTER_TXX
