#ifndef REORIENTIMAGEMODEL_H
#define REORIENTIMAGEMODEL_H

#include <AbstractModel.h>
#include <PropertyModel.h>
#include <ImageCoordinateGeometry.h>

class GlobalUIModel;

/**
 * @brief The ReorientImageModel class
 * Model for the reorient dialog
 */
class ReorientImageModel : public AbstractModel
{
public:

  // A representation for an axis-direction pair
  typedef ImageCoordinateGeometry::AxisDirection AxisDirection;
  typedef STLMapWrapperItemSetDomain<AxisDirection, std::string> AxisDirectionDomain;
  typedef AbstractPropertyModel<AxisDirection, AxisDirectionDomain>
    AbstractAxisDirectionProperty;

  // A model encapsulating the 4x4 NIFTI matrix
  typedef vnl_matrix<double> WorldMatrix;
  typedef AbstractPropertyModel<WorldMatrix> AbstractMatrixProperty;
  typedef ConcretePropertyModel<WorldMatrix> ConcreteMatrixProperty;

  // Standard ITK stuff
  irisITKObjectMacro(ReorientImageModel, AbstractModel)

  /** Initialize with the parent model */
  void SetParentModel(GlobalUIModel *parent);

  /** A model for the current RAI field */
  irisGetMacro(NewRAICodeModel, AbstractSimpleStringProperty *)

  /** A model for the current RAI field */
  irisGetMacro(CurrentRAICodeModel, AbstractSimpleStringProperty *)

  /** A model for the invalid status */
  irisGetMacro(InvalidStatusModel, AbstractSimpleStringProperty *)

  /** Models for the current direction of the three coordinate axes */
  AbstractSimpleStringProperty *GetCurrentAxisDirectionModel(int axis) const;

  /** Models for the new direction of the three coordinate axes */
  AbstractAxisDirectionProperty *GetNewAxisDirectionModel(int axis) const;

  /** Model for the current NIFTI matrix */
  irisGetMacro(CurrentWorldMatrixModel, AbstractMatrixProperty *)

  /** Model for the new NIFTI matrix */
  irisGetMacro(NewWorldMatrixModel, AbstractMatrixProperty *)

  /** Model for the current ITK Direction matrix */
  irisGetMacro(CurrentDirectionMatrixModel, AbstractMatrixProperty *)

  /** Model for the new ITK Direction matrix */
  irisGetMacro(NewDirectionMatrixModel, AbstractMatrixProperty *)

  /** Apply current RAI code to the image */
  void ApplyCurrentRAI();

  /** Reverse the direction of one of the axes */
  void ReverseAxisDirection(int axis);

  /**
    States in which the model can be, which allow the activation and
    deactivation of various widgets in the interface
    */
  enum UIState {
    UIF_IMAGE_LOADED,
    UIF_VALID_NEW_RAI,
    UIF_VALID_AXIS_DIRECTION_X,
    UIF_VALID_AXIS_DIRECTION_Y,
    UIF_VALID_AXIS_DIRECTION_Z
  };

  /**
    Check the state flags above
    */
  bool CheckState(UIState state);

protected:

  // Constructor
  ReorientImageModel();
  virtual ~ReorientImageModel() {}

  // Model for the new RAI code
  SmartPtr<ConcreteSimpleStringProperty> m_NewRAICodeModel;

  // Model for the current RAI code
  SmartPtr<AbstractSimpleStringProperty> m_CurrentRAICodeModel;
  bool GetCurrentRAICodeValue(std::string &value);

  // Model for the error message
  SmartPtr<AbstractSimpleStringProperty> m_InvalidStatusModel;
  bool GetInvalidStatusValue(std::string &value);

  // Models for the three current axis directions
  SmartPtr<AbstractSimpleStringProperty> m_CurrentAxisDirectionModel[3];
  bool GetNthCurrentAxisDirectionValue(int axis, std::string &value);

  // Models for the three new axis directions
  SmartPtr<AbstractAxisDirectionProperty> m_NewAxisDirectionModel[3];

  bool GetNthNewAxisDirectionValueAndDomain(
      int axis, AxisDirection &value, AxisDirectionDomain *domain);

  void SetNthNewAxisDirectionValue(int axis, AxisDirection value);

  // Current world matrix
  SmartPtr<ConcreteMatrixProperty> m_CurrentWorldMatrixModel;

  // Current direction matrix
  SmartPtr<ConcreteMatrixProperty> m_CurrentDirectionMatrixModel;

  // New world matrix
  SmartPtr<AbstractMatrixProperty> m_NewWorldMatrixModel;
  bool GetNewWorldMatrixValue(WorldMatrix &value);

  // New direction matrix
  SmartPtr<AbstractMatrixProperty> m_NewDirectionMatrixModel;
  bool GetNewDirectionMatrixValue(WorldMatrix &value);

  // Respond to events received by the model
  virtual void OnUpdate();

  // The parent model
  GlobalUIModel *m_Parent;

  // The current RAI of the loaded image. This is updated in the OnUpdate
  // method, and is equal to the GetImageToAnatomyRAI in IRISApplication.
  // We keep a copy of this value to avoid unnecessary computation of the RAI
  // in response to widget events.
  std::string m_CurrentRAIValue;

  // Whether the current rai value is oblique
  bool m_CurrentOrientationIsOblique;

};

#endif // REORIENTIMAGEMODEL_H
