#ifndef IMAGELABELINTEGRATOR_H
#define IMAGELABELINTEGRATOR_H

#include <itkProcessObject.h>
#include <itkImage.h>
#include <itkTransform.h>
#include <itkDataObjectDecorator.h>

using itk::DataObjectDecorator;

class ImageLabelIntegratorBase : public itk::ProcessObject
{
public:
  /** Standard class type aliases. */
  using Self = ImageLabelIntegratorBase;
  using Superclass = itk::ProcessObject;
  using Pointer = itk::SmartPointer<Self>;
  using ConstPointer = itk::SmartPointer<const Self>;

  /** Collect statistics from a run length of pixels */
  virtual void GetRunLengthIntensityStatistics(
    const itk::Index<3> &startIdx, long runlength,
    double &out_nvalid, double &out_sum, double &out_sumsq) = 0;

protected:
  ImageLabelIntegratorBase() {}
  virtual ~ImageLabelIntegratorBase() {}

};


template <typename TInputImage, typename TOutputImage, typename TWorkerTraits>
class NonOrthogonalImageLabelIntegrator : public ImageLabelIntegratorBase
{
public:
  /** Standard class type aliases. */
  using Self = NonOrthogonalImageLabelIntegrator<TInputImage, TOutputImage, TWorkerTraits>;
  using Superclass = ImageLabelIntegratorBase;
  using Pointer = itk::SmartPointer<Self>;
  using ConstPointer = itk::SmartPointer<const Self>;

  using InputImageType = TInputImage;
  using InputImagePointer = typename InputImageType::ConstPointer;
  using InputPixelType = typename InputImageType::PixelType;
  using InputComponentType = typename InputImageType::InternalPixelType;

  using OutputImageType = TOutputImage;
  using OutputImagePointer = typename OutputImageType::Pointer;
  using OutputPixelType = typename OutputImageType::PixelType;
  using OutputComponentType = typename OutputImageType::InternalPixelType;

  /** Method for creation through the object factory. */
  itkNewMacro(Self)

  /** Run-time type information (and related methods). */
  itkTypeMacro(NonOrthogonalImageLabelIntegrator, ImageLabelIntegratorBase)

  itkStaticConstMacro(InputImageDimension, unsigned int, TInputImage::ImageDimension);
  using ReferenceImageBaseType = itk::ImageBase<InputImageDimension>;

  /** Reference image input */
  itkSetInputMacro(InputImage, InputImageType)
  itkGetInputMacro(InputImage, InputImageType)

  /** Reference image input */
  itkSetInputMacro(ReferenceImage, ReferenceImageBaseType)
  itkGetInputMacro(ReferenceImage, ReferenceImageBaseType)

  /** Transform */
  typedef itk::Transform<double, InputImageDimension, InputImageDimension> TransformType;

  /** Transform input */
  itkSetDecoratedObjectInputMacro(Transform, TransformType)
  itkGetDecoratedObjectInputMacro(Transform, TransformType)

  /** Interpolation type */
  itkSetMacro(UseNearestNeighbor, bool)
  itkGetMacro(UseNearestNeighbor, bool)

  /** Initialize the class for run-line processing */
  virtual void Initialize();

  /** Process a run length */
  virtual void GetRunLengthIntensityStatistics(
    const itk::Index<3> &startIdx, long runlength,
    double &out_nvalid, double &out_sum, double &out_sumsq) override;

protected:
  NonOrthogonalImageLabelIntegrator();
  ~NonOrthogonalImageLabelIntegrator();

  bool m_UseNearestNeighbor = false;

  // Pointer to the worker that samples the image. This is a plain c pointer because
  // neither the worker nor the interpolator inherit from itk::Object
  TWorkerTraits *m_Worker = nullptr;
};

#ifndef ITK_MANUAL_INSTANTIATION
#include "ImageLabelIntegrator.txx"
#endif


#endif // IMAGELABELINTEGRATOR_H
