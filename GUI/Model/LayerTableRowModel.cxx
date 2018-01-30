#include "LayerTableRowModel.h"
#include "GlobalUIModel.h"
#include "IRISApplication.h"
#include "GenericImageData.h"
#include "ImageWrapperBase.h"
#include "NumericPropertyToggleAdaptor.h"
#include "DisplayMappingPolicy.h"
#include "DisplayLayoutModel.h"
#include "ImageIOWizardModel.h"
#include "ColorMapModel.h"
#include "IntensityCurveModel.h"
#include "LayerGeneralPropertiesModel.h"
#include "SNAPImageData.h"


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

  m_ColorMapPresetModel = wrapGetterSetterPairAsProperty(
        this,
        &Self::GetColorMapPresetValue,
        &Self::SetColorMapPresetValue);

  m_StickyModel = wrapGetterSetterPairAsProperty(
        this,
        &Self::GetStickyValue,
        &Self::SetSticklyValue);

  m_DisplayModeModel = wrapGetterSetterPairAsProperty(
        this,
        &Self::GetDisplayModeValue,
        &Self::SetDisplayModeValue);

  m_VisibilityToggleModel = NewNumericPropertyToggleAdaptor(
        m_LayerOpacityModel.GetPointer(), 0, 50);

  m_LayerRole = NO_ROLE;
  m_LayerPositionInRole = -1;
  m_ImageData = NULL;
}

bool LayerTableRowModel::CheckState(UIState state)
{
  // Are we in tiling mode?
  bool tiling = (
        m_ParentModel->GetGlobalState()->GetSliceViewLayerLayout() ==
        LAYOUT_TILED);

  bool snapmode = m_ParentModel->GetDriver()->IsSnakeModeActive();

  // Check the states
  switch(state)
    {
    // Opacity can be edited for all layers except the main image layer
    case LayerTableRowModel::UIF_OPACITY_EDITABLE:
      return (m_Layer->IsSticky());

    // Pinnable means it's not sticky and may be overlayed (i.e., not main)
    case LayerTableRowModel::UIF_PINNABLE:
      return (m_LayerRole != MAIN_ROLE && !m_Layer->IsSticky());

    // Unpinnable means it's not sticky and may be overlayed (i.e., not main)
    case LayerTableRowModel::UIF_UNPINNABLE:
      return (m_LayerRole != MAIN_ROLE && m_Layer->IsSticky());

    case LayerTableRowModel::UIF_MOVABLE_UP:
      return (m_LayerRole == OVERLAY_ROLE
              && m_LayerPositionInRole > 0);

    case LayerTableRowModel::UIF_MOVABLE_DOWN:
      return (m_LayerRole == OVERLAY_ROLE
              && m_LayerPositionInRole < m_LayerNumberOfLayersInRole - 1);

    case LayerTableRowModel::UIF_CLOSABLE:
      return ((m_LayerRole == OVERLAY_ROLE
               || m_LayerRole == MAIN_ROLE) && !snapmode);

    case LayerTableRowModel::UIF_CONTRAST_ADJUSTABLE:
      return (m_Layer && m_Layer->GetDisplayMapping()->GetIntensityCurve());

    case LayerTableRowModel::UIF_COLORMAP_ADJUSTABLE:
      return (m_Layer && m_Layer->GetDisplayMapping()->GetColorMap());

    case LayerTableRowModel::UIF_MULTICOMPONENT:
      return (m_Layer && m_Layer->GetNumberOfComponents() > 1);
    }

  return false;
}

void LayerTableRowModel::UpdateRoleInfo()
{
  LayerIterator it(m_ImageData);
  it.Find(m_Layer);
  m_LayerRole = it.GetRole();
  m_LayerPositionInRole = it.GetPositionInRole();
  m_LayerNumberOfLayersInRole = it.GetNumberOfLayersInRole();
}

void LayerTableRowModel::UpdateDisplayModeList()
{
  m_AvailableDisplayModes.clear();
  if(m_Layer && m_Layer->GetNumberOfComponents() > 1)
    {
    for(int i = 0; i < m_Layer->GetNumberOfComponents(); i++)
      m_AvailableDisplayModes.push_back(
            MultiChannelDisplayMode(false, SCALAR_REP_COMPONENT, i));
    m_AvailableDisplayModes.push_back(
          MultiChannelDisplayMode(false, SCALAR_REP_MAGNITUDE, 0));
    m_AvailableDisplayModes.push_back(
          MultiChannelDisplayMode(false, SCALAR_REP_MAX, 0));
    m_AvailableDisplayModes.push_back(
          MultiChannelDisplayMode(false, SCALAR_REP_AVERAGE, 0));

    if(m_Layer->GetNumberOfComponents() == 3)
      m_AvailableDisplayModes.push_back(
            MultiChannelDisplayMode::DefaultForRGB());
    }
}

MultiChannelDisplayMode LayerTableRowModel::GetDisplayMode()
{
  AbstractMultiChannelDisplayMappingPolicy *dp = dynamic_cast<
      AbstractMultiChannelDisplayMappingPolicy *>(m_Layer->GetDisplayMapping());
  return dp->GetDisplayMode();
}

void LayerTableRowModel::Initialize(GlobalUIModel *parentModel, ImageWrapperBase *layer)
{
  m_ParentModel = parentModel;
  m_Layer = layer;
  m_ImageData = parentModel->GetDriver()->GetCurrentImageData();

  // For some of the functions, it is useful to know the role and the index
  // in the role of this layer. We shouldn't have to worry about this info
  // changing, since the rows get rebuilt when LayerChangeEvent() is fired.
  UpdateRoleInfo();

  // Update the list of display modes (again, should not change during the
  // lifetime of this object
  UpdateDisplayModeList();

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

  Rebroadcast(layer, WrapperMetadataChangeEvent(), StateMachineChangeEvent());
}

void LayerTableRowModel::MoveLayerUp()
{
  m_ParentModel->GetDriver()->ChangeOverlayPosition(m_Layer, -1);
}

void LayerTableRowModel::MoveLayerDown()
{
  m_ParentModel->GetDriver()->ChangeOverlayPosition(m_Layer, +1);
}


SmartPtr<ImageIOWizardModel> LayerTableRowModel::CreateIOWizardModelForSave()
{
  return m_ParentModel->CreateIOWizardModelForSave(m_Layer, m_LayerRole);
}

bool LayerTableRowModel::IsMainLayer()
{
  return m_LayerRole == MAIN_ROLE;
}

void LayerTableRowModel::SetSelected(bool selected)
{
  // If the layer is selected and is not sticky, we set is as the currently visible
  // layer in the render views
  if(selected && !m_Layer->IsSticky())
    {
    m_ParentModel->GetGlobalState()->SetSelectedLayerId(m_Layer->GetUniqueId());
    }
}

void LayerTableRowModel::CloseLayer()
{
  // If this is an overlay, we unload it like this
  if(m_LayerRole == OVERLAY_ROLE)
    {
    m_ParentModel->GetDriver()->UnloadOverlay(m_Layer);
    m_Layer = NULL;
    }

  // The main image can also be unloaded
  else if(m_LayerRole == MAIN_ROLE)
    {
    m_ParentModel->GetDriver()->UnloadMainImage();
    m_Layer = NULL;
    }
}

void LayerTableRowModel::AutoAdjustContrast()
{
  if(m_Layer && m_Layer->GetDisplayMapping()->GetIntensityCurve())
    {
    // TODO: this is a bit of a hack, since we go through a different model
    // and have to swap out that model's current layer, which adds some overhead
    IntensityCurveModel *icm = m_ParentModel->GetIntensityCurveModel();
    ImageWrapperBase *currentLayer = icm->GetLayer();
    icm->SetLayer(m_Layer);
    icm->OnAutoFitWindow();
    icm->SetLayer(currentLayer);
    }
}

#include "MomentTextures.h"

void LayerTableRowModel::GenerateTextureFeatures()
{
  ScalarImageWrapperBase *scalar = dynamic_cast<ScalarImageWrapperBase *>(m_Layer);
  if(scalar)
    {
    // Get the image out
    SmartPtr<ScalarImageWrapperBase::CommonFormatImageType> common_rep =
        scalar->GetCommonFormatImage();

    /*
    SmartPtr<ScalarImageWrapperBase::CommonFormatImageType> texture_image =
        ScalarImageWrapperBase::CommonFormatImageType::New();

    texture_image->CopyInformation(common_rep);
    texture_image->SetRegions(common_rep->GetBufferedRegion());
    texture_image->Allocate();*/

    // Create a radius - hard-coded for now
    itk::Size<3> radius; radius.Fill(2);

    // Create a filter to generate textures
    typedef AnatomicImageWrapperTraits<GreyType>::ImageType TextureImageType;
    typedef bilwaj::MomentTextureFilter<
        ScalarImageWrapperBase::CommonFormatImageType,
        TextureImageType> MomentFilterType;

    MomentFilterType::Pointer filter = MomentFilterType::New();
    filter->SetInput(common_rep);
    filter->SetRadius(radius);
    filter->SetHighestDegree(3);
    filter->Update();

    // Create a new image wrapper
    SmartPtr<AnatomicImageWrapper> newWrapper = AnatomicImageWrapper::New();
    newWrapper->InitializeToWrapper(m_Layer, filter->GetOutput(), NULL, NULL);
    newWrapper->SetDefaultNickname("Textures");
    this->GetParentModel()->GetDriver()->AddDerivedOverlayImage(newWrapper);
    }
}

std::string
LayerTableRowModel::GetDisplayModeString(const MultiChannelDisplayMode &mode)
{
  if(mode.UseRGB)
    {
    return "RGB";
    }

  std::ostringstream oss;
  switch(mode.SelectedScalarRep)
    {
    case SCALAR_REP_COMPONENT:
      oss << (1 + mode.SelectedComponent) << "/" << m_Layer->GetNumberOfComponents();
      return oss.str();

    case SCALAR_REP_MAGNITUDE:
      return "Mag";

    case SCALAR_REP_MAX:
      return "Max";

    case SCALAR_REP_AVERAGE:
      return "Avg";

    case NUMBER_OF_SCALAR_REPS:
      break;
    };

  return "";
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
  // Get the name of the compomnent
  if(m_Layer && m_Layer->GetNumberOfComponents() > 1)
    {
    value = this->GetDisplayModeString(this->GetDisplayMode());
    return true;
    }

  return false;
}

bool LayerTableRowModel::GetColorMapPresetValue(std::string &value)
{
  if(m_Layer && m_Layer->GetDisplayMapping()->GetColorMap())
    {
    value = ColorMap::GetPresetName(
          m_Layer->GetDisplayMapping()->GetColorMap()->GetSystemPreset());
    return true;
    }
  return false;
}

void LayerTableRowModel::SetColorMapPresetValue(std::string value)
{
  // TODO: this is a cheat! The current way of handling color map presets in
  // snap is hacky, and I really don't like it. We need a common class that
  // can handle system and user presets for a variety of objects, one that
  // can interface nicely with the GUI models. Here we are going through
  // the functionality provided by the ColorMapModel class.
  ColorMapModel *cmm = m_ParentModel->GetColorMapModel();
  ImageWrapperBase *currentLayer = cmm->GetLayer();
  cmm->SetLayer(m_Layer);
  cmm->SelectPreset(value);
  cmm->SetLayer(currentLayer);
}

bool LayerTableRowModel::GetDisplayModeValue(MultiChannelDisplayMode &value)
{
  if(m_Layer && m_Layer->GetNumberOfComponents() > 1)
    {
    AbstractMultiChannelDisplayMappingPolicy *dp = dynamic_cast<
        AbstractMultiChannelDisplayMappingPolicy *>(m_Layer->GetDisplayMapping());
    value = dp->GetDisplayMode();
    return true;
    }
  return false;
}

void LayerTableRowModel::SetDisplayModeValue(MultiChannelDisplayMode value)
{
  AbstractMultiChannelDisplayMappingPolicy *dp = dynamic_cast<
      AbstractMultiChannelDisplayMappingPolicy *>(m_Layer->GetDisplayMapping());
  dp->SetDisplayMode(value);
}

void LayerTableRowModel::OnUpdate()
{
  // Has our layer been deleted?
  if(this->m_EventBucket->HasEvent(itk::DeleteEvent(), m_Layer))
    {
    m_Layer = NULL;
    m_LayerRole = NO_ROLE;
    m_LayerPositionInRole = -1;
    m_LayerNumberOfLayersInRole = -1;
    }
  else if(this->m_EventBucket->HasEvent(LayerChangeEvent()))
    {
    this->UpdateRoleInfo();
    }
}



bool LayerTableRowModel::GetLayerOpacityValueAndRange(int &value, NumericValueRange<int> *domain)
{
  // For opacity to be defined, the layer must be sticky
  if(!m_Layer || !m_Layer->IsSticky()) return false;

  // Meaning of 'visible' is different for sticky and non-sticky layers
  value = (int)(100.0 * m_Layer->GetAlpha());

  if(domain)
    domain->Set(0, 100, 5);

  return true;
}
 void LayerTableRowModel::SetLayerOpacityValue(int value)
{
  assert(m_Layer && m_Layer->IsSticky());
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
  // Make sure the selected ID is legitimate
  if(m_ParentModel->GetGlobalState()->GetSelectedLayerId() == m_Layer->GetUniqueId())
    {
    m_ParentModel->GetGlobalState()->SetSelectedLayerId(
          m_ParentModel->GetDriver()->GetCurrentImageData()->GetMain()->GetUniqueId());
    }
  m_Layer->SetSticky(value);
}

