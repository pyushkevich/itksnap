#ifndef COMPONENTSELECTIONMODEL_H
#define COMPONENTSELECTIONMODEL_H

#include "AbstractLayerAssociatedModel.h"
#include "PropertyModel.h"

class AbstractMultiChannelDisplayMappingPolicy;

/**
 * Properties maintained for each layer in the layer association
 */
class ComponentSelectionLayerProperties
{
public:
  irisGetSetMacro(ObserverTag, unsigned long)

  virtual ~ComponentSelectionLayerProperties() {}

protected:

  // Whether or not we are already listening to events from this layer
  unsigned long m_ObserverTag;
};

typedef AbstractLayerAssociatedModel<
    ComponentSelectionLayerProperties,
    VectorImageWrapperBase> ComponentSelectionModelBase;

class ComponentSelectionModel : public ComponentSelectionModelBase
{
public:
  irisITKObjectMacro(ComponentSelectionModel, ComponentSelectionModelBase)

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
  void RegisterWithLayer(VectorImageWrapperBase *layer);
  void UnRegisterFromLayer(VectorImageWrapperBase *layer);

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

protected:

  ComponentSelectionModel();
  virtual ~ComponentSelectionModel();

  SmartPtr<AbstractDisplayModeModel> m_DisplayModeModel;
  bool GetDisplayModeValueAndRange(DisplayMode &value, DisplayModeDomain *domain);
  void SetDisplayModeValue(DisplayMode value);

  SmartPtr<AbstractRangedIntProperty> m_SelectedComponentModel;
  bool GetSelectedComponentValueAndRange(int &value, NumericValueRange<int> *domain);
  void SetSelectedComponentValue(int value);

  SmartPtr<AbstractSimpleBooleanProperty> m_AnimateModel;
  bool GetAnimateValue(bool &value);
  void SetAnimateValue(bool value);

  // Get the current display settings
  AbstractMultiChannelDisplayMappingPolicy *GetDisplayPolicy();
};

#endif // COMPONENTSELECTIONMODEL_H
