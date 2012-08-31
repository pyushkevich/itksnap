#include "SlicePreviewFilterWrapper.h"

#include "GreyImageWrapper.h"
#include "SpeedImageWrapper.h"

#include "SmoothBinaryThresholdImageFilter.h"
#include "EdgePreprocessingImageFilter.h"
#include <IRISSlicer.h>
#include <ColorMap.h>


template <class TFilterConfigTraits>
SlicePreviewFilterWrapper<TFilterConfigTraits>
::SlicePreviewFilterWrapper()
{
  m_PreviewMode = false;

  // Allocate the volume filter and the preview filters
  for(int i = 0; i < 3; i++)
    {
    m_PreviewFilter[i] = FilterType::New();
    }
}

template <class TFilterConfigTraits>
void
SlicePreviewFilterWrapper<TFilterConfigTraits>
::SetParameters(ParameterType *param)
{
  // Set the parameters of the preview filters
  for(int i = 0; i < 3; i++)
    m_PreviewFilter[i]->SetParameters(param);
}

template <class TFilterConfigTraits>
void
SlicePreviewFilterWrapper<TFilterConfigTraits>
::AttachInputs(SNAPImageData *sid)
{
  for(unsigned int i = 0; i < 3; i++)
    Traits::AttachInputs(sid, m_PreviewFilter[i]);
}

template <class TFilterConfigTraits>
void
SlicePreviewFilterWrapper<TFilterConfigTraits>
::SetPreviewMode(bool mode)
{
  if(m_PreviewMode != mode)
    {
    m_PreviewMode = mode;
    this->Modified();
    this->UpdatePipeline();
    }
}

template <class TFilterConfigTraits>
void
SlicePreviewFilterWrapper<TFilterConfigTraits>
::AttachOutputWrapper(OutputWrapperType *wrapper)
{
  // The slice preview filters need to be attached to the slicer
  m_OutputWrapper = wrapper;
  this->UpdatePipeline();
}

template <class TFilterConfigTraits>
void
SlicePreviewFilterWrapper<TFilterConfigTraits>
::DetachInputsAndOutputs()
{
  if(m_OutputWrapper)
    {
    for(unsigned int i = 0; i < 3; i++)
      {
      // Disconnect wrapper from this pipeline
      m_OutputWrapper->GetSlicer(i)->SetInput(m_OutputWrapper->GetImage());

      // Undo the graft
      m_PreviewFilter[i]->GraftOutput(m_PreviewFilter[i]->GetOutput());
      }
    }

  m_OutputWrapper = NULL;

  for(unsigned int i = 0; i < 3; i++)
    {
    // Disconnect wrapper from this pipeline
    Traits::DetachInputs(m_PreviewFilter[i]);
    }
}

template <class TFilterConfigTraits>
void
SlicePreviewFilterWrapper<TFilterConfigTraits>
::UpdatePipeline()
{
  if(m_OutputWrapper)
    {
    if(m_PreviewMode)
      {
      for(unsigned int i = 0; i < 3; i++)
        {
        m_OutputWrapper->GetSlicer(i)->SetInput(m_PreviewFilter[i]->GetOutput());
        m_PreviewFilter[i]->GraftOutput(m_OutputWrapper->GetImage());
        m_PreviewFilter[i]->Modified();
        }
      }
    else
      {
      for(unsigned int i = 0; i < 3; i++)
        {
        m_OutputWrapper->GetSlicer(i)->SetInput(m_OutputWrapper->GetImage());
        m_PreviewFilter[i]->GraftOutput(m_PreviewFilter[i]->GetOutput());
        }
      }
    }
}

template <class TFilterConfigTraits>
void
SlicePreviewFilterWrapper<TFilterConfigTraits>
::ComputeOutputVolume(itk::Command *progress)
{
  // Attach the progress monitor
  unsigned long tag = 0;

  if(progress)
    tag = m_PreviewFilter[0]->AddObserver(itk::ProgressEvent(), progress);

  // Execute the preprocessing on the whole image extent
  m_PreviewFilter[0]->UpdateLargestPossibleRegion();

  // Remove the progress monitor
  if(progress)
    m_PreviewFilter[0]->RemoveObserver(tag);

  // Set the buffered region on all the preview filters to be the
  // largest possible region.
  for(unsigned int i = 0; i < 3; i++)
    m_PreviewFilter[i]->GetOutput()->SetBufferedRegion(
          m_OutputWrapper->GetImage()->GetLargestPossibleRegion());
}
