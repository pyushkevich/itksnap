#ifndef DISPLAYLAYOUTMODEL_H
#define DISPLAYLAYOUTMODEL_H

class GlobalUIModel;

#include "PropertyModel.h"
#include "GlobalState.h"

/**
 * @brief A model that manages display layout properties. This model is
 * just a collection of specific properties
 */
class DisplayLayoutModel : public AbstractModel
{
public:

  // ITK macros
  irisITKObjectMacro(DisplayLayoutModel, AbstractModel)

  // Parent event type for all events fired by this model
  itkEventMacro(DisplayLayoutChangeEvent, IRISEvent)

  // Event fired when the layout of the slice panels changes
  itkEventMacro(ViewPanelLayoutChangeEvent, DisplayLayoutChangeEvent)

  // Event fired when the layout of the overlays in a slice panel changes
  itkEventMacro(LayerLayoutChangeEvent, DisplayLayoutChangeEvent)

  // This model fires a ModelUpdateEvent whenever there is a change in the
  // display layout
  FIRES(ViewPanelLayoutChangeEvent)
  FIRES(LayerLayoutChangeEvent)

  /** Layout of the SNAP slice views */
  enum ViewPanelLayout {
    VIEW_ALL = 0, VIEW_AXIAL, VIEW_CORONAL, VIEW_SAGITTAL, VIEW_3D
  };

  typedef AbstractPropertyModel<ViewPanelLayout> AbstractViewPanelLayoutProperty;

  /** Get the parent model */
  irisGetMacro(ParentModel, GlobalUIModel *)

  /** Assign the parent model */
  void SetParentModel(GlobalUIModel *parentModel);

  /** Model managing the view panel layouts */
  irisGetMacro(ViewPanelLayoutModel, AbstractViewPanelLayoutProperty *)

  /**
   * Read-only boolean property models for the visibility of any specific
   * view panel (0-2 are slice windows, 3 is the 3D window).
   */
  AbstractSimpleBooleanProperty *GetViewPanelVisibilityModel(int view) const
    { return m_ViewPanelVisibilityModel[view]; }

  /**
   * A model handing the layout of the layers in a slice view. This gives
   * the number of rows and columns into which the slice views are broken
   * up when displaying overlays. When this is 1x1, the overlays are painted
   * on top of each other using transparency. Otherwise, each overlay is shown
   * in its own cell.
   */
  irisGetMacro(SliceViewLayerTilingModel, AbstractSimpleUIntVec2Property *)

  /**
   * A model for the layout of the layers in a slice view. This model sets
   * the stacked/tiled state. If the state is set to tiled, the number of
   * tiles will be updated as the number of loaded images changes. This model
   * actually just returns the model with the same name in the global state
   */
  AbstractPropertyModel<LayerLayout, TrivialDomain> *GetSliceViewLayerLayoutModel() const;

  /**
   * Read-only boolean property models that dictate what icon should be
   * displayed in the expand/contract buttons in each slice view. There
   * are four of these models (one for each slice view), and they are of
   * the type ViewPanelLayout()
   */
  AbstractViewPanelLayoutProperty *GetViewPanelExpandButtonActionModel(int view) const
    { return m_ViewPanelExpandButtonActionModel[view]; }

protected:

  GlobalUIModel *m_ParentModel;

  typedef ConcretePropertyModel<ViewPanelLayout> ConcreteViewPanelLayoutProperty;
  SmartPtr<ConcreteViewPanelLayoutProperty> m_ViewPanelLayoutModel;

  SmartPtr<AbstractSimpleBooleanProperty> m_ViewPanelVisibilityModel[4];

  SmartPtr<AbstractViewPanelLayoutProperty> m_ViewPanelExpandButtonActionModel[4];

  bool GetNthViewPanelVisibilityValue(int panel, bool &value);

  SmartPtr<AbstractSimpleUIntVec2Property> m_SliceViewLayerTilingModel;
  bool GetSliceViewLayerTilingValue(Vector2ui &value);

  bool GetNthViewPanelExpandButtonActionValue(int panel, ViewPanelLayout &value);

  // Update the tiling based on the number of layouts in memory
  void UpdateSliceViewTiling();

  // The current tiling dimensons
  Vector2ui m_LayerTiling;

  virtual void OnUpdate();

  DisplayLayoutModel();
  virtual ~DisplayLayoutModel() {}
};

#endif // DISPLAYLAYOUTMODEL_H
