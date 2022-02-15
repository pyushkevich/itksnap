#include "LayerGeneralPropertiesModel.h"
#include "DisplayMappingPolicy.h"
#include "LayerAssociation.txx"
#include "NumericPropertyToggleAdaptor.h"
#include "LayerTableRowModel.h"
#include "TimePointProperties.h"

template class LayerAssociation<GeneralLayerProperties,
                                WrapperBase,
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

  m_TagsModel = wrapGetterSetterPairAsProperty(
        this,
        &Self::GetTagsValue,
        &Self::SetTagsValue);

  m_CrntTimePointNicknameModel = wrapGetterSetterPairAsProperty(
        this,
        &Self::GetCrntTimePointNicknameValue,
        &Self::SetCrntTimePointNicknameValue);

  m_CrntTimePointTagListModel = wrapGetterSetterPairAsProperty(
        this,
        &Self::GetCrntTimePointTagListValue,
        &Self::SetCrntTimePointTagListValue);

  m_MeshDataArrayNameModel = wrapGetterSetterPairAsProperty(
        this,
        &Self::GetMeshDataArrayNameValueAndRange,
        &Self::SetMeshDataArrayNameValue);
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

  this->Rebroadcast(
        parent->GetDriver(),
        CursorTimePointUpdateEvent(), ModelUpdateEvent());

  m_TimePointProperties = parent->GetDriver()->GetIRISImageData()->GetTimePointProperties();
}

void
LayerGeneralPropertiesModel::RegisterWithLayer(WrapperBase *layer)
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
LayerGeneralPropertiesModel::UnRegisterFromLayer(WrapperBase *layer, bool being_deleted)
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
}

bool LayerGeneralPropertiesModel::CheckState(LayerGeneralPropertiesModel::UIState state)
{
  if(!m_Layer)
    return false;

  // Defer to the row model when we can
  AbstractLayerTableRowModel *row_model = this->GetSelectedLayerTableRowModel();

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
      {
        auto image_layer = dynamic_cast<ImageWrapperBase*>(m_Layer);
        if (image_layer)
          return image_layer->GetNumberOfComponents() > 1;
        else
          return false;
      }

    case UIF_IS_STICKINESS_EDITABLE:
      return row_model->CheckState(AbstractLayerTableRowModel::UIF_PINNABLE)
          || row_model->CheckState(AbstractLayerTableRowModel::UIF_UNPINNABLE);

    case UIF_IS_OPACITY_EDITABLE:
      return row_model->CheckState(AbstractLayerTableRowModel::UIF_OPACITY_EDITABLE);

    case UIF_MOVABLE_UP:
      return row_model->CheckState(AbstractLayerTableRowModel::UIF_MOVABLE_UP);

    case UIF_MOVABLE_DOWN:
      return row_model->CheckState(AbstractLayerTableRowModel::UIF_MOVABLE_DOWN);

    case UIF_IS_4D_IMAGE:
      return this->m_ParentModel->GetDriver()->GetNumberOfTimePoints() > 1;

    case UIF_IS_MESH:
      {
        auto mesh_layer = dynamic_cast<MeshWrapperBase*>(m_Layer);
        return mesh_layer != nullptr;
      }
    }

  return false;
}

void LayerGeneralPropertiesModel::MoveLayerUp()
{
  auto img_model = dynamic_cast<ImageLayerTableRowModel*>(
        this->GetSelectedLayerTableRowModel());

  if (img_model)
    img_model->MoveLayerUp();
}

void LayerGeneralPropertiesModel::MoveLayerDown()
{
  auto img_model = dynamic_cast<ImageLayerTableRowModel*>(
        this->GetSelectedLayerTableRowModel());

  if (img_model)
    img_model->MoveLayerDown();
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
  else if(mode.RenderAsGrid)
    {
    value = MODE_GRID;
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
      {
      (*domain)[MODE_RGB] = "RGB Display";
      (*domain)[MODE_GRID] = "Deformation Grid Display";
      }
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
      mode.RenderAsGrid = false;
      break;
    case LayerGeneralPropertiesModel::MODE_MAGNITUDE:
      mode.SelectedScalarRep = SCALAR_REP_MAGNITUDE;
      mode.SelectedComponent = 0;
      mode.UseRGB = false;
      mode.RenderAsGrid = false;
      break;
    case LayerGeneralPropertiesModel::MODE_MAX:
      mode.SelectedScalarRep = SCALAR_REP_MAX;
      mode.SelectedComponent = 0;
      mode.UseRGB = false;
      mode.RenderAsGrid = false;
      break;
    case LayerGeneralPropertiesModel::MODE_AVERAGE:
      mode.SelectedScalarRep = SCALAR_REP_AVERAGE;
      mode.SelectedComponent = 0;
      mode.UseRGB = false;
      mode.RenderAsGrid = false;
      break;
    case LayerGeneralPropertiesModel::MODE_RGB:
      mode.UseRGB = true;
      mode.RenderAsGrid = false;
      mode.SelectedScalarRep = SCALAR_REP_COMPONENT;
      mode.SelectedComponent = 0;
      break;
    case LayerGeneralPropertiesModel::MODE_GRID:
      mode.UseRGB = false;
      mode.RenderAsGrid = true;
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
  if(!mode.IsSingleComponent())
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
  if(!mode.IsSingleComponent())
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
  AbstractLayerTableRowModel *trm = GetSelectedLayerTableRowModel();
  return trm ? trm->GetLayerOpacityModel()->GetValueAndDomain(value, domain) : false;
}

void LayerGeneralPropertiesModel::SetLayerOpacityValue(int value)
{
  AbstractLayerTableRowModel *trm = GetSelectedLayerTableRowModel();
  trm->GetLayerOpacityModel()->SetValue(value);
}

bool LayerGeneralPropertiesModel::GetLayerVisibilityValue(bool &value)
{
  AbstractLayerTableRowModel *trm = GetSelectedLayerTableRowModel();
  return trm ? trm->GetVisibilityToggleModel()->GetValueAndDomain(value, NULL) : false;
}

void LayerGeneralPropertiesModel::SetLayerVisibilityValue(bool value)
{
  AbstractLayerTableRowModel *trm = GetSelectedLayerTableRowModel();
  trm->GetVisibilityToggleModel()->SetValue(value);
}

bool LayerGeneralPropertiesModel::GetFilenameValue(std::string &value)
{
  auto layer = this->GetLayer();
  if(layer)
    {
    value = layer->GetFileName();
    return true;
    }
  return false;
}

bool LayerGeneralPropertiesModel::GetNicknameValue(std::string &value)
{
  auto layer = this->GetLayer();
  if(layer)
    {
    value = layer->GetCustomNickname();
    return true;
    }
  return false;
}

void LayerGeneralPropertiesModel::SetNicknameValue(std::string value)
{
  auto layer = this->GetLayer();
  layer->SetCustomNickname(value);
}

bool LayerGeneralPropertiesModel::GetTagsValue(TagList &value)
{
  auto layer = this->GetLayer();
  if(layer)
    {
    value = layer->GetTags();
    return true;
    }
  return false;
}

void LayerGeneralPropertiesModel::SetTagsValue(TagList value)
{
  auto layer = this->GetLayer();
  layer->SetTags(value);
}

VectorImageWrapperBase *
LayerGeneralPropertiesModel::GetLayerAsVector()
{
  return dynamic_cast<VectorImageWrapperBase *>(this->GetLayer());
}


bool LayerGeneralPropertiesModel::GetIsStickyValue(bool &value)
{
  // Delegate to the row model for this
  AbstractLayerTableRowModel *trm = GetSelectedLayerTableRowModel();
  return trm ? trm->GetStickyModel()->GetValueAndDomain(value, NULL) : false;
}

void LayerGeneralPropertiesModel::SetIsStickyValue(bool value)
{
  // Delegate to the row model for this
  AbstractLayerTableRowModel *trm = GetSelectedLayerTableRowModel();

  // Calling this method will set the globally selected layer to the main layer
  // because the globally selected layer cannot be sticky. However, we can still
  // have a sticky layer selected in the layer inspector. So we override.
  trm->GetStickyModel()->SetValue(value);  
}

bool LayerGeneralPropertiesModel::
GetCrntTimePointNicknameValue(std::string &value)
{
  if (!m_ParentModel->GetDriver()->IsMainImageLoaded())
    return false;

  unsigned int crntTP = m_ParentModel->GetDriver()->GetCursorTimePoint() + 1;
  value = m_TimePointProperties->GetProperty(crntTP)->Nickname;
  return true;
}

void LayerGeneralPropertiesModel::
SetCrntTimePointNicknameValue(std::string value)
{
  if (!m_ParentModel->GetDriver()->IsMainImageLoaded())
    return;

  unsigned int crntTP = m_ParentModel->GetDriver()->GetCursorTimePoint() + 1;
  m_TimePointProperties->GetProperty(crntTP)->Nickname = value;
}

bool LayerGeneralPropertiesModel::
GetCrntTimePointTagListValue(TagList &value)
{
  if(!m_ParentModel->GetDriver()->IsMainImageLoaded())
    return false;

  unsigned int crntTP = m_ParentModel->GetDriver()->GetCursorTimePoint() + 1;
  TimePointProperty *tpp = m_TimePointProperties->GetProperty(crntTP);
  value = tpp->Tags;
  return true;
}

void LayerGeneralPropertiesModel::
SetCrntTimePointTagListValue(TagList value)
{
  if (!m_ParentModel->GetDriver()->IsMainImageLoaded())
    return;

  unsigned int crntTP = m_ParentModel->GetDriver()->GetCursorTimePoint() + 1;
  TimePointProperty *tpp = m_TimePointProperties->GetProperty(crntTP);
  tpp->Tags = value;
}


AbstractMultiChannelDisplayMappingPolicy *
LayerGeneralPropertiesModel::GetMultiChannelDisplayPolicy()
{
  AbstractMultiChannelDisplayMappingPolicy *dp = static_cast<
      AbstractMultiChannelDisplayMappingPolicy *>(
        this->GetLayer()->GetDisplayMapping());
  return dp;
}

AbstractLayerTableRowModel *LayerGeneralPropertiesModel::GetSelectedLayerTableRowModel()
{
  if(m_Layer)
    return dynamic_cast<AbstractLayerTableRowModel *>(m_Layer->GetUserData("LayerTableRowModel"));
  else return NULL;
}

bool
LayerGeneralPropertiesModel::
GetMeshDataArrayNameValueAndRange(int &value, MeshDataArrayNameDomain *domain)
{
  // The current layer has to be a mesh layer
  MeshWrapperBase *mesh_layer = dynamic_cast<MeshWrapperBase*>(m_Layer);
  if (!mesh_layer)
    return false;

  auto props = mesh_layer->GetLayerDataProperty();

  if (domain)
    {
    for (auto it = props.cbegin(); it != props.cend(); ++it)
      {
      (*domain)[it->first] = it->second->GetName();
      }

    value = mesh_layer->GetActiveMeshLayerDataPropertyId();
    }

  return true;
}

void
LayerGeneralPropertiesModel::
SetMeshDataArrayNameValue(int value)
{
  // The current layer has to be a mesh layer
  MeshWrapperBase *mesh_layer = dynamic_cast<MeshWrapperBase*>(m_Layer);
  if (!mesh_layer)
    return;

  mesh_layer->SetActiveMeshLayerDataPropertyId(value);
}
