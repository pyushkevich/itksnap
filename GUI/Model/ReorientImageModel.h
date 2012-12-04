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

  /** Models for the direction of the three coordinate axes */
  AbstractAxisDirectionProperty *GetAxisDirectionModel(int axis) const;

  /** Apply current RAI code to the image */
  void ApplyCurrentRAI();

  /** Reverse the direction of one of the axes */
  void ReverseAxisDirection(int axis);

  /**
    States in which the model can be, which allow the activation and
    deactivation of various widgets in the interface
    */
  enum UIState {
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

  bool GetNthAxisDirectionValueAndDomain(
      int axis, AxisDirection &value, AxisDirectionDomain *domain);

  void SetNthAxisDirectionValue(int axis, const AxisDirection &value);

  // Models for the three axis directions
  SmartPtr<AbstractAxisDirectionProperty> m_AxisDirectionModel[3];

  // Model for the X-coordinate axis
  bool GetXAxisDirectionValueAndDomain(
      AxisDirection &value, AxisDirectionDomain *domain)
    { return this->GetNthAxisDirectionValueAndDomain(0, value, domain); }
  void SetXAxisDirectionValue(AxisDirection value)
    { this->SetNthAxisDirectionValue(0, value); }

  // Model for the Y-coordinate axis
  bool GetYAxisDirectionValueAndDomain(
      AxisDirection &value, AxisDirectionDomain *domain)
    { return this->GetNthAxisDirectionValueAndDomain(1, value, domain); }
  void SetYAxisDirectionValue(AxisDirection value)
    { this->SetNthAxisDirectionValue(1, value); }

  // Model for the Z-coordinate axis
  bool GetZAxisDirectionValueAndDomain(
      AxisDirection &value, AxisDirectionDomain *domain)
    { return this->GetNthAxisDirectionValueAndDomain(2, value, domain); }
  void SetZAxisDirectionValue(AxisDirection value)
    { this->SetNthAxisDirectionValue(2, value); }

  // Respond to events received by the model
  virtual void OnUpdate();

  // The parent model
  GlobalUIModel *m_Parent;





};

#endif // REORIENTIMAGEMODEL_H
