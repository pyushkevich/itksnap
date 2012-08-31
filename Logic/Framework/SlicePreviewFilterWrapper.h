#ifndef SLICEPREVIEWFILTERWRAPPER_H
#define SLICEPREVIEWFILTERWRAPPER_H

#include "SNAPCommon.h"
#include "itkDataObject.h"
#include "itkObjectFactory.h"

template <class TPixel> class ImageWrapper;
template <class TPixel> class ScalarImageWrapper;
template <class TPixel> class IRISSlicer;

class SNAPImageData;

/**
  This class provides support for on-the-fly preview of speed images in
  SNAP. It is designed to be very generic to allow use with complex
  preprocessing pipelines that have multiple inputs. To make the object
  so generic, we parameterize it by a traits object:

    TFilterConfigTraits

  The traits object must define the following types

    FilterType      The filter that encapsulates the preprocessing pipeline

    ParameterType   The parameters passed to the filter

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
class SlicePreviewFilterWrapper : public itk::DataObject
{
public:

  typedef SlicePreviewFilterWrapper<TFilterConfigTraits>              Self;
  typedef itk::Object                                            Superclass;
  typedef SmartPtr<Self>                                            Pointer;
  typedef SmartPtr<const Self>                                 ConstPointer;

  itkTypeMacro(SlicePreviewFilterWrapper, itk::DataObject)

  itkNewMacro(Self)

  typedef TFilterConfigTraits                                        Traits;

  typedef typename TFilterConfigTraits::FilterType               FilterType;
  typedef typename FilterType::OutputImageType              OutputImageType;
  typedef typename OutputImageType::PixelType               OutputPixelType;

  typedef ImageWrapper<OutputPixelType>                   OutputWrapperType;
  typedef IRISSlicer<OutputPixelType>                            SlicerType;

  typedef typename TFilterConfigTraits::ParameterType         ParameterType;

  /** Is the preview mode on? */
  irisIsMacro(PreviewMode)

  /** Enter preview mode */
  void SetPreviewMode(bool mode);

  /** Set the input image, etc */
  void AttachInputs(SNAPImageData *sid);

  /** Set the output volume */
  void AttachOutputWrapper(OutputWrapperType *wrapper);

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

  SmartPtr<FilterType> m_PreviewFilter[3];

  bool m_PreviewMode;
};


#endif // SLICEPREVIEWFILTERWRAPPER_H
