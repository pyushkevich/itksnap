#ifndef LAYERTABLEROWMODEL_H
#define LAYERTABLEROWMODEL_H

#include "PropertyModel.h"

class ImageWrapperBase;
class MeshWrapperBase;
class WrapperBase;
class GlobalUIModel;
class ImageIOWizardModel;
struct MultiChannelDisplayMode;
class GenericImageData;
class AbstractReloadWrapperDelegate;
class IRISWarningList;

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
class AbstractLayerTableRowModel : public AbstractModel
{
public:
  irisITKAbstractObjectMacro(AbstractLayerTableRowModel, AbstractModel)

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
    UIF_SAVABLE,
    UIF_COLORMAP_ADJUSTABLE,
    UIF_CONTRAST_ADJUSTABLE,
    UIF_MULTICOMPONENT,
    UIF_IS_4D, // the 4th dimension greater than 1
    UIF_VOLUME_RENDERABLE,
    UIF_IMAGE,
    UIF_MESH,
    UIF_MESH_HAS_DATA, // mesh has data for color rendering
    UIF_FILE_RELOADABLE // has a corresponding file for reloading
    };

  // ----------------------------------------------
  // Begin virtual methods implementation

  /**
   *  Contains common initializing logic
   *  Implement in a subclass to do wrapper specific logic
   *  Important: Call parent method at the end of subclass's Initialize method
  */
  virtual void Initialize(GlobalUIModel *parentModel, WrapperBase *layer);

  /** Implement in a subclass to Check the state of the system */
  virtual bool CheckState(UIState state);

  /**
   * Mark the layer as selected. For non-segmentation images this makes them be displayed in the
   * non-tiled mode. For segmentation images, it makes it the segmentaiton image that is being edited
   * For meshes, it will make the mesh be rendered in the 3D view
   */
  virtual void SetActivated(bool activated) = 0;

  /**
   * Test if the layer is currently selected in its role or group of roles. We can have an anatomical
   * image selected at the same time as a segmentation image and a mesh is selected
   */
  virtual bool IsActivated() const = 0;

  /**
   * Close the current layer
   */
  virtual void CloseLayer() = 0;

  /** Auto-adjust contrast (via the IntensityCurveModel) */
  virtual void AutoAdjustContrast() = 0;

  // End of virtual methods implementation
  // ----------------------------------------------

  /** Getter of Parent Model */
  irisGetMacro(ParentModel, GlobalUIModel *)

  /** Getter of Layer. User can downcast the type to more concrete type based on IsA result */
  irisGetMacro(Layer, WrapperBase *)

  /** A model for the color map preset currently selected */
  irisSimplePropertyAccessMacro(ColorMapPreset, std::string)

  /** A model for overall layer opacity (int, range 0..100) */
  irisRangedPropertyAccessMacro(LayerOpacity, int)

  /** A model for layer visibility (on/off, interacts with opacity) */
  irisSimplePropertyAccessMacro(VisibilityToggle, bool)

  /** A model for the nickname */
  irisSimplePropertyAccessMacro(Nickname, std::string)

  /** A model for layer stickiness */
  irisSimplePropertyAccessMacro(Sticky, bool)

  /**
   * Whether closing the layer requires prompting for changes
   */
  bool IsMainLayer();

  /**
   * Reload wrapper data from the file on disk
   */
  virtual void ReloadWrapperFromFile(IRISWarningList &wl) = 0;

protected:
  AbstractLayerTableRowModel();
  virtual ~AbstractLayerTableRowModel() {}

  // ------------------------------------------
  //  Begin virtual methods implementation

  /** Implement in subclass to update cached role information */
  virtual void UpdateRoleInfo() = 0;

  /** Implement in subclass to test if layer is sticky */
  SmartPtr<AbstractSimpleBooleanProperty> m_StickyModel;
  virtual bool GetStickyValue(bool &value) = 0;
  virtual void SetStickyValue(bool value) = 0;

  /** Implement in subclass to access the layer's color map */
  SmartPtr<AbstractSimpleStringProperty> m_ColorMapPresetModel;
  virtual bool GetColorMapPresetValue(std::string &value) = 0;
  virtual void SetColorMapPresetValue(std::string value) = 0;


  //  End of virtual methods implementation
  // ------------------------------------------

  // Parent model
  GlobalUIModel *m_ParentModel;

  // Layer Wrapper
  SmartPtr<WrapperBase> m_Layer;

  // Generic image data object in which this layer lives.
  GenericImageData *m_ImageData;

  // Role information (cached)
  LayerRole m_LayerRole;

  int m_LayerPositionInRole, m_LayerNumberOfLayersInRole;

  // Visibility model
  SmartPtr<AbstractSimpleBooleanProperty> m_VisibilityToggleModel;

  // Opacity model
  SmartPtr<AbstractRangedIntProperty> m_LayerOpacityModel;
  bool GetLayerOpacityValueAndRange(int &value, NumericValueRange<int> *domain);
  void SetLayerOpacityValue(int value);

  // Nickname model
  SmartPtr<AbstractSimpleStringProperty> m_NicknameModel;
  bool GetNicknameValue(std::string &value);
  void SetNicknameValue(std::string value);

  // Update our state in response to events from the layer
  virtual void OnUpdate() override;

};

/**
 * \brief The ImageLayerTableModel class
 *
 */
class ImageLayerTableRowModel : public AbstractLayerTableRowModel
{
public:
  irisITKObjectMacro(ImageLayerTableRowModel, AbstractLayerTableRowModel)

  /** A model for volume rendering on/off */
  irisSimplePropertyAccessMacro(VolumeRenderingEnabled, bool)

  // ----------------------------------------------
  // Virtual methods implementation

  /**
   *  Implement in a subclass to do wrapper specific logic
   */
  void Initialize(GlobalUIModel *parentModel, WrapperBase *layer) override;

  /** Implement in a subclass to Check the state of the system */
  bool CheckState(UIState state) override;

  /**
   * Mark the layer as selected. For non-segmentation images this makes them be displayed in the
   * non-tiled mode. For segmentation images, it makes it the segmentaiton image that is being edited
   * For meshes, it will make the mesh be rendered in the 3D view
   */
  void SetActivated(bool activated) override;

  /**
   * Test if the layer is currently selected in its role or group of roles. We can have an anatomical
   * image selected at the same time as a segmentation image and a mesh is selected
   */
  bool IsActivated() const override;

  /**
   * Close the current layer
   */
  void CloseLayer() override;

  /** Auto-adjust contrast (via the IntensityCurveModel) */
  void AutoAdjustContrast() override;

  // End of virtual methods implementation
  // ----------------------------------------------

  /** A model for the component name */
  irisSimplePropertyAccessMacro(ComponentName, std::string)

  /** A model for the display mode */
  typedef AbstractPropertyModel<MultiChannelDisplayMode, TrivialDomain> AbstractDisplayModeModel;
  irisGetMacro(DisplayModeModel, AbstractDisplayModeModel *)

  /** Move layer up or down in its role. */
  void MoveLayerUp();
  void MoveLayerDown();

  /** Get the printable name for a display mode */
  std::string GetDisplayModeString(const MultiChannelDisplayMode &mode);

  typedef std::list<MultiChannelDisplayMode> DisplayModeList;

  /** Get a mapping of available display modes to user-readable strings */
  irisGetMacro(AvailableDisplayModes, const DisplayModeList &)

  /**
   * Create a temporary model for saving this image to a file, to use in
   * conjunction with an IO wizard
   */
  SmartPtr<ImageIOWizardModel> CreateIOWizardModelForSave();

  /**
   * Generate texture features from this layer
   * TODO: this is a placeholder for the future more complex functionality
   */
  void GenerateTextureFeatures();

  /**
   * Reload current 4d image layer to a multi-component image layer
   */
  void ReloadAsMultiComponent();

  /**
   * Reload current multi-component image layer to a 4d image layer
   */
  void ReloadAs4D();

  /**
   * Reload wrapper data from the file on disk
   */
  void ReloadWrapperFromFile(IRISWarningList &wl) ITK_OVERRIDE;

protected:
  ImageLayerTableRowModel();
  virtual ~ImageLayerTableRowModel() = default;

  // ------------------------------------------
  //  Virtual methods implementation

  /** Implement in subclass to update cached role information */
  void UpdateRoleInfo() override;

  /** Implement in subclass to test if layer is sticky */
  bool GetStickyValue(bool &value) override;
  void SetStickyValue(bool value) override;

  /** Implement in subclass to access the layer's color map */
  bool GetColorMapPresetValue(std::string &value) override;
  void SetColorMapPresetValue(std::string value) override;

  //  End of virtual methods implementation
  // ------------------------------------------

  /** Called by initialize() to configure m_AvailableDisplayModes */
  void UpdateDisplayModeList();

  /** Get the display mode */
  MultiChannelDisplayMode GetDisplayMode();

  // Component name model
  SmartPtr<AbstractSimpleStringProperty> m_ComponentNameModel;
  bool GetComponentNameValue(std::string &value);

  // Display mode model
  SmartPtr<AbstractDisplayModeModel> m_DisplayModeModel;
  bool GetDisplayModeValue(MultiChannelDisplayMode &value);
  void SetDisplayModeValue(MultiChannelDisplayMode value);

  // Volume rendering enabled model
  SmartPtr<AbstractSimpleBooleanProperty> m_VolumeRenderingEnabledModel;
  bool GetVolumeRenderingEnabledValue(bool &value);
  void SetVolumeRenderingEnabledValue(bool value);

  // Cached list of display modes
  DisplayModeList m_AvailableDisplayModes;

  SmartPtr<ImageWrapperBase> m_ImageLayer;
};


/** \brief The MeshLayerRowModel class
 *
 */

class MeshLayerTableRowModel : public AbstractLayerTableRowModel
{
public:
  irisITKObjectMacro(MeshLayerTableRowModel, AbstractLayerTableRowModel)

  // ----------------------------------------------
  // Begin virtual methods implementation

  /** Implement in a subclass to do wrapper specific logic */
  void Initialize(GlobalUIModel *parentModel, WrapperBase *layer) override;

  /** Implement in a subclass to Check the state of the system */
  bool CheckState(UIState state) override;

  /**
   * Mark the layer as selected. For non-segmentation images this makes them be displayed in the
   * non-tiled mode. For segmentation images, it makes it the segmentaiton image that is being edited
   * For meshes, it will make the mesh be rendered in the 3D view
   */
  void SetActivated(bool activated) override;

  /**
   * Test if the layer is currently selected in its role or group of roles. We can have an anatomical
   * image selected at the same time as a segmentation image and a mesh is selected
   */
  bool IsActivated() const override;

  /**
   * Close the current layer
   */
  void CloseLayer() override;

  /** Auto-adjust contrast (via the IntensityCurveModel) */
  void AutoAdjustContrast() override;

  /**
   * Reload wrapper data from the file on disk
   */
  void ReloadWrapperFromFile(IRISWarningList &wl) ITK_OVERRIDE;

  // End of virtual methods implementation
  // ----------------------------------------------

protected:
  MeshLayerTableRowModel();
  virtual ~MeshLayerTableRowModel() = default;

  // ------------------------------------------
  //  Virtual methods implementation

  /** Implement in subclass to update cached role information */
  void UpdateRoleInfo() override;

  /** Implement in subclass to test if layer is sticky */
  bool GetStickyValue(bool &value) override;
  void SetStickyValue(bool value) override;

  /** Implement in subclass to access the layer's color map */
  bool GetColorMapPresetValue(std::string &value) override;
  void SetColorMapPresetValue(std::string value) override;

  //  End of virtual methods implementation
  // ------------------------------------------

  SmartPtr<MeshWrapperBase> m_MeshLayer;
};

#endif // LAYERTABLEROWMODEL_H
