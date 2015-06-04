#ifndef CURSORINSPECTIONMODEL_H
#define CURSORINSPECTIONMODEL_H

#include <AbstractModel.h>
#include <PropertyModel.h>
#include <GenericImageData.h>
#include <AbstractLayerInfoItemSetDomain.h>

class GlobalUIModel;
class IRISApplication;

/**
  This structure describes the intensity information at a cursor location
  in a particular layer - which is displayed in the table
  */
struct LayerCurrentVoxelInfo
{
  std::string LayerName;
  std::string IntensityValue;
  Vector3ui Color;
  bool isSelectedGroundLayer;
  bool isSticky;
};

/**
  A definition of the item set domain for the table that shows current
  under the cursor intensity values
  */
class CurrentVoxelInfoItemSetDomain :
  public AbstractLayerInfoItemSetDomain<LayerCurrentVoxelInfo>
{
public:

  typedef AbstractLayerInfoItemSetDomain<LayerCurrentVoxelInfo> Superclass;

  // Define the domain with the specificed filter
  CurrentVoxelInfoItemSetDomain(
      GlobalUIModel *model = NULL, int role_filter = ALL_ROLES);

  // Define the description method
  LayerCurrentVoxelInfo GetDescription(const LayerIterator &it) const;

protected:
  GlobalUIModel *m_Model;
};

// A concrete model encapsulating the above domain
typedef ConcretePropertyModel<int, CurrentVoxelInfoItemSetDomain>
  ConcreteLayerVoxelAtCursorModel;

/**
  This little model handles the logic for the cursor inspection page
  */
class CursorInspectionModel : public AbstractModel
{
public:

  irisITKObjectMacro(CursorInspectionModel, AbstractModel)

  void SetParentModel(GlobalUIModel *parent);

  irisGetMacro(Parent, GlobalUIModel*)

  /** Get the model for the label under the cursor */
  irisGetMacro(LabelUnderTheCursorIdModel, AbstractSimpleLabelTypeProperty*)

  /** Get the model for the label description under the cursor */
  irisGetMacro(LabelUnderTheCursorTitleModel, AbstractSimpleStringProperty*)

  /** Get the model for the cursor location */
  AbstractRangedUIntVec3Property *GetCursorPositionModel() const;

  // The model for a table of intensity values at cursor
  irisGetMacro(VoxelAtCursorModel, ConcreteLayerVoxelAtCursorModel *)

protected:

  CursorInspectionModel();

private:

  // Label under the cursor
  SmartPtr<AbstractSimpleLabelTypeProperty> m_LabelUnderTheCursorIdModel;
  bool GetLabelUnderTheCursorIdValue(LabelType &value);

  // Title of the label under the cursor
  SmartPtr<AbstractSimpleStringProperty> m_LabelUnderTheCursorTitleModel;
  bool GetLabelUnderTheCursorTitleValue(std::string &value);

  // A pointer to the parent's cusror model
  AbstractRangedUIntVec3Property *m_CursorPositionModel;

  // The model for the intensity table
  SmartPtr<ConcreteLayerVoxelAtCursorModel> m_VoxelAtCursorModel;

  // Parent
  GlobalUIModel *m_Parent;

};

#endif // CURSORINSPECTIONMODEL_H
