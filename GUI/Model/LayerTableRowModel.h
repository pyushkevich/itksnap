#ifndef LAYERTABLEROWMODEL_H
#define LAYERTABLEROWMODEL_H

#include "PropertyModel.h"

class ImageWrapperBase;
class GlobalUIModel;
class ImageIOWizardModel;
struct MultiChannelDisplayMode;
class GenericImageData;

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

  /** A model for the color map preset currently selected */
  irisSimplePropertyAccessMacro(ColorMapPreset, std::string)

  /** A model for the display mode */
  typedef AbstractPropertyModel<MultiChannelDisplayMode, TrivialDomain> AbstractDisplayModeModel;
  irisGetMacro(DisplayModeModel, AbstractDisplayModeModel *)

  /**
    States in which the model can be, which allow the activation and
    deactivation of various widgets in the interface
    */
  enum UIState {
    UIF_OPACITY_EDITABLE,
    UIF_PINNABLE,
    UIF_UNPINNABLE,
    UIF_MOVABLE_UP,
    UIF_MOVABLE_DOWN,
    UIF_CLOSABLE,
    UIF_COLORMAP_ADJUSTABLE,
    UIF_CONTRAST_ADJUSTABLE,
    UIF_MULTICOMPONENT
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

  /**
   * Whether closing the layer requires prompting for changes
   */
  bool IsMainLayer();

  /**
   * Mark the layer as selected
   */
  void SetSelected(bool selected);

  /**
   * Close the current layer
   */
  void CloseLayer();

  /** Auto-adjust contrast (via the IntensityCurveModel) */
  void AutoAdjustContrast();

  /**
   * Generate texture features from this layer
   * TODO: this is a placeholder for the future more complex functionality
   */
  void GenerateTextureFeatures();


  typedef std::list<MultiChannelDisplayMode> DisplayModeList;

  /** Get a mapping of available display modes to user-readable strings */
  irisGetMacro(AvailableDisplayModes, const DisplayModeList &)

  /** Get the printable name for a display mode */
  std::string GetDisplayModeString(const MultiChannelDisplayMode &mode);

protected:
  LayerTableRowModel();
  virtual ~LayerTableRowModel() {}

  // Parent model
  GlobalUIModel *m_ParentModel;

  // Generic image data object in which this layer lives.
  GenericImageData *m_ImageData;

  // Layer
  ImageWrapperBase *m_Layer;

  // Role information (cached)
  LayerRole m_LayerRole;

  int m_LayerPositionInRole, m_LayerNumberOfLayersInRole;

  // Cached list of display modes
  DisplayModeList m_AvailableDisplayModes;

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

  // Color map preset model
  SmartPtr<AbstractSimpleStringProperty> m_ColorMapPresetModel;
  bool GetColorMapPresetValue(std::string &value);
  void SetColorMapPresetValue(std::string value);

  // Display mode model
  SmartPtr<AbstractDisplayModeModel> m_DisplayModeModel;
  bool GetDisplayModeValue(MultiChannelDisplayMode &value);
  void SetDisplayModeValue(MultiChannelDisplayMode value);

  // Update our state in response to events from the layer
  virtual void OnUpdate();

  // Update cached role information
  void UpdateRoleInfo();

  // Update cached display modes list
  void UpdateDisplayModeList();

  // Get the display mode
  MultiChannelDisplayMode GetDisplayMode();
};

#endif // LAYERTABLEROWMODEL_H
