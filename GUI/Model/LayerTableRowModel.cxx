#include "LayerTableRowModel.h"
#include "GlobalUIModel.h"
#include "IRISApplication.h"
#include "GenericImageData.h"
#include "ImageWrapperBase.h"
#include "NumericPropertyToggleAdaptor.h"
#include "DisplayMappingPolicy.h"
#include "DisplayLayoutModel.h"
#include "ImageIOWizardModel.h"

LayerTableRowModel::LayerTableRowModel()
{
  m_LayerOpacityModel = wrapGetterSetterPairAsProperty(
        this,
        &Self::GetLayerOpacityValueAndRange,
        &Self::SetLayerOpacityValue);

  m_NicknameModel = wrapGetterSetterPairAsProperty(
        this,
        &Self::GetNicknameValue,
        &Self::SetNicknameValue);

  m_ComponentNameModel = wrapGetterSetterPairAsProperty(
        this,
        &Self::GetComponentNameValue);

  m_StickyModel = wrapGetterSetterPairAsProperty(
        this,
        &Self::GetStickyValue,
        &Self::SetSticklyValue);

  m_VisibilityToggleModel = NewNumericPropertyToggleAdaptor(
        m_LayerOpacityModel.GetPointer(), 0, 50);

  m_LayerRole = -1;
  m_LayerPositionInRole = -1;
}

bool LayerTableRowModel::CheckState(UIState state)
{
  // Are we in tiling mode?
  bool tiling = (
        m_ParentModel->GetDisplayLayoutModel()->GetSliceViewLayerLayout() ==
        DisplayLayoutModel::LAYOUT_TILED);

  // Check the states
  switch(state)
    {
    // Opacity can be edited for all layers except the main image layer
    case LayerTableRowModel::UIF_OPACITY_EDITABLE:
      return (m_LayerRole != LayerIterator::MAIN_ROLE);
    case LayerTableRowModel::UIF_PINNABLE:
      return (m_LayerRole != LayerIterator::MAIN_ROLE && tiling);
    case LayerTableRowModel::UIF_MOVABLE_UP:
      return (m_LayerRole == LayerIterator::OVERLAY_ROLE
              && m_LayerPositionInRole > 0);
    case LayerTableRowModel::UIF_MOVABLE_DOWN:
      return (m_LayerRole == LayerIterator::OVERLAY_ROLE
              && m_LayerPositionInRole < m_LayerNumberOfLayersInRole - 1);
    }

  return false;
}

void LayerTableRowModel::UpdateRoleInfo()
{
  LayerIterator it(m_ParentModel->GetDriver()->GetCurrentImageData());
  it.Find(m_Layer);
  m_LayerRole = (int) it.GetRole();
  m_LayerPositionInRole = it.GetPositionInRole();
  m_LayerNumberOfLayersInRole = it.GetNumberOfLayersInRole();
}

void LayerTableRowModel::Initialize(GlobalUIModel *parentModel, ImageWrapperBase *layer)
{
  m_ParentModel = parentModel;
  m_Layer = layer;

  // For some of the functions, it is useful to know the role and the index
  // in the role of this layer. We shouldn't have to worry about this info
  // changing, since the rows get rebuilt when LayerChangeEvent() is fired.
  UpdateRoleInfo();

  // Listen to cosmetic events from the layer
  Rebroadcast(layer, WrapperMetadataChangeEvent(), ModelUpdateEvent());
  Rebroadcast(layer, WrapperDisplayMappingChangeEvent(), ModelUpdateEvent());

  // What happens if the layer is deleted? The model should be notified, and
  // it should update its state to a NULL state before something bad happens
  // in the GUI...
  Rebroadcast(layer, itk::DeleteEvent(), ModelUpdateEvent());

  // The state of this model only depends on wrapper's position in the list of
  // layers, not on the wrapper metadata
  Rebroadcast(m_ParentModel->GetDriver(), LayerChangeEvent(),
              StateMachineChangeEvent());

  // The state also depends on the current tiling mode
  Rebroadcast(m_ParentModel->GetDisplayLayoutModel()->GetSliceViewLayerLayoutModel(),
              ValueChangedEvent(),
              StateMachineChangeEvent());

}

void LayerTableRowModel::MoveLayerUp()
{
  m_ParentModel->GetDriver()->ChangeOverlayPosition(m_Layer, -1);
}

void LayerTableRowModel::MoveLayerDown()
{
  m_ParentModel->GetDriver()->ChangeOverlayPosition(m_Layer, +1);
}

#include "SNAPImageData.h"

SmartPtr<ImageIOWizardModel> LayerTableRowModel::CreateIOWizardModelForSave()
{
  // TODO: need some unified way of handling histories and categories

  // Which history does the image belong under? This goes beyond the role
  // of the image, as in snake mode, there are sub-roles that the wrappers
  // have. The safest thing is to have the history information be stored
  // as a kind of user data in each wrapper. However, for now, we will just
  // infer it from the role and type
  std::string history, category;
  if(m_LayerRole == LayerIterator::MAIN_ROLE)
    {
    history = "AnatomicImage";
    category = "Main Image";
    }

  else if(m_LayerRole == LayerIterator::OVERLAY_ROLE)
    {
    history = "AnatomicImage";
    category = "Overlay Image";
    }

  else if(m_LayerRole == LayerIterator::SNAP_ROLE)
    {
    if(dynamic_cast<SpeedImageWrapper *>(m_Layer))
      {
      history = "SpeedImage";
      category = "Speed Image";
      }

    else if(dynamic_cast<LevelSetImageWrapper *>(m_Layer))
      {
      history = "LevelSetImage";
      category = "Level Set Image";
      }
    }

  // Create delegate
  SmartPtr<DefaultSaveImageDelegate> delegate
      = DefaultSaveImageDelegate::New();
  delegate->Initialize(GetParentModel(),m_Layer,history);

  // Create a model for IO
  SmartPtr<ImageIOWizardModel> modelIO = ImageIOWizardModel::New();
  modelIO->InitializeForSave(GetParentModel(), delegate,
                           history.c_str(), category.c_str());

  return modelIO;
}


bool LayerTableRowModel::GetNicknameValue(std::string &value)
{
  if(!m_Layer) return false;

  value = m_Layer->GetNickname();
  return true;
}

void LayerTableRowModel::SetNicknameValue(std::string value)
{
  m_Layer->SetCustomNickname(value);
}

bool LayerTableRowModel::GetComponentNameValue(std::string &value)
{
  if(!m_Layer) return false;

  // Get the name of the compomnent, or empty string if this is a uni-component
  // image
  if(m_Layer->GetNumberOfComponents() > 1)
    {
    AbstractMultiChannelDisplayMappingPolicy *dp = dynamic_cast<
        AbstractMultiChannelDisplayMappingPolicy *>(m_Layer->GetDisplayMapping());
    MultiChannelDisplayMode mode = dp->GetDisplayMode();

    if(mode.UseRGB)
      {
      value = "RGB";
      return true;
      }

    std::ostringstream oss;
    switch(mode.SelectedScalarRep)
      {
      case SCALAR_REP_COMPONENT:
        oss << "Component ";
        oss << (1 + mode.SelectedComponent);
        value = oss.str();
        return true;

      case SCALAR_REP_MAGNITUDE:
        value = "Magnitude";
        return true;

      case SCALAR_REP_MAX:
        value = "Maximum";
        return true;

      case SCALAR_REP_AVERAGE:
        value = "Average";
        return true;

      case NUMBER_OF_SCALAR_REPS:
        return false;
      };
    }

  return false;
}

void LayerTableRowModel::OnUpdate()
{
  // Has our layer been deleted?
  if(this->m_EventBucket->HasEvent(itk::DeleteEvent(), m_Layer))
    {
    m_Layer = NULL;
    m_LayerRole = -1;
    m_LayerPositionInRole = -1;
    m_LayerNumberOfLayersInRole = -1;
    }
}



bool LayerTableRowModel::GetLayerOpacityValueAndRange(int &value, NumericValueRange<int> *domain)
{
  if(!m_Layer) return false;

  value = (int)(100.0 * m_Layer->GetAlpha());
  if(domain)
    domain->Set(0, 100, 5);
  return true;
}

void LayerTableRowModel::SetLayerOpacityValue(int value)
{
  m_Layer->SetAlpha(value / 100.0);
}

bool LayerTableRowModel::GetStickyValue(bool &value)
{
  if(!m_Layer) return false;

  value = m_Layer->IsSticky();
  return true;
}

void LayerTableRowModel::SetSticklyValue(bool value)
{
  m_Layer->SetSticky(value);
}

