#ifndef LAYERGENERALPROPERTIESMODEL_H
#define LAYERGENERALPROPERTIESMODEL_H

#include "AbstractLayerAssociatedModel.h"
#include "PropertyModel.h"
#include "TagList.h"
#include "MeshWrapperBase.h"
#include "MeshDataArrayProperty.h"
#include "LayerTableRowModel.h"

class AbstractMultiChannelDisplayMappingPolicy;
class TimePointProperties;

/**
 * Properties maintained for each layer in the layer association
 */
class GeneralLayerProperties
{
public:
  irisGetSetMacro(ObserverTag, unsigned long)
  irisGetSetMacro(HistogramChangeObserverTag, unsigned long)
  irisGetSetMacro(VisibilityToggleModel, AbstractSimpleBooleanProperty *)

  GeneralLayerProperties()
    : m_VisibilityToggleModel(NULL), m_ObserverTag(0), m_HistogramChangeObserverTag(0) {}

  virtual ~GeneralLayerProperties() {}

protected:

  // The visibility toggle model for this layer. This model must remember the
  // previous opacity value (and restore it when toggled from off to on) so it
  // has to be associated with each layer
  SmartPtr<AbstractSimpleBooleanProperty> m_VisibilityToggleModel;

  // Whether or not we are already listening to events from this layer
  unsigned long m_ObserverTag, m_HistogramChangeObserverTag;
};

typedef AbstractLayerAssociatedModel<
    GeneralLayerProperties, WrapperBase> LayerGeneralPropertiesModelBase;

class LayerGeneralPropertiesModel : public LayerGeneralPropertiesModelBase
{
public:
  irisITKObjectMacro(LayerGeneralPropertiesModel, LayerGeneralPropertiesModelBase)

  /**
   * This enum defines the selections that the user can make for display mode.
   * These selections roughly map to the ScalarRepresentation
   * but the RGB mode is handled differently here and there (because RGB mode does
   * is not a scalar representation).
   */
  enum DisplayMode {
    MODE_COMPONENT = 0, MODE_MAGNITUDE, MODE_MAX, MODE_AVERAGE, MODE_RGB, MODE_GRID
  };

  /** States for this model */
  enum UIState {
    UIF_MULTICOMPONENT,
    UIF_CAN_SWITCH_COMPONENTS,
    UIF_IS_STICKINESS_EDITABLE,
    UIF_IS_OPACITY_EDITABLE,
    UIF_MOVABLE_UP,
    UIF_MOVABLE_DOWN,
    UIF_IS_4D_IMAGE,
    UIF_IS_MESH,
    UIF_IS_MESHDATA_SOLID_COLOR,
    UIF_IS_MESHDATA_MULTICOMPONENT,
		UIF_MESH_HAS_DATA
  };

	/** For UI layer to map mesh data type to resources */
	typedef MeshDataArrayProperty::MeshDataType MeshDataType;

  // Implementation of virtual functions from parent class
  void RegisterWithLayer(WrapperBase *layer) override;
  void UnRegisterFromLayer(WrapperBase *layer, bool being_deleted) override;

  // Parent model assignment override
  virtual void SetParentModel(GlobalUIModel *parent);

  // Function called in response to events
  virtual void OnUpdate() override;

  // State management
  bool CheckState(UIState state);

  // Typedefs for the model for component selection
  typedef SimpleItemSetDomain<DisplayMode, std::string> DisplayModeDomain;
  typedef AbstractPropertyModel<DisplayMode, DisplayModeDomain> AbstractDisplayModeModel;

  typedef MeshWrapperBase::MeshLayerCombinedPropertyMap MeshLayerDataPropertiesMap;

  // Models
  irisGetMacro(DisplayModeModel, AbstractDisplayModeModel *)
  irisGetMacro(SelectedComponentModel, AbstractRangedIntProperty *)
  irisGetMacro(AnimateModel, AbstractSimpleBooleanProperty *)

  /** A model for overall layer opacity (int, range 0..100) */
  irisRangedPropertyAccessMacro(LayerOpacity, int)

  /** A model for the layer visibility on/off state */
  irisSimplePropertyAccessMacro(LayerVisibility, bool)

  /** A model for the stickiness */
  irisSimplePropertyAccessMacro(IsSticky, bool)

  /** A model for the filename */
  irisSimplePropertyAccessMacro(Filename, std::string)

  /** A model for the nickname */
  irisSimplePropertyAccessMacro(Nickname, std::string)

  /** A model for the tags */
  irisSimplePropertyAccessMacro(Tags, TagList)

  /** A model for the current timepoint nickname */
  irisSimplePropertyAccessMacro(CrntTimePointNickname, std::string)

  /** A model for the current timepoint taglist */
  irisSimplePropertyAccessMacro(CrntTimePointTagList, TagList)

  /** Getter for Mesh Layer Data Array Properties */
  bool GetMeshDataArrayPropertiesMap(MeshLayerDataPropertiesMap &outmap);

  /** Model for the active data property (-1 is solid color) */
  irisGenericPropertyAccessMacro(ActiveMeshLayerDataPropertyId, int, MeshLayerTableRowModel::MeshDataArrayDomain)

  /** A model for mesh multi-component (vector) display mode */
  irisGenericPropertyAccessMacro(MeshVectorMode, vtkIdType, MeshLayerTableRowModel::MeshVectorModeDomain)

  /** Model for the current mesh solid color */
  irisSimplePropertyAccessMacro(MeshSolidColor, Vector3d)

  /** Model for the current mesh view opacity (0-100 range) */
  irisRangedPropertyAccessMacro(MeshSliceViewOpacity, int)

  /** Move the layer up in the list */
  void MoveLayerUp();
  void MoveLayerDown();

protected:

  LayerGeneralPropertiesModel();
  virtual ~LayerGeneralPropertiesModel();

  SmartPtr<AbstractDisplayModeModel> m_DisplayModeModel;
  bool GetDisplayModeValueAndRange(DisplayMode &value, DisplayModeDomain *domain);
  void SetDisplayModeValue(DisplayMode value);

  SmartPtr<AbstractRangedIntProperty> m_SelectedComponentModel;
  bool GetSelectedComponentValueAndRange(int &value, NumericValueRange<int> *domain);
  void SetSelectedComponentValue(int value);

  SmartPtr<AbstractSimpleBooleanProperty> m_AnimateModel;
  bool GetAnimateValue(bool &value);
  void SetAnimateValue(bool value);

  // layer opacity and visibility models
  SmartPtr<AbstractRangedIntProperty> m_LayerOpacityModel;
  SmartPtr<AbstractSimpleBooleanProperty> m_LayerVisibilityModel;

  // Callbacks for the opacity model
  bool GetLayerOpacityValueAndRange(int &value, NumericValueRange<int> *domain);
  void SetLayerOpacityValue(int value);

  // Callbacks for the visibility model
  bool GetLayerVisibilityValue(bool &value);
  void SetLayerVisibilityValue(bool value);

  // Filename and nickname
  SmartPtr<AbstractSimpleStringProperty> m_FilenameModel;
  SmartPtr<AbstractSimpleStringProperty> m_NicknameModel;

  // Tags
  SmartPtr<AbstractSimpleTagListProperty> m_TagsModel;

  bool GetFilenameValue(std::string &value);

  bool GetNicknameValue(std::string &value);
  void SetNicknameValue(std::string value);

  bool GetTagsValue(TagList &value);
  void SetTagsValue(TagList value);

  // Stickiness
  SmartPtr<AbstractSimpleBooleanProperty> m_IsStickyModel;
  bool GetIsStickyValue(bool &value);
  void SetIsStickyValue(bool value);

  // Get the current display settings
  VectorImageWrapperBase *GetLayerAsVector();
  AbstractMultiChannelDisplayMappingPolicy *GetMultiChannelDisplayPolicy();

  // Get the LayerTableRowModel corresponding to the selected layer, or
  // NULL if no layer is selected. Some of the properties that this model
  // exposes are already exposed in LayerTableRowModel, so we delegate to
  // them.
  AbstractLayerTableRowModel *GetSelectedLayerTableRowModel();
  MeshLayerTableRowModel *GetSelectedMeshLayerTableRowModel();
  ImageLayerTableRowModel *GetSelectedImageLayerTableRowModel();

  // TimePoint Properties
  TimePointProperties *m_TimePointProperties;

  // Current time point nickname
  SmartPtr<AbstractSimpleStringProperty> m_CrntTimePointNicknameModel;
  bool GetCrntTimePointNicknameValue(std::string &value);
  void SetCrntTimePointNicknameValue(std::string value);

  // Current time point tags
  SmartPtr<AbstractSimpleTagListProperty> m_CrntTimePointTagListModel;
  bool GetCrntTimePointTagListValue(TagList &value);
  void SetCrntTimePointTagListValue(TagList value);

  // Mesh active data property
  using MeshLayerDataPropertyIdModel = AbstractPropertyModel<int, MeshLayerTableRowModel::MeshDataArrayDomain>;
  SmartPtr<MeshLayerDataPropertyIdModel> m_ActiveMeshLayerDataPropertyIdModel;
  bool GetActiveMeshLayerDataPropertyIdValueAndRange(int &value, MeshLayerTableRowModel::MeshDataArrayDomain *domain);
  void SetActiveMeshLayerDataPropertyIdValue(int value);

  using MeshVectorModeModel = AbstractPropertyModel<vtkIdType, MeshLayerTableRowModel::MeshVectorModeDomain>;
  SmartPtr<MeshVectorModeModel> m_MeshVectorModeModel;
  bool GetMeshVectorModeValueAndRange(vtkIdType &value, MeshLayerTableRowModel::MeshVectorModeDomain *domain);
  void SetMeshVectorModeValue(vtkIdType value);

  // Mesh solid color
  SmartPtr<AbstractSimpleDoubleVec3Property> m_MeshSolidColorModel;
  bool GetMeshSolidColorValue(Vector3d &value);
  void SetMeshSolidColorValue(const Vector3d value);

  // Mesh opacity in slice views
  SmartPtr<AbstractRangedIntProperty> m_MeshSliceViewOpacityModel;
  bool GetMeshSliceViewOpacityValueAndRange(int &value, NumericValueRange<int> *domain);
  void SetMeshSliceViewOpacityValue(int value);
};

#endif // LAYERGENERALPROPERTIESMODEL_H
