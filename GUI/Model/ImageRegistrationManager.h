#ifndef IMAGEREGISTRATIONMANAGER_H
#define IMAGEREGISTRATIONMANAGER_H

#include "AbstractModel.h"
#include "itkMatrixOffsetTransformBase.h"

class GenericImageData;

namespace itk {
  class FastMutexLock;
  template <class TFloat> class ConjugateGradientLineSearchOptimizerv4Template;
  template <class TFloat, unsigned int VDim1, unsigned int VDim2> class MatrixOffsetTransformBase;
}

/**
 * This class encapsulates the logic of rigid/affine image registration. It
 * can perform registration and provide registration results.
 */
class ImageRegistrationManager : public AbstractModel
{
public:

  irisITKObjectMacro(ImageRegistrationManager, AbstractModel)

  // Registration enums
  enum RegistrationMode { INITONLY = 0, RIGID, SIMILARITY, AFFINE, INVALID_MODE };
  enum RegistrationMetric { NMI = 0, NCC, SSD, INVALID_METRIC };
  enum RegistrationInit { HEADERS=0, CENTERS, INVALID_INIT };

  // Perform registration with provided parameters
  void PerformRegistration(GenericImageData *imageData,
                           RegistrationMode mode,
                           RegistrationMetric metric,
                           RegistrationInit init);

  // Update image data with the registration progress
  void UpdateImageTransformFromRegistration(GenericImageData *imageData);

  // Get the value of the objective function
  double GetRegistrationObjective();

protected:

  ImageRegistrationManager();
  ~ImageRegistrationManager() {}

  void OnRegistrationUpdate(itk::Object *caller, const itk::EventObject &event);

private:

  // Registration typedefs
  typedef itk::ConjugateGradientLineSearchOptimizerv4Template<double> OptimizerType;
  typedef itk::MatrixOffsetTransformBase<double, 3, 3> TransformBase;

  // Representation of the result of image registration
  struct RegistrationResult {
    double MetricValue;
    SmartPtr<TransformBase> Transform;
  };

  // Thread-safe variable for storing registration results
  RegistrationResult m_RegistrationResult;
  SmartPtr<itk::FastMutexLock> m_RegistrationResultLock;

  // Initial direction matrix of the image
  vnl_matrix<double> m_InitialDirectionMatrix;
  vnl_vector<double> m_InitialOrigin;
};

#endif // IMAGEREGISTRATIONMANAGER_H
