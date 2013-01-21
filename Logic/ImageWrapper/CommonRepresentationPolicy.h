#ifndef SCALARIMAGEWRAPPERCOMMONREPRESENTATION_H
#define SCALARIMAGEWRAPPERCOMMONREPRESENTATION_H

#include "ImageWrapperBase.h"

namespace itk
{
template<class TIn, class TOut> class CastImageFilter;
}

/**
 * The parent class for the hierarchy of ScalarImageWrapper policy classes
 * that allow the internal image in the ScalarImageWrapper to be cast to
 * an itk::Image in some common format. The resust of casting to a common
 * format can be used in downstream pipelines without needing to know what
 * the format of the internal image was
 */
template <class TOutputPixel>
class AbstractScalarImageWrapperCommonRepresentation
{
public:
  typedef itk::Image<TOutputPixel, 3> OutputImageType;

  /**
   * Export the image to a common format
   * @see ScalarImageWrapperBase::GetCommonFormatImage
   */
  virtual OutputImageType *GetOutput(ScalarImageWrapperBase::ExportChannel channel) = 0;
};

/**
 * This implementation of the policy is for use by ScalarImageWrappers whose
 * internal image is a plain itk::Image of TOutputPixel type. No casting takes
 * place in that case, and the output image simply shares the pixel container
 * with the image stored in the ScalarImageWrapper.
 */
template <class TOutputPixel, class TWrapperTraits>
class InPlaceScalarImageWrapperCommonRepresentation
    : public AbstractScalarImageWrapperCommonRepresentation<TOutputPixel>
{
public:
  typedef AbstractScalarImageWrapperCommonRepresentation<TOutputPixel> Superclass;
  typedef typename Superclass::OutputImageType                 OutputImageType;
  typedef typename TWrapperTraits::ImageType                    InputImageType;

  InPlaceScalarImageWrapperCommonRepresentation();
  ~InPlaceScalarImageWrapperCommonRepresentation();

  OutputImageType *GetOutput(ScalarImageWrapperBase::ExportChannel channel);

  void UpdateInputImage(InputImageType *image);

private:
  SmartPtr<OutputImageType> m_Image;
};

/**
 * This implementation of the policy uses an itk cast filter. This is used when
 * the image stored in the ScalarImageWrapper does not have the type needed for
 * export.
 */
template <class TOutputPixel, class TWrapperTraits>
class CastingScalarImageWrapperCommonRepresentation
    : public AbstractScalarImageWrapperCommonRepresentation<TOutputPixel>
{
public:
  typedef AbstractScalarImageWrapperCommonRepresentation<TOutputPixel> Superclass;
  typedef typename Superclass::OutputImageType                 OutputImageType;
  typedef typename TWrapperTraits::ImageType                    InputImageType;

  CastingScalarImageWrapperCommonRepresentation();
  ~CastingScalarImageWrapperCommonRepresentation();

  OutputImageType *GetOutput(ScalarImageWrapperBase::ExportChannel channel);

  void UpdateInputImage(InputImageType *image);

private:
  typedef itk::CastImageFilter<InputImageType, OutputImageType> CastFilterType;
  typedef SmartPtr<CastFilterType>                           CastFilterPointer;

  // Array of casting filters
  std::vector<CastFilterPointer> m_CastFilter;
};

/**
 * A null implementation of the above policy, intended for classes that do not
 * need to be cast to any output type. The representation always returns NULL
 * in GetOutput().
 */
template <class TOutputPixel, class TWrapperTraits>
class NullScalarImageWrapperCommonRepresentation
    : public AbstractScalarImageWrapperCommonRepresentation<TOutputPixel>
{
public:
  typedef AbstractScalarImageWrapperCommonRepresentation<TOutputPixel> Superclass;
  typedef typename Superclass::OutputImageType                 OutputImageType;
  typedef typename TWrapperTraits::ImageType                    InputImageType;

  OutputImageType *GetOutput(ScalarImageWrapperBase::ExportChannel channel)
    { return NULL; }

  void UpdateInputImage(InputImageType *) {};

};

#endif // SCALARIMAGEWRAPPERCOMMONREPRESENTATION_H
