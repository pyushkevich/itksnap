#include "LayerGeneralPropertiesModel.h"
#include "DisplayMappingPolicy.h"
#include "LayerAssociation.txx"
#include "NumericPropertyToggleAdaptor.h"
#include "LayerTableRowModel.h"

template class LayerAssociation<GeneralLayerProperties,
                                ImageWrapperBase,
                                LayerGeneralPropertiesModel::PropertiesFactory>;


LayerGeneralPropertiesModel::LayerGeneralPropertiesModel()
{
  // Create the derived models
  m_DisplayModeModel = wrapGetterSetterPairAsProperty(
        this,
        &Self::GetDisplayModeValueAndRange,
        &Self::SetDisplayModeValue);

  m_SelectedComponentModel = wrapGetterSetterPairAsProperty(
        this,
        &Self::GetSelectedComponentValueAndRange,
        &Self::SetSelectedComponentValue);

  m_AnimateModel = wrapGetterSetterPairAsProperty(
        this,
        &Self::GetAnimateValue,
        &Self::SetAnimateValue);

  m_LayerOpacityModel = wrapGetterSetterPairAsProperty(
        this,
        &Self::GetLayerOpacityValueAndRange,
        &Self::SetLayerOpacityValue);

  m_LayerVisibilityModel = wrapGetterSetterPairAsProperty(
        this,
        &Self::GetLayerVisibilityValue,
        &Self::SetLayerVisibilityValue);

  m_FilenameModel = wrapGetterSetterPairAsProperty(
        this, &Self::GetFilenameValue);

  m_NicknameModel = wrapGetterSetterPairAsProperty(
        this,
        &Self::GetNicknameValue,
        &Self::SetNicknameValue);

  m_IsStickyModel = wrapGetterSetterPairAsProperty(
        this,
        &Self::GetIsStickyValue,
        &Self::SetIsStickyValue);

}

LayerGeneralPropertiesModel::~LayerGeneralPropertiesModel()
{

}


void
LayerGeneralPropertiesModel::SetParentModel(GlobalUIModel *parent)
{
  Superclass::SetParentModel(parent);

  this->Rebroadcast(
        parent->GetDriver(),
        WrapperMetadataChangeEvent(), ModelUpdateEvent());

  this->Rebroadcast(
        parent->GetDriver(),
        WrapperDisplayMappingChangeEvent(), ModelUpdateEvent());

  this->Rebroadcast(
        parent->GetDriver(),
        MainImageDimensionsChangeEvent(), ModelUpdateEvent());

  this->Rebroadcast(this, ModelUpdateEvent(), StateMachineChangeEvent());
}

void
LayerGeneralPropertiesModel::RegisterWithLayer(ImageWrapperBase *layer)
{
  // Listen to changes in the layer's intensity curve
  // TODO: maybe we need better event granularity here? This event will fire when
  // the intensity curve or colormap is changed too
  unsigned long tag =
      Rebroadcast(layer->GetDisplayMapping(),
                  itk::ModifiedEvent(), ModelUpdateEvent());

  // Set a flag so we don't register a listener again
  GetProperties().SetObserverTag(tag);  
}

void
LayerGeneralPropertiesModel::UnRegisterFromLayer(ImageWrapperBase *layer, bool being_deleted)
{
  if(!being_deleted)
    {
    // It's safe to call GetProperties()
    unsigned long tag = GetProperties().GetObserverTag();
    if(tag)
      {
      layer->GetDisplayMapping()->RemoveObserver(tag);
      }
    }
}

void
LayerGeneralPropertiesModel::OnUpdate()
{
  Superclass::OnUpdate();

  // Do we need this method?
}

bool LayerGeneralPropertiesModel::CheckState(LayerGeneralPropertiesModel::UIState state)
{
  if(!m_Layer)
    return false;

  // Defer to the row model when we can
  LayerTableRowModel *row_model = this->GetSelectedLayerTableRowModel();

  switch(state)
    {
    case UIF_CAN_SWITCH_COMPONENTS:
      {
        AbstractMultiChannelDisplayMappingPolicy *dp =
            dynamic_cast<AbstractMultiChannelDisplayMappingPolicy *>(
              m_Layer->GetDisplayMapping());
        return dp && dp->GetDisplayMode().SelectedScalarRep == SCALAR_REP_COMPONENT;
      }
    case UIF_MULTICOMPONENT:
      return m_Layer->GetNumberOfComponents() > 1;

    case UIF_IS_STICKINESS_EDITABLE:
      return row_model->CheckState(LayerTableRowModel::UIF_PINNABLE)
          || row_model->CheckState(LayerTableRowModel::UIF_UNPINNABLE);

    case UIF_IS_OPACITY_EDITABLE:
      return row_model->CheckState(LayerTableRowModel::UIF_OPACITY_EDITABLE);

    case UIF_MOVABLE_UP:
      return row_model->CheckState(LayerTableRowModel::UIF_MOVABLE_UP);

    case UIF_MOVABLE_DOWN:
      return row_model->CheckState(LayerTableRowModel::UIF_MOVABLE_DOWN);
    }

  return false;
}

void LayerGeneralPropertiesModel::MoveLayerUp()
{
  this->GetSelectedLayerTableRowModel()->MoveLayerUp();
}

void LayerGeneralPropertiesModel::MoveLayerDown()
{
  this->GetSelectedLayerTableRowModel()->MoveLayerDown();
}

bool LayerGeneralPropertiesModel
::GetDisplayModeValueAndRange(DisplayMode &value, DisplayModeDomain *domain)
{
  VectorImageWrapperBase *layer = this->GetLayerAsVector();
  if(!layer)
    return false;

  // Get the current display mode
  MultiChannelDisplayMode mode = GetMultiChannelDisplayPolicy()->GetDisplayMode();
  if(mode.UseRGB)
    {
    value = MODE_RGB;
    }
  else
    {
    switch(mode.SelectedScalarRep)
      {
      case SCALAR_REP_MAGNITUDE:
        value = MODE_MAGNITUDE; break;
      case SCALAR_REP_MAX:
        value = MODE_MAX; break;
      case SCALAR_REP_AVERAGE:
        value = MODE_AVERAGE; break;
      case SCALAR_REP_COMPONENT:
        value = MODE_COMPONENT; break;
      default:
        return false;
      }
    }

  // Fill out the domain
  if(domain)
    {
    (*domain)[MODE_COMPONENT] = "Single Component";
    (*domain)[MODE_MAGNITUDE] = "Magnitude of Components";
    (*domain)[MODE_MAX] = "Maximum Component";
    (*domain)[MODE_AVERAGE] = "Average of Components";

    // RGB is available only if there are three components
    if(layer->GetNumberOfComponents() == 3)
      (*domain)[MODE_RGB] = "RGB Display";
    }

  return true;
}

void LayerGeneralPropertiesModel
::SetDisplayModeValue(DisplayMode value)
{
  assert(this->GetLayerAsVector());

  // Get the current display mode
  MultiChannelDisplayMode mode = GetMultiChannelDisplayPolicy()->GetDisplayMode();

  // Update the current display mode
  switch(value)
    {
    case LayerGeneralPropertiesModel::MODE_COMPONENT:
      mode.SelectedScalarRep = SCALAR_REP_COMPONENT;
      mode.UseRGB = false;
      break;
    case LayerGeneralPropertiesModel::MODE_MAGNITUDE:
      mode.SelectedScalarRep = SCALAR_REP_MAGNITUDE;
      mode.SelectedComponent = 0;
      mode.UseRGB = false;
      break;
    case LayerGeneralPropertiesModel::MODE_MAX:
      mode.SelectedScalarRep = SCALAR_REP_MAX;
      mode.SelectedComponent = 0;
      mode.UseRGB = false;
      break;
    case LayerGeneralPropertiesModel::MODE_AVERAGE:
      mode.SelectedScalarRep = SCALAR_REP_AVERAGE;
      mode.SelectedComponent = 0;
      mode.UseRGB = false;
      break;
    case LayerGeneralPropertiesModel::MODE_RGB:
      mode.UseRGB = true;
      mode.SelectedScalarRep = SCALAR_REP_COMPONENT;
      mode.SelectedComponent = 0;
      break;
    }

  GetMultiChannelDisplayPolicy()->SetDisplayMode(mode);
}

bool LayerGeneralPropertiesModel
::GetSelectedComponentValueAndRange(int &value, NumericValueRange<int> *domain)
{
  VectorImageWrapperBase *layer = this->GetLayerAsVector();
  if(!layer)
    return false;

  // Get the current display mode
  MultiChannelDisplayMode mode = GetMultiChannelDisplayPolicy()->GetDisplayMode();

  // Mode must be single component
  if(mode.UseRGB ||
     mode.SelectedScalarRep != SCALAR_REP_COMPONENT)
    return false;

  // Use 1-based indexing
  value = mode.SelectedComponent + 1;

  if(domain)
    {
    domain->Set(1, layer->GetNumberOfComponents(), 1);
    }

  return true;
}

void LayerGeneralPropertiesModel
::SetSelectedComponentValue(int value)
{
  assert(this->GetLayerAsVector());

  // Get the current display mode
  MultiChannelDisplayMode mode = GetMultiChannelDisplayPolicy()->GetDisplayMode();
  mode.SelectedComponent = value - 1;
  GetMultiChannelDisplayPolicy()->SetDisplayMode(mode);
}

bool LayerGeneralPropertiesModel
::GetAnimateValue(bool &value)
{
  if(!this->GetLayerAsVector())
    return false;

  // Get the current display mode
  MultiChannelDisplayMode mode = GetMultiChannelDisplayPolicy()->GetDisplayMode();

  // Animation is only possible when showing components
  if(mode.UseRGB ||
     mode.SelectedScalarRep != SCALAR_REP_COMPONENT)
    return false;

  value = GetMultiChannelDisplayPolicy()->GetAnimate();
  return true;
}

void LayerGeneralPropertiesModel
::SetAnimateValue(bool value)
{
  assert(this->GetLayerAsVector());

  // Get the current display mode
  GetMultiChannelDisplayPolicy()->SetAnimate(value);
}

bool LayerGeneralPropertiesModel
::GetLayerOpacityValueAndRange(int &value, NumericValueRange<int> *domain)
{
  LayerTableRowModel *trm = GetSelectedLayerTableRowModel();
  return trm ? trm->GetLayerOpacityModel()->GetValueAndDomain(value, domain) : false;
}

void LayerGeneralPropertiesModel::SetLayerOpacityValue(int value)
{
  LayerTableRowModel *trm = GetSelectedLayerTableRowModel();
  trm->GetLayerOpacityModel()->SetValue(value);
}

bool LayerGeneralPropertiesModel::GetLayerVisibilityValue(bool &value)
{
  LayerTableRowModel *trm = GetSelectedLayerTableRowModel();
  return trm ? trm->GetVisibilityToggleModel()->GetValueAndDomain(value, NULL) : false;
}

void LayerGeneralPropertiesModel::SetLayerVisibilityValue(bool value)
{
  LayerTableRowModel *trm = GetSelectedLayerTableRowModel();
  trm->GetVisibilityToggleModel()->SetValue(value);
}

bool LayerGeneralPropertiesModel::GetFilenameValue(std::string &value)
{
  ImageWrapperBase *layer = this->GetLayer();
  if(layer)
    {
    value = layer->GetFileName();
    return true;
    }
  return false;
}

bool LayerGeneralPropertiesModel::GetNicknameValue(std::string &value)
{
  ImageWrapperBase *layer = this->GetLayer();
  if(layer)
    {
    value = layer->GetCustomNickname();
    return true;
    }
  return false;
}

void LayerGeneralPropertiesModel::SetNicknameValue(std::string value)
{
  ImageWrapperBase *layer = this->GetLayer();
  layer->SetCustomNickname(value);
}

VectorImageWrapperBase *
LayerGeneralPropertiesModel::GetLayerAsVector()
{
  return dynamic_cast<VectorImageWrapperBase *>(this->GetLayer());
}


bool LayerGeneralPropertiesModel::GetIsStickyValue(bool &value)
{
  // Delegate to the row model for this
  LayerTableRowModel *trm = GetSelectedLayerTableRowModel();
  return trm ? trm->GetStickyModel()->GetValueAndDomain(value, NULL) : false;
}

void LayerGeneralPropertiesModel::SetIsStickyValue(bool value)
{
  // Delegate to the row model for this
  LayerTableRowModel *trm = GetSelectedLayerTableRowModel();
  trm->GetStickyModel()->SetValue(value);
}


AbstractMultiChannelDisplayMappingPolicy *
LayerGeneralPropertiesModel::GetMultiChannelDisplayPolicy()
{
  AbstractMultiChannelDisplayMappingPolicy *dp = static_cast<
      AbstractMultiChannelDisplayMappingPolicy *>(
        this->GetLayer()->GetDisplayMapping());
  return dp;
}

LayerTableRowModel *LayerGeneralPropertiesModel::GetSelectedLayerTableRowModel()
{
  if(m_Layer)
    return dynamic_cast<LayerTableRowModel *>(m_Layer->GetUserData("LayerTableRowModel"));
  else return NULL;
}
