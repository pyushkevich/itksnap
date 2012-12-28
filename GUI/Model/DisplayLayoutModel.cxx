#include "DisplayLayoutModel.h"
#include "GlobalUIModel.h"
#include "IRISApplication.h"

DisplayLayoutModel::DisplayLayoutModel()
{
  // Initialize the view layout to four views
  m_ViewPanelLayoutModel = ConcreteViewPanelLayoutProperty::New();
  m_ViewPanelLayoutModel->SetValue(VIEW_ALL);

  // Cell layout model
  m_SliceViewCellLayoutModel = ConcreteSimpleUIntVec2Property::New();
  m_SliceViewCellLayoutModel->SetValue(Vector2ui(1,1));

  // Rebroadcast relevant events from child models
  Rebroadcast(m_ViewPanelLayoutModel, ValueChangedEvent(), ViewPanelLayoutChangeEvent());
  Rebroadcast(m_SliceViewCellLayoutModel, ValueChangedEvent(), OverlayLayoutChangeEvent());

  // Set up the boolean visibility models
  for(int panel = 0; panel < 4; panel++)
    {
    // Create the derived property for panel visibility
    m_ViewPanelVisibilityModel[panel] = wrapIndexedGetterSetterPairAsProperty(
          this, panel,
          &Self::GetNthViewPanelVisibilityValue);

    // The derived model must react to changes in view panel layout
    m_ViewPanelVisibilityModel[panel]->Rebroadcast(
          this, ViewPanelLayoutChangeEvent(), ValueChangedEvent());

    // Create a derived property for panel expand action
    m_ViewPanelExpandButtonActionModel[panel] = wrapIndexedGetterSetterPairAsProperty(
          this, panel,
          &Self::GetNthViewPanelExpandButtonActionValue);

    // The derived model must react to changes in view panel layout
    m_ViewPanelExpandButtonActionModel[panel]->Rebroadcast(
          this, ViewPanelLayoutChangeEvent(), ValueChangedEvent());
    }
}

void DisplayLayoutModel::SetParentModel(GlobalUIModel *parentModel)
{
  m_ParentModel = parentModel;

  // We need to rebroadcast the change in the display-to-anatomy mapping
  // as one of our own events, which will in turn be rebroadcast as value
  // change events in the derived models.
  Rebroadcast(m_ParentModel->GetDriver(),
              DisplayToAnatomyCoordinateMappingChangeEvent(),
              ViewPanelLayoutChangeEvent());
}

bool DisplayLayoutModel
::GetNthViewPanelVisibilityValue(int panel, bool &value)
{
  // The current layout
  ViewPanelLayout layout = m_ViewPanelLayoutModel->GetValue();

  // If the current mode is four views, then every view is visible
  if(layout == VIEW_ALL)
    {
    value = true;
    }

  // If asking about the 3D window, there is no need to check display orientation
  else if(panel == 3)
    {
    value = (layout == VIEW_3D);
    }

  // Otherwise, we need to know the orientation of the panel in question
  else
    {
    IRISApplication *driver = m_ParentModel->GetDriver();
    int wAxial = driver->GetDisplayWindowForAnatomicalDirection(ANATOMY_AXIAL);
    int wCoronal = driver->GetDisplayWindowForAnatomicalDirection(ANATOMY_CORONAL);
    int wSagittal = driver->GetDisplayWindowForAnatomicalDirection(ANATOMY_SAGITTAL);
    value = (layout == VIEW_AXIAL && panel == wAxial)
        || (layout == VIEW_CORONAL && panel == wCoronal)
        || (layout == VIEW_SAGITTAL && panel == wSagittal);
    }

  return true;
}

bool DisplayLayoutModel::GetNthViewPanelExpandButtonActionValue(
    int panel, DisplayLayoutModel::ViewPanelLayout &value)
{
  // The current layout
  ViewPanelLayout layout = m_ViewPanelLayoutModel->GetValue();

  // When the mode is 4-views, the action is to expand to the individual view
  if(layout == VIEW_ALL)
    {
    IRISApplication *driver = m_ParentModel->GetDriver();
    if(panel == (int) driver->GetDisplayWindowForAnatomicalDirection(ANATOMY_AXIAL))
      value = VIEW_AXIAL;
    else if (panel == (int) driver->GetDisplayWindowForAnatomicalDirection(ANATOMY_CORONAL))
      value = VIEW_CORONAL;
    else if (panel == (int) driver->GetDisplayWindowForAnatomicalDirection(ANATOMY_SAGITTAL))
      value = VIEW_SAGITTAL;
    else
      value = VIEW_3D;
    }

  // Otherwise, the action is to return back to 4 views
  else
    {
    value = VIEW_ALL;
    }

  return true;
}
