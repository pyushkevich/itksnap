#include "SlicePreviewFilterWrapper.h"

#include "SmoothBinaryThresholdImageFilter.h"
#include "EdgePreprocessingImageFilter.h"
#include "itkStreamingImageFilter.h"
#include <IRISSlicer.h>
#include <ColorMap.h>
#include <itkTimeProbe.h>


template <class TFilterConfigTraits>
SlicePreviewFilterWrapper<TFilterConfigTraits>
::SlicePreviewFilterWrapper()
{
  m_PreviewMode = TFilterConfigTraits::GetDefaultPreviewMode();

  // Allocate the volume filter and the preview filters
  m_VolumeFilter = FilterType::New();
  m_VolumeFilter->ReleaseDataFlagOn();

  for(int i = 0; i < 3; i++)
    m_PreviewFilter[i] = FilterType::New();

  // Allocate the streamer and attach to the volume filter
  m_VolumeStreamer = Streamer::New();
  m_VolumeStreamer->SetInput(m_VolumeFilter->GetOutput());

  // Set up a streaming filter to reduce the memory footprint during execution
  m_VolumeStreamer->SetNumberOfStreamDivisions(9);

  // No active layer by default
  m_ActiveScalarLayer = NULL;

  // Set the output wrapper to NULL
  m_OutputWrapper = NULL;
}

template <class TFilterConfigTraits>
void
SlicePreviewFilterWrapper<TFilterConfigTraits>
::SetParameters(ParameterType *param)
{
  // Set the parameters of all the filters
  for(int i = 0; i < 4; i++)
    Traits::SetParameters(param, this->GetNthFilter(i), i);

  // After updates to the parameters/filters update the pipeline readiness
  // status in the output wrapper
  this->UpdateOutputPipelineReadyStatus();
}

template <class TFilterConfigTraits>
void
SlicePreviewFilterWrapper<TFilterConfigTraits>
::UpdateOutputPipelineReadyStatus()
{
  FilterType *array[] = {m_PreviewFilter[0], m_PreviewFilter[1], m_PreviewFilter[2]};
  if(m_OutputWrapper)
    m_OutputWrapper->SetPipelineReady(Traits::IsPreviewable(array));
}


template <class TFilterConfigTraits>
void
SlicePreviewFilterWrapper<TFilterConfigTraits>
::AttachInputs(InputDataType *sid)
{
  // Get the default scalar layer for the traits. If this is NULL, the method
  // does not expect an active layer to be specified (acts on all inputs)
  m_ActiveScalarLayer = Traits::GetDefaultScalarLayer(sid);
  for(int i = 0; i < 4; i++)
    {
    Traits::AttachInputs(sid, this->GetNthFilter(i), i);
    if(m_ActiveScalarLayer)
      Traits::SetActiveScalarLayer(m_ActiveScalarLayer, this->GetNthFilter(i), i);
    }
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
      m_OutputWrapper->GetSlicer(i)->SetPreviewInput(NULL);
      }

    // Undo the graft
    m_VolumeStreamer->GraftOutput(m_VolumeStreamer->GetOutput());
    }

  m_OutputWrapper = NULL;

  for(unsigned int i = 0; i < 4; i++)
    {
    // Disconnect wrapper from this pipeline
    Traits::DetachInputs(this->GetNthFilter(i));
    }

  m_ActiveScalarLayer = NULL;
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
      // Attach the pipeline filters
      m_OutputWrapper->AttachPreviewPipeline(
            m_PreviewFilter[0], m_PreviewFilter[1], m_PreviewFilter[2]);

      this->UpdateOutputPipelineReadyStatus();
      }
    else
      {
      m_OutputWrapper->DetachPreviewPipeline();
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
    tag = m_VolumeStreamer->AddObserver(itk::ProgressEvent(), progress);

  // Temporarily graft the target volume as output of the filter
  m_VolumeStreamer->GraftOutput(m_OutputWrapper->GetImage());

  // Execute the preprocessing on the whole image extent
  // itk::TimeProbe probe;
  // probe.Start();
  m_VolumeStreamer->UpdateLargestPossibleRegion();
  // probe.Stop();
  // std::cout << "Time Elapsed: " << probe.GetTotal() << std::endl;

  // Remove the progress monitor
  if(progress)
    m_VolumeStreamer->RemoveObserver(tag);

  // Undo the graft
  m_VolumeStreamer->GraftOutput(m_VolumeStreamer->GetOutput());

  // Update the m-time of the output image
  m_OutputWrapper->GetImage()->Modified();
  m_OutputWrapper->GetImage()->DisconnectPipeline();
}

template <class TFilterConfigTraits>
typename SlicePreviewFilterWrapper<TFilterConfigTraits>::FilterType *
SlicePreviewFilterWrapper<TFilterConfigTraits>
::GetNthFilter(int index)
{
  return (index > 0) ? m_PreviewFilter[index-1] : m_VolumeFilter;
}


template <class TFilterConfigTraits>
void
SlicePreviewFilterWrapper<TFilterConfigTraits>
::SetActiveScalarLayer(ScalarImageWrapperBase *layer)
{
  m_ActiveScalarLayer = layer;
  for(int i = 0; i < 4; i++)
    Traits::SetActiveScalarLayer(m_ActiveScalarLayer, this->GetNthFilter(i), i);
  this->Modified();
}
