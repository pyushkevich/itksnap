#ifndef REGISTRATIONMODEL_H
#define REGISTRATIONMODEL_H

#include "AbstractModel.h"
#include "PropertyModel.h"
#include "itkMatrix.h"
#include "itkVector.h"

class GlobalUIModel;
class IRISApplication;
class ImageWrapperBase;

class RegistrationModel : public AbstractModel
{
public:
  irisITKObjectMacro(RegistrationModel, AbstractModel)

  /**
    States in which the model can be, which allow the activation and
    deactivation of various widgets in the interface
    */
  enum UIState {
    UIF_MOVING_SELECTION_AVAILABLE,
    UIF_MOVING_SELECTED
  };

  /** Allowed transformation models - to be expanded in the future */
  enum Transformation { RIGID = 0, AFFINE, INVALID_MODE };

  /** Image similarity metrics */
  enum SimilarityMetric { NMI = 0, NCC, SSD, INVALID_METRIC };

  /**
    Check the state flags above
    */
  bool CheckState(UIState state);

  void SetParentModel(GlobalUIModel *model);
  irisGetMacro(Parent, GlobalUIModel *)

  virtual void OnUpdate();

  typedef SimpleItemSetDomain<unsigned long, std::string> LayerSelectionDomain;
  typedef AbstractPropertyModel<unsigned long, LayerSelectionDomain> AbstractLayerSelectionModel;

  /** Model for selecting the layer for registration */
  irisGetMacro(MovingLayerModel, AbstractLayerSelectionModel *)

  /** Euler angles */
  irisGetMacro(EulerAnglesModel, AbstractRangedDoubleVec3Property *)

  /** Translation */
  irisGetMacro(TranslationModel, AbstractRangedDoubleVec3Property *)

  /** Scaling factor */
  irisGetMacro(ScalingModel, AbstractRangedDoubleVec3Property *)

  /** Logarithm of the scaling factor - for the slider */
  irisGetMacro(LogScalingModel, AbstractRangedDoubleVec3Property *)

  /** Interactive registration tool button */
  irisGetMacro(InteractiveToolModel, AbstractSimpleBooleanProperty *)

  /** Set the center of rotation to current cross-hairs position */
  void SetCenterOfRotationToCursor();

  /** Reset the rotation to identity */
  void ResetTransformToIdentity();

  /** Apply a rotation around a fixed angle */
  void ApplyRotation(const Vector3d &axis, double theta);

  /** Apply a translation (specified in physical ITK space) */
  void ApplyTranslation(const Vector3d &tran);

  /** Get a pointer to the selected moving wrapper, or NULL if none selected */
  ImageWrapperBase *GetMovingLayerWrapper();

  /** Get the center of rotation, in voxel units of the main image */
  irisGetMacro(RotationCenter, Vector3ui)

  // Automatic registration parameter domains
  typedef SimpleItemSetDomain<Transformation, std::string> TransformationDomain;
  typedef SimpleItemSetDomain<SimilarityMetric, std::string> SimilarityMetricDomain;

  // Access to registration models
  irisGenericPropertyAccessMacro(Transformation, Transformation, TransformationDomain)
  irisGenericPropertyAccessMacro(SimilarityMetric, SimilarityMetric, SimilarityMetricDomain)

  void RunAutoRegistration();


protected:
  RegistrationModel();
  ~RegistrationModel();

  typedef itk::Matrix<double, 3, 3> ITKMatrixType;
  typedef itk::Vector<double, 3> ITKVectorType;

  GlobalUIModel *m_Parent;
  IRISApplication *m_Driver;

  void ResetOnMainImageChange();

  // This method is used to updated the cached matrix/offset and the parameters such
  // as Euler angles from the information in the moving image wrapper
  void UpdateManualParametersFromWrapper(bool force_update = false);

  // This method is called to recompute the transform in the moving image wrapper from
  // parameters including scaling, euler angles, and translation
  void UpdateWrapperFromManualParameters();

  void SetRotationCenter(const Vector3ui &pos);

  // Get the transform currently stored in the moving layer
  void GetMovingTransform(ITKMatrixType &matrix, ITKVectorType &offset);

  // Set the transform in the moving layer
  void SetMovingTransform(const ITKMatrixType &matrix, const ITKVectorType &offset);

  SmartPtr<AbstractLayerSelectionModel> m_MovingLayerModel;
  bool GetMovingLayerValueAndRange(unsigned long &value, LayerSelectionDomain *range);
  void SetMovingLayerValue(unsigned long value);

  SmartPtr<AbstractSimpleBooleanProperty> m_InteractiveToolModel;
  bool GetInteractiveToolValue(bool &value);
  void SetInteractiveToolValue(bool value);

  SmartPtr<AbstractRangedDoubleVec3Property> m_EulerAnglesModel;
  bool GetEulerAnglesValueAndRange(Vector3d &value, NumericValueRange<Vector3d> *range);
  void SetEulerAnglesValue(Vector3d value);

  SmartPtr<AbstractRangedDoubleVec3Property> m_TranslationModel;
  bool GetTranslationValueAndRange(Vector3d &value, NumericValueRange<Vector3d> *range);
  void SetTranslationValue(Vector3d value);

  SmartPtr<AbstractRangedDoubleVec3Property> m_ScalingModel;
  bool GetScalingValueAndRange(Vector3d &value, NumericValueRange<Vector3d> *range);
  void SetScalingValue(Vector3d value);

  SmartPtr<AbstractRangedDoubleVec3Property> m_LogScalingModel;
  bool GetLogScalingValueAndRange(Vector3d &value, NumericValueRange<Vector3d> *range);
  void SetLogScalingValue(Vector3d value);

  typedef ConcretePropertyModel<Transformation, TransformationDomain> TransformationModel;
  SmartPtr<TransformationModel> m_TransformationModel;

  typedef ConcretePropertyModel<SimilarityMetric, SimilarityMetricDomain> SimilarityMetricModel;
  SmartPtr<SimilarityMetricModel> m_SimilarityMetricModel;


  // Value corresponding to no layer selected
  static const unsigned long NOID;

  // The active layer for the segmentation
  unsigned long m_MovingLayerId;

  // The components of the transform that are presented to the user by this widget
  struct TransformManualParameters
  {
    // The affine matrix/offset from which the parameters were generated
    ITKMatrixType AffineMatrix;
    ITKVectorType AffineOffset;

    // Euler angles
    Vector3d EulerAngles;

    // Translation
    Vector3d Translation;

    // Scaling
    Vector3d Scaling;

    // Shearing
    vnl_matrix_fixed<double, 3, 3> ShearingMatrix;

    // Range of translation
    NumericValueRange<Vector3d> TranslationRange;

    // The unique layer ID for which this data was computed
    unsigned long LayerID;

    // Time stamp of the last update
    itk::TimeStamp UpdateTime;

    TransformManualParameters() : LayerID(NOID) {}
  };

  // The current cached manual parameters
  TransformManualParameters m_ManualParam;

  // Current center of rotation - should be initialized to the center when new image is loaded
  Vector3ui m_RotationCenter;
};

#endif // REGISTRATIONMODEL_H
