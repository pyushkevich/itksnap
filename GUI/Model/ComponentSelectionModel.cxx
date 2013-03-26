#include "ComponentSelectionModel.h"
#include "DisplayMappingPolicy.h"
#include "LayerAssociation.txx"

template class LayerAssociation<ComponentSelectionLayerProperties,
                                VectorImageWrapperBase,
                                ComponentSelectionModel::PropertiesFactory>;


ComponentSelectionModel::ComponentSelectionModel()
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
}

ComponentSelectionModel::~ComponentSelectionModel()
{

}


void
ComponentSelectionModel::SetParentModel(GlobalUIModel *parent)
{
  Superclass::SetParentModel(parent);
}

void
ComponentSelectionModel::RegisterWithLayer(VectorImageWrapperBase *layer)
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
ComponentSelectionModel::UnRegisterFromLayer(VectorImageWrapperBase *layer, bool being_deleted)
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
ComponentSelectionModel::OnUpdate()
{
  Superclass::OnUpdate();

  // Do we need this method?
}

bool ComponentSelectionModel
::GetDisplayModeValueAndRange(DisplayMode &value, DisplayModeDomain *domain)
{
  VectorImageWrapperBase *layer = this->GetLayer();
  if(!layer)
    return false;

  // Get the current display mode
  MultiChannelDisplayMode mode = GetDisplayPolicy()->GetDisplayMode();
  if(mode.UseRGB)
    {
    value = MODE_RGB;
    }
  else
    {
    switch(mode.SelectedScalarRep)
      {
      case VectorImageWrapperBase::SCALAR_REP_MAGNITUDE:
        value = MODE_MAGNITUDE; break;
      case VectorImageWrapperBase::SCALAR_REP_MAX:
        value = MODE_MAX; break;
      case VectorImageWrapperBase::SCALAR_REP_AVERAGE:
        value = MODE_AVERAGE; break;
      case VectorImageWrapperBase::SCALAR_REP_COMPONENT:
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

void ComponentSelectionModel
::SetDisplayModeValue(DisplayMode value)
{
  // Get the current display mode
  MultiChannelDisplayMode mode = GetDisplayPolicy()->GetDisplayMode();

  // Update the current display mode
  switch(value)
    {
    case ComponentSelectionModel::MODE_COMPONENT:
      mode.SelectedScalarRep = VectorImageWrapperBase::SCALAR_REP_COMPONENT;
      mode.UseRGB = false;
      break;
    case ComponentSelectionModel::MODE_MAGNITUDE:
      mode.SelectedScalarRep = VectorImageWrapperBase::SCALAR_REP_MAGNITUDE;
      mode.SelectedComponent = 0;
      mode.UseRGB = false;
      break;
    case ComponentSelectionModel::MODE_MAX:
      mode.SelectedScalarRep = VectorImageWrapperBase::SCALAR_REP_MAX;
      mode.SelectedComponent = 0;
      mode.UseRGB = false;
      break;
    case ComponentSelectionModel::MODE_AVERAGE:
      mode.SelectedScalarRep = VectorImageWrapperBase::SCALAR_REP_AVERAGE;
      mode.SelectedComponent = 0;
      mode.UseRGB = false;
      break;
    case ComponentSelectionModel::MODE_RGB:
      mode.UseRGB = true;
      mode.SelectedScalarRep = VectorImageWrapperBase::SCALAR_REP_COMPONENT;
      mode.SelectedComponent = 0;
      break;
    }

  GetDisplayPolicy()->SetDisplayMode(mode);
}

bool ComponentSelectionModel
::GetSelectedComponentValueAndRange(int &value, NumericValueRange<int> *domain)
{
  VectorImageWrapperBase *layer = this->GetLayer();
  if(!layer)
    return false;

  // Get the current display mode
  MultiChannelDisplayMode mode = GetDisplayPolicy()->GetDisplayMode();

  // Mode must be single component
  if(mode.UseRGB ||
     mode.SelectedScalarRep != VectorImageWrapperBase::SCALAR_REP_COMPONENT)
    return false;

  // Use 1-based indexing
  value = mode.SelectedComponent + 1;

  if(domain)
    {
    domain->Set(1, layer->GetNumberOfComponents(), 1);
    }

  return true;
}

void ComponentSelectionModel
::SetSelectedComponentValue(int value)
{
  // Get the current display mode
  MultiChannelDisplayMode mode = GetDisplayPolicy()->GetDisplayMode();
  mode.SelectedComponent = value - 1;
  GetDisplayPolicy()->SetDisplayMode(mode);
}

bool ComponentSelectionModel
::GetAnimateValue(bool &value)
{
  if(!this->GetLayer())
    return false;

  // Get the current display mode
  MultiChannelDisplayMode mode = GetDisplayPolicy()->GetDisplayMode();

  // Animation is only possible when showing components
  if(mode.UseRGB ||
     mode.SelectedScalarRep != VectorImageWrapperBase::SCALAR_REP_COMPONENT)
    return false;

  value = GetDisplayPolicy()->GetAnimate();
  return true;
}

void ComponentSelectionModel
::SetAnimateValue(bool value)
{
  // Get the current display mode
  GetDisplayPolicy()->SetAnimate(value);
}

AbstractMultiChannelDisplayMappingPolicy *
ComponentSelectionModel::GetDisplayPolicy()
{
  AbstractMultiChannelDisplayMappingPolicy *dp = static_cast<
      AbstractMultiChannelDisplayMappingPolicy *>(
        this->GetLayer()->GetDisplayMapping());
  return dp;
}
