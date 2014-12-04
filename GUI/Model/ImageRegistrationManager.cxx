#include "ImageRegistrationManager.h"
#include "GenericImageData.h"

#include <itkImageRegistrationMethodv4.h>
#include <itkAffineTransform.h>
#include <itkANTSNeighborhoodCorrelationImageToImageMetricv4.h>
// #include <itkJointHistogramMutualInformationImageToImageMetricv4.h>
#include <itkMattesMutualInformationImageToImageMetricv4.h>
#include <itkMeanSquaresImageToImageMetricv4.h>
#include <itkEuler3DTransform.h>
#include <itkConjugateGradientLineSearchOptimizerv4.h>
#include <itkRegistrationParameterScalesFromPhysicalShift.h>
#include <itkFastMutexLock.h>

#include <itkRecursiveGaussianImageFilter.h>

ImageRegistrationManager::ImageRegistrationManager()
{
  m_RegistrationResultLock = itk::FastMutexLock::New();
  m_RegistrationResult.Transform = TransformBase::New();
}

void ImageRegistrationManager::PerformRegistration(GenericImageData *imageData,
                                                   RegistrationMode in_mode,
                                                   RegistrationMetric in_metric,
                                                   RegistrationInit in_init)
{
  // Get the things we will be registering
  typedef itk::Image<float, 3> ImageType;

  // The main image already loaded - this will be the fixed image
  ImageWrapperBase *main = imageData->GetMain();
  ImageWrapperBase *overlay = imageData->GetLastOverlay();

  // Cast to floating point format
  SmartPtr<ScalarImageWrapperBase::FloatImageSource> castMain =
      main->GetDefaultScalarRepresentation()->CreateCastToFloatPipeline();

  SmartPtr<ScalarImageWrapperBase::FloatImageSource> castOverlay =
      overlay->GetDefaultScalarRepresentation()->CreateCastToFloatPipeline();

  // Apply some default smoothing to the images, with standard deviation equal to twice
  // the smallest image spacing
  typedef itk::RecursiveGaussianImageFilter<ImageType, ImageType> SmoothFilter;
  SmartPtr<SmoothFilter> smoothMain = SmoothFilter::New();
  smoothMain->SetInput(castMain->GetOutput());
  smoothMain->SetSigma(castMain->GetOutput()->GetSpacing().GetVnlVector().min_value() * 1.0);
  smoothMain->InPlaceOn();

  SmartPtr<SmoothFilter> smoothOverlay = SmoothFilter::New();
  smoothOverlay->SetInput(castOverlay->GetOutput());
  smoothOverlay->SetSigma(castOverlay->GetOutput()->GetSpacing().GetVnlVector().min_value() * 1.0);
  smoothOverlay->InPlaceOn();

  // Set up registration (for now, rigid is the default)
  typedef itk::Euler3DTransform<double> TransformType;
  typedef itk::ImageRegistrationMethodv4<ImageType, ImageType, TransformType> RegMethod;

  // Set up the registration method
  SmartPtr<RegMethod> method = RegMethod::New();

  // Set the fixed and moving images
  method->SetFixedImage(smoothMain->GetOutput());
  method->SetMovingImage(smoothOverlay->GetOutput());

  // Set the metric
  typedef itk::MattesMutualInformationImageToImageMetricv4<ImageType, ImageType> MetricType;
  SmartPtr<MetricType> metric = MetricType::New();

  metric->SetNumberOfHistogramBins(32);
  metric->SetUseMovingImageGradientFilter( false );
  metric->SetUseFixedImageGradientFilter( false );
  metric->SetUseFixedSampledPointSet( false );
  metric->SetVirtualDomainFromImage(castMain->GetOutput());

  method->SetMetric(metric);
  method->SetMetricSamplingStrategy(RegMethod::REGULAR);
  method->SetMetricSamplingPercentage(0.1);

  // Set up the number of shrink levels
  RegMethod::ShrinkFactorsArrayType shrinkArray(2);
  shrinkArray[0] = 4;
  shrinkArray[1] = 2;

  method->SetNumberOfLevels(2);
  method->SetShrinkFactorsPerLevel(shrinkArray);

  RegMethod::SmoothingSigmasArrayType smoothArray(2);
  smoothArray.fill(0.0);
  method->SetSmoothingSigmasPerLevel(smoothArray);

  // Set up the scales estimator
  typedef itk::RegistrationParameterScalesFromPhysicalShift<MetricType> ScalesEstimatorType;
  SmartPtr<ScalesEstimatorType> scalesEstimator = ScalesEstimatorType::New();
  scalesEstimator->SetMetric( metric );
  scalesEstimator->SetTransformForward( true );

  // Set up the optimizer
  typedef itk::ConjugateGradientLineSearchOptimizerv4Template<double> OptimizerType;
  SmartPtr<OptimizerType> optimizer = OptimizerType::New();
  optimizer->SetLowerLimit( 0 );
  optimizer->SetUpperLimit( 2 );
  optimizer->SetEpsilon( 0.2 );
  optimizer->SetLearningRate( 0.25 );
  optimizer->SetMaximumStepSizeInPhysicalUnits( 0.25 );
  optimizer->SetNumberOfIterations( 1000 );
  optimizer->SetScalesEstimator( scalesEstimator );
  optimizer->SetDoEstimateScales( true );
  optimizer->SetMinimumConvergenceValue( 1e-6 );
  optimizer->SetConvergenceWindowSize( 10 );
  optimizer->SetDoEstimateLearningRateAtEachIteration( true );
  optimizer->SetDoEstimateLearningRateOnce( false );

  // Set the optimizer
  method->SetOptimizer(optimizer);

  // Print out the optimizer status at every iteration.
  typedef itk::MemberCommand<Self> CommandType;
  SmartPtr<CommandType> command = CommandType::New();
  command->SetCallbackFunction(this, &Self::OnRegistrationUpdate);
  optimizer->AddObserver(itk::IterationEvent(), command);

  // Perform registration
  method->Update();
}

void
ImageRegistrationManager
::OnRegistrationUpdate(itk::Object *caller, const itk::EventObject &event)
{
  // Get the optimizer
  OptimizerType *optimizer = static_cast<OptimizerType *>(caller);

  // Obtain a lock on the registration result
  m_RegistrationResultLock->Lock();

  // Get the metric value
  m_RegistrationResult.MetricValue = optimizer->GetValue();

  // TODO: split by transform type!

  // Get the parameters and convert them to a matrix
  typedef itk::Euler3DTransform<double> TransformType;
  SmartPtr<TransformType> transform = TransformType::New();
  transform->SetParameters(optimizer->GetCurrentPosition());

  m_RegistrationResult.Transform->SetMatrix(transform->GetMatrix());
  m_RegistrationResult.Transform->SetOffset(transform->GetOffset());

  // Release the lock on the registration result
  m_RegistrationResultLock->Unlock();

  // Fire an iteration event
  InvokeEvent(itk::IterationEvent());
}

void ImageRegistrationManager::UpdateImageTransformFromRegistration(GenericImageData *imageData)
{
  // Make a copy of the registration result
  m_RegistrationResultLock->Lock();
  RegistrationResult result = m_RegistrationResult;
  m_RegistrationResultLock->Unlock();

  std::cout << "METRIC Value: " << result.MetricValue << std::endl;

  // Apply the registration result to the data
  imageData->GetLastOverlay()->SetITKTransform(
        imageData->GetMain()->GetImageBase(), result.Transform);
}

double ImageRegistrationManager::GetRegistrationObjective()
{
  return m_RegistrationResult.MetricValue;
}

