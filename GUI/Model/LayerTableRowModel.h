#ifndef LAYERTABLEROWMODEL_H
#define LAYERTABLEROWMODEL_H

#include "PropertyModel.h"

class ImageWrapperBase;
class GlobalUIModel;
class ImageIOWizardModel;

/**
 * @brief The LayerTableRowModel class
 *
 * This is a GUI model used to access image layer properties that can appear
 * in a list/table of layers. These properties include opacity, visibility,
 * nickname, etc.
 *
 * This model is different from models such as LayerGeneralPropertiesModel:
 * it has a one-to-one association with the layers, whereas those other models
 * have a 1-N association with the layers (just one model object, parameterized
 * by a pointer to the 'active' layer).
 */
class LayerTableRowModel : public AbstractModel
{
public:

  irisITKObjectMacro(LayerTableRowModel, AbstractModel)

  irisGetMacro(Layer, ImageWrapperBase *)

  irisGetMacro(ParentModel, GlobalUIModel *)

  /** A model for overall layer opacity (int, range 0..100) */
  irisRangedPropertyAccessMacro(LayerOpacity, int)

  /** A model for layer visibility (on/off, interacts with opacity) */
  irisSimplePropertyAccessMacro(VisibilityToggle, bool)

  /** A model for layer stickiness */
  irisSimplePropertyAccessMacro(Sticky, bool)

  /** A model for the nickname */
  irisSimplePropertyAccessMacro(Nickname, std::string)

  /** A model for the component name */
  irisSimplePropertyAccessMacro(ComponentName, std::string)

  /**
    States in which the model can be, which allow the activation and
    deactivation of various widgets in the interface
    */
  enum UIState {
    UIF_OPACITY_EDITABLE,
    UIF_PINNABLE,
    UIF_MOVABLE_UP,
    UIF_MOVABLE_DOWN
    };

  /** Check the state of the system */
  bool CheckState(UIState state);


  void Initialize(GlobalUIModel *parentModel, ImageWrapperBase *layer);

  /** Move layer up or down in its role. */
  void MoveLayerUp();
  void MoveLayerDown();

  /**
   * Create a temporary model for saving this image to a file, to use in
   * conjunction with an IO wizard
   */
  SmartPtr<ImageIOWizardModel> CreateIOWizardModelForSave();


protected:
  LayerTableRowModel();
  virtual ~LayerTableRowModel() {}

  // Parent model
  GlobalUIModel *m_ParentModel;

  // Layer
  ImageWrapperBase *m_Layer;

  // Role information (cached)
  int m_LayerRole, m_LayerPositionInRole, m_LayerNumberOfLayersInRole;

  // Visibility model
  SmartPtr<AbstractSimpleBooleanProperty> m_VisibilityToggleModel;

  // Stickly model
  SmartPtr<AbstractSimpleBooleanProperty> m_StickyModel;
  bool GetStickyValue(bool &value);
  void SetSticklyValue(bool value);

  // Opacity model
  SmartPtr<AbstractRangedIntProperty> m_LayerOpacityModel;
  bool GetLayerOpacityValueAndRange(int &value, NumericValueRange<int> *domain);
  void SetLayerOpacityValue(int value);

  // Nickname model
  SmartPtr<AbstractSimpleStringProperty> m_NicknameModel;
  bool GetNicknameValue(std::string &value);
  void SetNicknameValue(std::string value);

  // Component name model
  SmartPtr<AbstractSimpleStringProperty> m_ComponentNameModel;
  bool GetComponentNameValue(std::string &value);

  // Update our state in response to events from the layer
  virtual void OnUpdate();

  // Update cached role information
  void UpdateRoleInfo();
};

#endif // LAYERTABLEROWMODEL_H
