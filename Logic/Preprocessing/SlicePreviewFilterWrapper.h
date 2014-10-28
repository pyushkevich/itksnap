#ifndef SLICEPREVIEWFILTERWRAPPER_H
#define SLICEPREVIEWFILTERWRAPPER_H

#include "SNAPCommon.h"
#include "itkDataObject.h"
#include "itkObjectFactory.h"

class ImageWrapperBase;
class ScalarImageWrapperBase;
template <class TInputImage, class TOutputImage> class IRISSlicer;

namespace itk {
  template<class TIn, class TOut> class StreamingImageFilter;
}

class SNAPImageData;

/**
  Abstract parent class for SlicePreviewFilterWrapper. Allows us to call
  some methods in the SlicePreviewFilterWrapper without knowing what traits
  it has been templated over.
  */
class AbstractSlicePreviewFilterWrapper : public itk::DataObject
{
public:
  typedef AbstractSlicePreviewFilterWrapper                            Self;
  typedef itk::DataObject                                        Superclass;
  typedef SmartPtr<Self>                                            Pointer;
  typedef SmartPtr<const Self>                                 ConstPointer;

  itkTypeMacro(AbstractSlicePreviewFilterWrapper, itk::DataObject)

  /** Is the preview mode on? */
  virtual bool IsPreviewMode() const = 0;

  /** Enter preview mode */
  virtual void SetPreviewMode(bool mode) = 0;

  /** Compute the output volume (corresponds to the 'Apply' operation) */
  virtual void ComputeOutputVolume(itk::Command *progress) = 0;

  /** Select the active scalar layer (for filters that operate on only one) */
  virtual void SetActiveScalarLayer(ScalarImageWrapperBase *layer) = 0;

  /** Get the active scalar layer (for filters that operate on only one). */
  virtual ScalarImageWrapperBase *GetActiveScalarLayer() const = 0;

protected:

  AbstractSlicePreviewFilterWrapper() {}
  virtual ~AbstractSlicePreviewFilterWrapper() {}

};

/**
  This class provides support for on-the-fly preview of speed images in
  SNAP. It is designed to be very generic to allow use with complex
  preprocessing pipelines that have multiple inputs. To make the object
  so generic, we parameterize it by a traits object:

    TFilterConfigTraits

  The traits object must define the following types

    FilterType         The filter that encapsulates the preprocessing pipeline

    ParameterType      The parameters passed to the filter

    OutputWrapperType  The type of wrapper these filters connect to

  The traits object must also define the following functions

    static void AttachInputs(SNAPImageData *sid, FilterType *filter)

        This function should set the input image(s) for the filter given
        a pointer to a SNAPImageData object.

    static void DetachInputs(FilterType *filter)

        This detaches all inputs from the filter.

    static void SetParameters(ParameterType *p, FilterType *filter)

        This sets the parameters of the filter


  What does this filter do? It creates an assembly consisting
  of three slice preview filters, and one whole-volume filter. The four
  filters share the same output buffer, but have different buffered
  regions.

  The behavior is as follows. When the preview mode is turned on, a request
  for a display slice will invoke one of the preview filters to apply its
  preprocessing operation to generate just the region of the speed image
  needed for display. When the preview mode is off, a request for a display
  slice uses the data currently stored in the speed image buffer (possibly
  uninitialized).

  The user can also ask this wrapper to apply the filter to generate the
  entire speed image volume. This can be done in or out of preview mode.

  The filter is smart enough to know when the whole volume is up to date. If
  the parameters of the preview filters have not been changed since the last
  time the whole speed volume was generated, the preview filters are deemed
  to be up to date, and no preprocessing operations take place.
  */
template<class TFilterConfigTraits>
class SlicePreviewFilterWrapper : public AbstractSlicePreviewFilterWrapper
{
public:

  typedef SlicePreviewFilterWrapper<TFilterConfigTraits>               Self;
  typedef AbstractSlicePreviewFilterWrapper                      Superclass;
  typedef SmartPtr<Self>                                            Pointer;
  typedef SmartPtr<const Self>                                 ConstPointer;

  itkTypeMacro(SlicePreviewFilterWrapper, AbstractSlicePreviewFilterWrapper)

  itkNewMacro(Self)

  typedef TFilterConfigTraits                                        Traits;

  typedef typename TFilterConfigTraits::FilterType               FilterType;
  typedef typename FilterType::OutputImageType              OutputImageType;
  typedef typename OutputImageType::PixelType               OutputPixelType;

  typedef typename TFilterConfigTraits::InputDataType         InputDataType;

  typedef typename TFilterConfigTraits::OutputWrapperType OutputWrapperType;
  typedef IRISSlicer<OutputImageType, itk::Image<OutputPixelType, 2> >
                                                                 SlicerType;

  typedef typename TFilterConfigTraits::ParameterType         ParameterType;

  /** Is the preview mode on? */
  irisIsMacro(PreviewMode)

  /** Enter preview mode */
  void SetPreviewMode(bool mode);

  /** Set the input image, etc */
  void AttachInputs(InputDataType *sid);

  /** Set the output volume */
  void AttachOutputWrapper(OutputWrapperType *wrapper);

  /** Select the active scalar layer (for filters that operate on only one) */
  void SetActiveScalarLayer(ScalarImageWrapperBase *layer);

  /** Get the active scalar layer (for filters that operate on only one). */
  irisGetMacro(ActiveScalarLayer, ScalarImageWrapperBase *)

  /** Set the parameters */
  void SetParameters(ParameterType *param);

  /** Detach from a wrapper */
  void DetachInputsAndOutputs();

  /** Compute the output volume (corresponds to the 'Apply' operation) */
  void ComputeOutputVolume(itk::Command *progress);

protected:

  SlicePreviewFilterWrapper();
  ~SlicePreviewFilterWrapper() {}

  void UpdatePipeline();

  OutputWrapperType *m_OutputWrapper;

  // Streamer - to allow reduced memory footprint
  typedef itk::StreamingImageFilter<OutputImageType, OutputImageType> Streamer;

  SmartPtr<FilterType> m_PreviewFilter[3];
  SmartPtr<FilterType> m_VolumeFilter;
  SmartPtr<Streamer> m_VolumeStreamer;

  // So we can loop over all four filters
  FilterType *GetNthFilter(int);

  // Active scalar layer (for layers that support this functionality)
  ScalarImageWrapperBase *m_ActiveScalarLayer;

  bool m_PreviewMode;

  void UpdateOutputPipelineReadyStatus();
};


#endif // SLICEPREVIEWFILTERWRAPPER_H
