#include "DisplayLayoutModel.h"

DisplayLayoutModel::DisplayLayoutModel()
{
  // Initialize the view layout to four views
  m_ViewPanelLayoutModel = ConcreteViewPanelLayoutProperty::New();
  m_ViewPanelLayoutModel->SetValue(FOUR_VIEWS);

  // Cell layout model
  m_SliceViewCellLayoutModel = ConcreteSimpleUIntVec2Property::New();
  m_SliceViewCellLayoutModel->SetValue(Vector2ui(1,1));

  // Rebroadcast relevant events from child models
  Rebroadcast(m_ViewPanelLayoutModel, ValueChangedEvent(), ViewPanelLayoutChangeEvent());
  Rebroadcast(m_SliceViewCellLayoutModel, ValueChangedEvent(), OverlayLayoutChangeEvent());
}
