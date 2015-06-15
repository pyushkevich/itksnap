#include "DisplayLayoutModel.h"
#include "GlobalUIModel.h"
#include "IRISApplication.h"
#include "GenericImageData.h"

DisplayLayoutModel::DisplayLayoutModel()
{
  // Initialize the view layout to four views
  m_ViewPanelLayoutModel = ConcreteViewPanelLayoutProperty::New();
  m_ViewPanelLayoutModel->SetValue(VIEW_ALL);

  // Rebroadcast relevant events from child models
  Rebroadcast(m_ViewPanelLayoutModel, ValueChangedEvent(), ViewPanelLayoutChangeEvent());

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

  // The tiling model
  m_SliceViewLayerTilingModel = wrapGetterSetterPairAsProperty(
        this,
        &Self::GetSliceViewLayerTilingValue);

  // The derived model must react to changes to the internal values
  m_SliceViewLayerTilingModel->Rebroadcast(
        this, LayerLayoutChangeEvent(), ValueChangedEvent());

  // The number of ground levels model
  m_NumberOfGroundLevelLayersModel = wrapGetterSetterPairAsProperty(
        this, &Self::GetNumberOfGroundLevelLayersValue);

  // The derived model must react to changes to the internal values
  m_NumberOfGroundLevelLayersModel->Rebroadcast(
        this, LayerLayoutChangeEvent(), ValueChangedEvent());

  // Thumbnail size
  m_ThumbnailRelativeSizeModel = NewRangedConcreteProperty(16.0, 0.0, 40.0, 1.0);
  Rebroadcast(m_ThumbnailRelativeSizeModel, ValueChangedEvent(), LayerLayoutChangeEvent());
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

  // We need to be notified when the number of overlays changes
  Rebroadcast(m_ParentModel->GetDriver(),
              LayerChangeEvent(), LayerLayoutChangeEvent());

  // and when the tiled/stacked mode changes
  Rebroadcast(m_ParentModel->GetGlobalState()->GetSliceViewLayerLayoutModel(),
              ValueChangedEvent(), LayerLayoutChangeEvent());

  // We also need notification when the layer stickiness changes
  Rebroadcast(m_ParentModel->GetDriver(),
              WrapperVisibilityChangeEvent(), LayerLayoutChangeEvent());

}

AbstractPropertyModel<LayerLayout, TrivialDomain> *
DisplayLayoutModel::GetSliceViewLayerLayoutModel() const
{
  return m_ParentModel->GetGlobalState()->GetSliceViewLayerLayoutModel();
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

bool DisplayLayoutModel::GetSliceViewLayerTilingValue(Vector2ui &value)
{
  value = m_LayerTiling;
  return true;
}

void DisplayLayoutModel::UpdateSliceViewTiling()
{
  GenericImageData *id = m_ParentModel->GetDriver()->GetCurrentImageData();

  // In stacked layout, there is only one layer to draw
  if(m_ParentModel->GetGlobalState()->GetSliceViewLayerLayout() == LAYOUT_STACKED)
    {
    m_LayerTiling.fill(1);
    }

  // Also, when there is no data, the layout is 1x1
  else if(!m_ParentModel->GetDriver()->IsMainImageLoaded())
    {
    m_LayerTiling.fill(1);
    }

  // Otherwise, we need to figure out in a smart way... But for now, we
  // should just use some default tiling scheme
  else
    {
    // Count the number of non-sticky layers, always counting main layer as 1
    int n = 0;
    for(LayerIterator it = id->GetLayers(); !it.IsAtEnd(); ++it)
      {
      if(it.GetRole() == MAIN_ROLE || !it.GetLayer()->IsSticky())
        n++;
      }

    // A simple algorithm to solve min(ab) s.t. ab >= n, k >= a/b >= 1, where
    // k is an aspect ratio value
    // TODO: replace this with a smart algorithm that uses current window size
    // and the amount of white space wasted in the currently visible views?
    m_LayerTiling.fill(n);
    double k = 2.01;
    for(unsigned int a = 1; a <= n; a++)
      {
      unsigned int b0 = (unsigned int)(ceil(a/k));
      for(unsigned int b = b0; b <= a; b++)
        {
        if(a * b >= n)
          {
          if(a * b < m_LayerTiling[1] * m_LayerTiling[0])
            {
            m_LayerTiling[1] = a;
            m_LayerTiling[0] = b;
            }
          }
        }
      }
    }
}

bool DisplayLayoutModel::GetNumberOfGroundLevelLayersValue(int &value)
{
  if(!m_ParentModel->GetDriver()->IsMainImageLoaded())
    return false;

  value = 0;
  GenericImageData *id = m_ParentModel->GetDriver()->GetCurrentImageData();
  for(LayerIterator it = id->GetLayers(); !it.IsAtEnd(); ++it)
    {
    if(it.GetRole() == MAIN_ROLE || !it.GetLayer()->IsSticky())
      value++;
    }

  return true;
}

void DisplayLayoutModel::OnUpdate()
{
  // If there has been a layer change event, we need to recompute the tiling
  // model
  if(m_EventBucket->HasEvent(LayerChangeEvent())
     || m_EventBucket->HasEvent(ValueChangedEvent(),
                                m_ParentModel->GetGlobalState()->GetSliceViewLayerLayoutModel())
     || m_EventBucket->HasEvent(WrapperVisibilityChangeEvent())
     )
    {
    this->UpdateSliceViewTiling();
    }
}




