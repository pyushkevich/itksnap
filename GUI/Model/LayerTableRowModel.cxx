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
#include "IncreaseDimensionImageFilter.h"
#include "SNAPImageData.h"
#include "ImageMeshLayers.h"
#include "MomentTextures.h"
#include "SegmentationMeshWrapper.h"


AbstractLayerTableRowModel::AbstractLayerTableRowModel()
{
  m_LayerOpacityModel = wrapGetterSetterPairAsProperty(
        this,
        &Self::GetLayerOpacityValueAndRange,
        &Self::SetLayerOpacityValue);

  m_NicknameModel = wrapGetterSetterPairAsProperty(
        this,
        &Self::GetNicknameValue,
        &Self::SetNicknameValue);

  m_StickyModel = wrapGetterSetterPairAsProperty(
        this,
        &Self::GetStickyValue,
        &Self::SetStickyValue);

  m_ColorMapPresetModel = wrapGetterSetterPairAsProperty(
        this,
        &Self::GetColorMapPresetValue,
        &Self::SetColorMapPresetValue);


  m_VisibilityToggleModel = NewNumericPropertyToggleAdaptor(
        m_LayerOpacityModel.GetPointer(), 0, 50);


  m_LayerRole = NO_ROLE;
  m_LayerPositionInRole = -1;
  m_ImageData = NULL;
}

bool AbstractLayerTableRowModel::CheckState(UIState state)
{
  // Are we in tiling mode?
  /* commenting out unused code to avoid warnings
  bool tiling = (
        m_ParentModel->GetGlobalState()->GetSliceViewLayerLayout() ==
        LAYOUT_TILED); */

  bool snapmode = m_ParentModel->GetDriver()->IsSnakeModeActive();

  // Check the states
  switch(state)
    {
    case AbstractLayerTableRowModel::UIF_MOVABLE_UP:
      return (m_LayerRole == OVERLAY_ROLE && m_LayerPositionInRole > 0);

    case AbstractLayerTableRowModel::UIF_MOVABLE_DOWN:
      return (m_LayerRole == OVERLAY_ROLE && m_LayerPositionInRole < m_LayerNumberOfLayersInRole - 1);

    case AbstractLayerTableRowModel::UIF_CLOSABLE:
      return !snapmode;

    default:
      break;
    }

  return false;
}


void AbstractLayerTableRowModel::Initialize(GlobalUIModel *parentModel, WrapperBase *layer)
{
  m_ParentModel = parentModel;
  m_Layer = layer;
  m_ImageData = parentModel->GetDriver()->GetCurrentImageData();

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

  Rebroadcast(layer, WrapperMetadataChangeEvent(), StateMachineChangeEvent());
}

bool AbstractLayerTableRowModel::IsMainLayer()
{
  return m_LayerRole == MAIN_ROLE;
}

bool AbstractLayerTableRowModel::GetNicknameValue(std::string &value)
{
  if(!m_Layer) return false;

  value = m_Layer->GetNickname();
  return true;
}

void AbstractLayerTableRowModel::SetNicknameValue(std::string value)
{
  m_Layer->SetCustomNickname(value);
}


void AbstractLayerTableRowModel::OnUpdate()
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



bool AbstractLayerTableRowModel::GetLayerOpacityValueAndRange(int &value, NumericValueRange<int> *domain)
{
  // For opacity to be defined, the layer must be sticky
  if(!m_Layer) return false;

  // Meaning of 'visible' is different for sticky and non-sticky layers
  value = (int)(100.0 * m_Layer->GetAlpha());

  if(domain)
    domain->Set(0, 100, 5);

  return true;
}
 void AbstractLayerTableRowModel::SetLayerOpacityValue(int value)
{
  assert(m_Layer);
  m_Layer->SetAlpha(value / 100.0);
}



/*
 * ===============================================
 *   ImageLayerTableRowModel Implementation
 * ===============================================
*/

ImageLayerTableRowModel::ImageLayerTableRowModel()
{
  m_ComponentNameModel = wrapGetterSetterPairAsProperty(
        this,
        &Self::GetComponentNameValue);

  m_DisplayModeModel = wrapGetterSetterPairAsProperty(
        this,
        &Self::GetDisplayModeValue,
        &Self::SetDisplayModeValue);

  m_ColorMapPresetModel = wrapGetterSetterPairAsProperty(
        this,
        &Self::GetColorMapPresetValue,
        &Self::SetColorMapPresetValue);

  m_VolumeRenderingEnabledModel = wrapGetterSetterPairAsProperty(
        this,
        &Self::GetVolumeRenderingEnabledValue,
        &Self::SetVolumeRenderingEnabledValue);
}

void ImageLayerTableRowModel::UpdateDisplayModeList()
{
  m_AvailableDisplayModes.clear();
  if(m_Layer && m_ImageLayer->GetNumberOfComponents() > 1)
    {
    for(std::size_t i = 0; i < m_ImageLayer->GetNumberOfComponents(); i++)
      m_AvailableDisplayModes.push_back(MultiChannelDisplayMode(false, false, SCALAR_REP_COMPONENT, i));

    m_AvailableDisplayModes.push_back(
          MultiChannelDisplayMode(false, false, SCALAR_REP_MAGNITUDE, 0));
    m_AvailableDisplayModes.push_back(
          MultiChannelDisplayMode(false, false, SCALAR_REP_MAX, 0));
    m_AvailableDisplayModes.push_back(
          MultiChannelDisplayMode(false, false, SCALAR_REP_AVERAGE, 0));

    if(m_ImageLayer->GetNumberOfComponents() == 3)
      {
      m_AvailableDisplayModes.push_back(
            MultiChannelDisplayMode(true, false, SCALAR_REP_COMPONENT));
      m_AvailableDisplayModes.push_back(
            MultiChannelDisplayMode(false, true, SCALAR_REP_COMPONENT));
      }
    else if(m_ImageLayer->GetNumberOfComponents() == 2 &&
            m_ImageLayer->GetSize()[2] == 1)
      {
      m_AvailableDisplayModes.push_back(
            MultiChannelDisplayMode(false, true, SCALAR_REP_MAGNITUDE, 0));
      }
    }
}

bool
ImageLayerTableRowModel::GetDisplayModeValue(MultiChannelDisplayMode &value)
{
  if(m_Layer && m_ImageLayer->GetNumberOfComponents() > 1)
    {
    AbstractMultiChannelDisplayMappingPolicy *dp = dynamic_cast<
        AbstractMultiChannelDisplayMappingPolicy *>(m_Layer->GetDisplayMapping());
    value = dp->GetDisplayMode();
    return true;
    }
  return false;
}

void
ImageLayerTableRowModel::SetDisplayModeValue(MultiChannelDisplayMode value)
{
  AbstractMultiChannelDisplayMappingPolicy *dp = dynamic_cast<
      AbstractMultiChannelDisplayMappingPolicy *>(m_Layer->GetDisplayMapping());
  dp->SetDisplayMode(value);
}

MultiChannelDisplayMode
ImageLayerTableRowModel::GetDisplayMode()
{
  AbstractMultiChannelDisplayMappingPolicy *dp = dynamic_cast<
      AbstractMultiChannelDisplayMappingPolicy *>(m_Layer->GetDisplayMapping());
  return dp->GetDisplayMode();
}

bool
ImageLayerTableRowModel::CheckState(UIState state)
{
  switch (state)
    {
    // Opacity can be edited for all layers except the main image layer
    case AbstractLayerTableRowModel::UIF_OPACITY_EDITABLE:
      return (m_LayerRole != LABEL_ROLE && m_ImageLayer->IsSticky());

    // Pinnable means it's not sticky and may be overlayed (i.e., not main)
    case AbstractLayerTableRowModel::UIF_PINNABLE:
      return (m_LayerRole != MAIN_ROLE && m_LayerRole != LABEL_ROLE && !m_ImageLayer->IsSticky());

    // Unpinnable means it's not sticky and may be overlayed (i.e., not main)
    case AbstractLayerTableRowModel::UIF_UNPINNABLE:
      return (m_LayerRole != MAIN_ROLE && m_LayerRole != LABEL_ROLE && m_ImageLayer->IsSticky());

    case AbstractLayerTableRowModel::UIF_MULTICOMPONENT:
      return (m_Layer && m_ImageLayer->GetNumberOfComponents() > 1);

    case AbstractLayerTableRowModel::UIF_CONTRAST_ADJUSTABLE:
      {
      return (m_Layer && m_Layer->GetDisplayMapping()->GetIntensityCurve());
      }


    case AbstractLayerTableRowModel::UIF_COLORMAP_ADJUSTABLE:
      return (m_Layer && m_Layer->GetDisplayMapping()->GetColorMap());

    case AbstractLayerTableRowModel::UIF_VOLUME_RENDERABLE:
      return m_LayerRole != LABEL_ROLE;

    case AbstractLayerTableRowModel::UIF_IMAGE:
      return true;

    case AbstractLayerTableRowModel::UIF_SAVABLE:
      return true;

    default:
      return Superclass::CheckState(state); // Children override Parents
    }

  return false;
}

void
ImageLayerTableRowModel::Initialize(GlobalUIModel *parentModel, WrapperBase *layer)
{
  // Initialize superclass first
  Superclass::Initialize(parentModel, layer);

  // Downcast wrapper. It has to be a ImageWrapperBase or raise error
  ImageWrapperBase *img_wrapper = static_cast<ImageWrapperBase*>(layer);
  m_ImageLayer = img_wrapper;

  // Update the list of display modes (again, should not change during the
  // lifetime of this object
  UpdateDisplayModeList();
}

void
ImageLayerTableRowModel::SetActivated(bool activated)
{
  // Odd why this would ever happen, but it does...
  if(!m_ImageLayer)
    return;

  // If the layer is selected and is not sticky, we set is as the currently visible
  // layer in the render views
  if(m_LayerRole == LABEL_ROLE && activated)
    {
    m_ParentModel->GetGlobalState()->SetSelectedSegmentationLayerId(m_ImageLayer->GetUniqueId());
    }
  else if(activated && !m_ImageLayer->IsSticky())
    {
    m_ParentModel->GetGlobalState()->SetSelectedLayerId(m_ImageLayer->GetUniqueId());
    }
}

bool
ImageLayerTableRowModel::IsActivated() const
{
  // Odd why this would ever happen, but it does...
  if(!m_Layer)
    return false;

  // Selection is based on id
  unsigned long uid = m_ImageLayer->GetUniqueId();

  // If the layer is selected and is not sticky, we set is as the currently visible
  // layer in the render views
  if(m_LayerRole == LABEL_ROLE)
    {
    if(uid == m_ParentModel->GetGlobalState()->GetSelectedSegmentationLayerId())
      return true;
    }
  else
    {
    if(uid == m_ParentModel->GetGlobalState()->GetSelectedLayerId())
      return true;
    }

  return false;
}

bool
ImageLayerTableRowModel::GetComponentNameValue(std::string &value)
{
  // Get the name of the compomnent
  if(m_ImageLayer && m_ImageLayer->GetNumberOfComponents() > 1)
    {
    value = this->GetDisplayModeString(this->GetDisplayMode());
    return true;
    }

  return false;
}

std::string
ImageLayerTableRowModel::GetDisplayModeString(const MultiChannelDisplayMode &mode)
{
  if(mode.UseRGB)
    {
    return "RGB";
    }

  if(mode.RenderAsGrid)
    {
    return "Grid";
    }

  std::ostringstream oss;
  switch(mode.SelectedScalarRep)
    {
    case SCALAR_REP_COMPONENT:
      oss << (1 + mode.SelectedComponent) << "/" << m_ImageLayer->GetNumberOfComponents();
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

void
ImageLayerTableRowModel::MoveLayerUp()
{
  m_ParentModel->GetDriver()->ChangeOverlayPosition(m_ImageLayer, -1);
}

void
ImageLayerTableRowModel::MoveLayerDown()
{
  m_ParentModel->GetDriver()->ChangeOverlayPosition(m_ImageLayer, +1);
}

void
ImageLayerTableRowModel::UpdateRoleInfo()
{
  LayerIterator it(m_ImageData);
  it.Find(static_cast<ImageWrapperBase*>(m_Layer.GetPointer()));
  if(!it.IsAtEnd())
    {
    m_LayerRole = it.GetRole();
    m_LayerPositionInRole = it.GetPositionInRole();
    m_LayerNumberOfLayersInRole = it.GetNumberOfLayersInRole();
    }
}

bool
ImageLayerTableRowModel::GetStickyValue(bool &value)
{
  if(!m_Layer) return false;

  value = m_ImageLayer->IsSticky();
  return true;
}

void
ImageLayerTableRowModel::SetStickyValue(bool value)
{
  // Make sure the selected ID is legitimate
  if(m_ParentModel->GetGlobalState()->GetSelectedLayerId() == m_ImageLayer->GetUniqueId())
    {
    m_ParentModel->GetGlobalState()->SetSelectedLayerId(
          m_ParentModel->GetDriver()->GetCurrentImageData()->GetMain()->GetUniqueId());
    }
  m_ImageLayer->SetSticky(value);
}

SmartPtr<ImageIOWizardModel>
ImageLayerTableRowModel::CreateIOWizardModelForSave()
{
  return m_ParentModel->CreateIOWizardModelForSave(m_ImageLayer, m_LayerRole);
}

bool
ImageLayerTableRowModel::GetColorMapPresetValue(std::string &value)
{
  if(m_Layer && m_Layer->GetDisplayMapping()->GetColorMap())
    {
    value = ColorMap::GetPresetName(
          m_Layer->GetDisplayMapping()->GetColorMap()->GetSystemPreset());
    return true;
    }
  return false;
}

void
ImageLayerTableRowModel::SetColorMapPresetValue(std::string value)
{
  // TODO: this is a cheat! The current way of handling color map presets in
  // snap is hacky, and I really don't like it. We need a common class that
  // can handle system and user presets for a variety of objects, one that
  // can interface nicely with the GUI models. Here we are going through
  // the functionality provided by the ColorMapModel class.
  ColorMapModel *cmm = m_ParentModel->GetColorMapModel();
  WrapperBase *currentLayer = cmm->GetLayer();
  cmm->SetLayer(m_ImageLayer);
  cmm->SelectPreset(value);
  cmm->SetLayer(currentLayer);
}

void
ImageLayerTableRowModel::AutoAdjustContrast()
{
  if(m_Layer && m_Layer->GetDisplayMapping()->GetIntensityCurve())
    {
    // TODO: this is a bit of a hack, since we go through a different model
    // and have to swap out that model's current layer, which adds some overhead
    IntensityCurveModel *icm = m_ParentModel->GetIntensityCurveModel();
    WrapperBase *currentLayer = icm->GetLayer();
    icm->SetLayer(m_ImageLayer);
    icm->OnAutoFitWindow();
    icm->SetLayer(currentLayer);
    }
}

void
ImageLayerTableRowModel::GenerateTextureFeatures()
{
  ScalarImageWrapperBase *scalar = dynamic_cast<ScalarImageWrapperBase *>(m_Layer.GetPointer());
  if(scalar)
    {
    // Get the image out
    SmartPtr<const ScalarImageWrapperBase::CommonFormatImageType> common_rep =
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
    typedef AnatomicImageWrapperTraits<GreyType>::Image4DType TextureImage4DType;
    typedef bilwaj::MomentTextureFilter<
        ScalarImageWrapperBase::CommonFormatImageType,
        TextureImageType> MomentFilterType;

    MomentFilterType::Pointer filter = MomentFilterType::New();
    filter->SetInput(common_rep);
    filter->SetRadius(radius);
    filter->SetHighestDegree(3);

    // Create a new image wrapper
    SmartPtr<AnatomicImageWrapper> newWrapper = AnatomicImageWrapper::New();

    // Up the image dimension
    typedef IncreaseDimensionImageFilter<TextureImageType, TextureImage4DType> UpDimFilter;
    typename UpDimFilter::Pointer updim = UpDimFilter::New();
    updim->SetInput(filter->GetOutput());
    updim->Update();

    newWrapper->InitializeToWrapper(m_ImageLayer, updim->GetOutput(), NULL, NULL);
    newWrapper->SetDefaultNickname("Textures");
    this->GetParentModel()->GetDriver()->AddDerivedOverlayImage(
          m_ImageLayer, newWrapper, false);
    }
}

void
ImageLayerTableRowModel::CloseLayer()
{
  // If this is an overlay, we unload it like this
  if(m_LayerRole == OVERLAY_ROLE)
    m_ParentModel->GetDriver()->UnloadOverlay(m_ImageLayer);

  // The main image can also be unloaded
  else if(m_LayerRole == MAIN_ROLE)
    m_ParentModel->GetDriver()->UnloadMainImage();

  // Segmentations can be unloaded
  else if (m_LayerRole == LABEL_ROLE)
    m_ParentModel->GetDriver()->UnloadSegmentation(m_ImageLayer);

  m_ImageLayer = NULL;
  m_Layer = NULL;
}

bool ImageLayerTableRowModel::GetVolumeRenderingEnabledValue(bool &value)
{
  auto imageLayer = dynamic_cast<ImageWrapperBase*>(m_Layer.GetPointer());
  if(imageLayer)
    {
    value = imageLayer->GetDefaultScalarRepresentation()->IsVolumeRenderingEnabled();
    return true;
    }
  return false;
}

void ImageLayerTableRowModel::SetVolumeRenderingEnabledValue(bool value)
{
  auto imageLayer = dynamic_cast<ImageWrapperBase*>(m_Layer.GetPointer());
  if(imageLayer)
    imageLayer->GetDefaultScalarRepresentation()->SetVolumeRenderingEnabled(value);
}

/*
 * ===============================================
 *   MeshLayerTableRowModel Implementation
 * ===============================================
*/
MeshLayerTableRowModel
::MeshLayerTableRowModel()
{

}

void
MeshLayerTableRowModel::Initialize(GlobalUIModel *parentModel, WrapperBase *layer)
{
  Superclass::Initialize(parentModel, layer);

  // Downcast wrapper. It has to be a MeshWrapperBase or raise error
  MeshWrapperBase *mesh_wrapper = static_cast<MeshWrapperBase*>(layer);
  m_MeshLayer = mesh_wrapper;
}

bool
MeshLayerTableRowModel::CheckState(UIState state)
{
  bool hasGenericDMP = (dynamic_cast<GenericMeshDisplayMappingPolicy*>(
                          m_MeshLayer->GetDisplayMapping()) != NULL);

  switch (state)
    {
    // This is a mesh
    case AbstractLayerTableRowModel::UIF_MESH:
      return true;

    // Allow opacity editing for all mesh layers
    case AbstractLayerTableRowModel::UIF_OPACITY_EDITABLE:
      return false;

    // We don't need to pin a mesh layer for now
    case AbstractLayerTableRowModel::UIF_PINNABLE:
      return false;

    // We don't need to pin a mesh layer for now
    case AbstractLayerTableRowModel::UIF_UNPINNABLE:
      return false;

    case AbstractLayerTableRowModel::UIF_VOLUME_RENDERABLE:
      return false;

    case AbstractLayerTableRowModel::UIF_MULTICOMPONENT:
			{
			auto prop = m_MeshLayer->GetActiveDataArrayProperty();
			if (prop)
				return prop->IsMultiComponent();
			}
    case AbstractLayerTableRowModel::UIF_CONTRAST_ADJUSTABLE:
      return hasGenericDMP;

    case AbstractLayerTableRowModel::UIF_COLORMAP_ADJUSTABLE:
      return hasGenericDMP;

		case AbstractLayerTableRowModel::UIF_MESH_HAS_DATA:
			return hasGenericDMP;

    case AbstractLayerTableRowModel::UIF_SAVABLE:
      return !hasGenericDMP; // Mesh layer is currently read only

    default:
      return Superclass::CheckState(state); // Children override parents
    }

  return false;
}

bool
MeshLayerTableRowModel::GetStickyValue(bool &)
{
  return false;
}

void
MeshLayerTableRowModel::SetStickyValue(bool )
{
}

void
MeshLayerTableRowModel::SetActivated(bool activated)
{
  if(!m_MeshLayer)
    return;

  if (activated)
    {
    m_ImageData->GetMeshLayers()->SetActiveLayerId(m_MeshLayer->GetUniqueId());
    }
}

bool
MeshLayerTableRowModel::IsActivated() const
{
  if(!m_Layer)
    return false;

  return (m_Layer->GetUniqueId() ==
          m_ParentModel->GetDriver()->GetCurrentImageData()->GetMeshLayers()->
          GetActiveLayerId());
}

void
MeshLayerTableRowModel::AutoAdjustContrast()
{
  auto genericDMP = dynamic_cast<GenericMeshDisplayMappingPolicy*>(m_Layer->GetDisplayMapping());
  if(m_Layer && genericDMP && genericDMP->GetIntensityCurve())
    {
    genericDMP->AutoFitContrast();
    }
}

void
MeshLayerTableRowModel::UpdateRoleInfo()
{
  m_LayerRole = MESH_ROLE;
  m_LayerPositionInRole = m_Layer->GetUniqueId();
  m_LayerNumberOfLayersInRole = m_ImageData->GetMeshLayers()->size();
}

void
MeshLayerTableRowModel::CloseLayer()
{
  m_ParentModel->GetDriver()->UnloadMeshLayer(m_MeshLayer->GetUniqueId());
  m_MeshLayer = NULL;
  m_Layer = NULL;
}

bool
MeshLayerTableRowModel::GetColorMapPresetValue(std::string &value)
{
  if(m_Layer && m_Layer->GetDisplayMapping()->GetColorMap())
    {
    value = ColorMap::GetPresetName(
          m_Layer->GetDisplayMapping()->GetColorMap()->GetSystemPreset());
    return true;
    }
  return false;
}

void
MeshLayerTableRowModel::SetColorMapPresetValue(std::string value)
{
  ColorMapModel *cmm = m_ParentModel->GetColorMapModel();
  WrapperBase *currentLayer = cmm->GetLayer();
  cmm->SetLayer(m_MeshLayer);
  cmm->SelectPreset(value);
  cmm->SetLayer(currentLayer);
}
