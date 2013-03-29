#ifndef LAYERGENERALPROPERTIESMODEL_H
#define LAYERGENERALPROPERTIESMODEL_H

#include "AbstractLayerAssociatedModel.h"
#include "PropertyModel.h"

class AbstractMultiChannelDisplayMappingPolicy;

/**
 * Properties maintained for each layer in the layer association
 */
class GeneralLayerProperties
{
public:
  irisGetSetMacro(ObserverTag, unsigned long)

  virtual ~GeneralLayerProperties() {}

protected:

  // Whether or not we are already listening to events from this layer
  unsigned long m_ObserverTag;
};

typedef AbstractLayerAssociatedModel<
    GeneralLayerProperties, ImageWrapperBase> LayerGeneralPropertiesModelBase;

class LayerGeneralPropertiesModel : public LayerGeneralPropertiesModelBase
{
public:
  irisITKObjectMacro(LayerGeneralPropertiesModel, LayerGeneralPropertiesModelBase)

  /**
   * This enum defines the selections that the user can make for display mode.
   * These selections roughly map to the VectorImageWrapperBase::ScalarRepresentation
   * but the RGB mode is handled differently here and there (because RGB mode does
   * is not a scalar representation).
   */
  enum DisplayMode {
    MODE_COMPONENT = 0, MODE_MAGNITUDE, MODE_MAX, MODE_AVERAGE, MODE_RGB
  };

  // Implementation of virtual functions from parent class
  void RegisterWithLayer(ImageWrapperBase *layer);
  void UnRegisterFromLayer(ImageWrapperBase *layer, bool being_deleted);

  // Parent model assignment override
  virtual void SetParentModel(GlobalUIModel *parent);

  // Function called in response to events
  virtual void OnUpdate();

  // Typedefs for the model for component selection
  typedef SimpleItemSetDomain<DisplayMode, std::string> DisplayModeDomain;
  typedef AbstractPropertyModel<DisplayMode, DisplayModeDomain> AbstractDisplayModeModel;

  // Models
  irisGetMacro(DisplayModeModel, AbstractDisplayModeModel *)
  irisGetMacro(SelectedComponentModel, AbstractRangedIntProperty *)
  irisGetMacro(AnimateModel, AbstractSimpleBooleanProperty *)

  /** A model for overall layer opacity (int, range 0..100) */
  irisRangedPropertyAccessMacro(LayerOpacity, int)

  /** A model for the layer visibility on/off state */
  irisSimplePropertyAccessMacro(LayerVisibility, bool)

  /** A model for the filename */
  irisSimplePropertyAccessMacro(Filename, std::string)

  /** A model for the nickname */
  irisSimplePropertyAccessMacro(Nickname, std::string)

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

  // Filename and nickname
  SmartPtr<AbstractSimpleStringProperty> m_FilenameModel;
  SmartPtr<AbstractSimpleStringProperty> m_NicknameModel;

  bool GetFilenameValue(std::string &value);

  bool GetNicknameValue(std::string &value);
  void SetNicknameValue(std::string value);

  // Get the current display settings
  VectorImageWrapperBase *GetLayerAsVector();
  AbstractMultiChannelDisplayMappingPolicy *GetMultiChannelDisplayPolicy();
};

#endif // LAYERGENERALPROPERTIESMODEL_H
