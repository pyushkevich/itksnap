/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: Filename.cxx,v $
  Language:  C++
  Date:      $Date: 2010/10/18 11:25:44 $
  Version:   $Revision: 1.12 $
  Copyright (c) 2011 Paul A. Yushkevich

  This file is part of ITK-SNAP

  ITK-SNAP is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

=========================================================================*/

#include "GlobalUIModel.h"

#include <IRISException.h>
#include <IRISApplication.h>
#include <SNAPAppearanceSettings.h>
#include <GenericSliceModel.h>
#include <OrthogonalSliceCursorNavigationModel.h>
#include <PolygonDrawingModel.h>
#include <AnnotationModel.h>
#include <SnakeROIModel.h>
#include <SliceWindowCoordinator.h>
#include <GenericImageData.h>
#include <GuidedNativeImageIO.h>
#include <ImageIODelegates.h>
#include <IntensityCurveModel.h>
#include <LayerSelectionModel.h>
#include <ColorMapModel.h>
#include <ImageInfoModel.h>
#include <LayerGeneralPropertiesModel.h>
#include <Generic3DModel.h>
#include <LabelEditorModel.h>
#include <CursorInspectionModel.h>
#include <SnakeWizardModel.h>
#include <RandomAccessCollectionModel.h>
#include <UIReporterDelegates.h>
#include <ReorientImageModel.h>
#include <DisplayLayoutModel.h>
#include <PaintbrushModel.h>
#include <PaintbrushSettingsModel.h>
#include "PolygonSettingsModel.h"
#include <SynchronizationModel.h>
#include <SnakeParameterModel.h>
#include <SnakeROIResampleModel.h>
#include "NumericPropertyToggleAdaptor.h"
#include "HistoryManager.h"
#include "MeshExportModel.h"
#include "GlobalPreferencesModel.h"
#include "MeshOptions.h"
#include "DefaultBehaviorSettings.h"
#include "SNAPAppearanceSettings.h"
#include "ImageIOWizardModel.h"
#include "IntensityCurveInterface.h"
#include "ColorLabelQuickListModel.h"

#include <itksys/SystemTools.hxx>

#include <SNAPUIFlag.h>
#include <SNAPUIFlag.txx>

// Enable this model to be used with the flag engine
template class SNAPUIFlag<GlobalUIModel, UIState>;


GlobalUIModel::GlobalUIModel()
  : AbstractModel()
{
  // Create the appearance settings objects
  m_AppearanceSettings = SNAPAppearanceSettings::New();

  // Global display settings
  m_GlobalDisplaySettings = GlobalDisplaySettings::New();

  // Create the IRIS application login
  m_Driver = IRISApplication::New();

  // Display layout model
  m_DisplayLayoutModel = DisplayLayoutModel::New();
  m_DisplayLayoutModel->SetParentModel(this);

  // Paintbrush settings
  m_PaintbrushSettingsModel = PaintbrushSettingsModel::New();
  m_PaintbrushSettingsModel->SetParentModel(this);

  // Create the slice models
  for (unsigned int i = 0; i < 3; i++)
    {
    m_SliceModel[i] = GenericSliceModel::New();
    m_SliceModel[i]->Initialize(this, i);
    m_CursorNavigationModel[i] =
        OrthogonalSliceCursorNavigationModel::New();
    m_CursorNavigationModel[i]->SetParent(m_SliceModel[i]);

    m_PolygonDrawingModel[i] = PolygonDrawingModel::New();
    m_PolygonDrawingModel[i]->SetParent(m_SliceModel[i]);

    m_SnakeROIModel[i] = SnakeROIModel::New();
    m_SnakeROIModel[i]->SetParent(m_SliceModel[i]);

    m_PaintbrushModel[i] = PaintbrushModel::New();
    m_PaintbrushModel[i]->SetParent(m_SliceModel[i]);

    m_AnnotationModel[i] = AnnotationModel::New();
    m_AnnotationModel[i]->SetParent(m_SliceModel[i]);
    }


  // Polygon settings
  m_PolygonSettingsModel = PolygonSettingsModel::New();
  m_PolygonSettingsModel->SetParentModel(this);

  // Connect them together with the coordinator
  m_SliceCoordinator = SliceWindowCoordinator::New();
  m_SliceCoordinator->SetParentModel(this);

  // Intensity curve model
  m_IntensityCurveModel = IntensityCurveModel::New();
  m_IntensityCurveModel->SetParentModel(this);

  // Color map model
  m_ColorMapModel = ColorMapModel::New();
  m_ColorMapModel->SetParentModel(this);

  // Image info model
  m_ImageInfoModel = ImageInfoModel::New();
  m_ImageInfoModel->SetParentModel(this);

  // Component selection
  m_LayerGeneralPropertiesModel = LayerGeneralPropertiesModel::New();
  m_LayerGeneralPropertiesModel->SetParentModel(this);

  // Layer selections
  m_LoadedLayersSelectionModel = LayerSelectionModel::New();
  m_LoadedLayersSelectionModel->SetParentModel(this);
  m_LoadedLayersSelectionModel->SetRoleFilter(
        MAIN_ROLE | OVERLAY_ROLE |
        SNAP_ROLE);

  // 3D model
  m_Model3D = Generic3DModel::New();
  m_Model3D->Initialize(this);

  // Label editor model
  m_LabelEditorModel = LabelEditorModel::New();
  m_LabelEditorModel->SetParentModel(this);

  // Reorient image model
  m_ReorientImageModel = ReorientImageModel::New();
  m_ReorientImageModel->SetParentModel(this);

  // Cursor inspection
  m_CursorInspectionModel = CursorInspectionModel::New();
  m_CursorInspectionModel->SetParentModel(this);

  // Snake model
  m_SnakeWizardModel = SnakeWizardModel::New();
  m_SnakeWizardModel->SetParentModel(this);

  // Snake ROI resampling model
  m_SnakeROIResampleModel = SnakeROIResampleModel::New();
  m_SnakeROIResampleModel->SetParentModel(this);

  // Synchronization model
  m_SynchronizationModel = SynchronizationModel::New();
  m_SynchronizationModel->SetParentModel(this);

  // Snake parameter model
  m_SnakeParameterModel = SnakeParameterModel::New();
  m_SnakeParameterModel->SetParentModel(this);

  // Mesh export model
  m_MeshExportModel = MeshExportModel::New();
  m_MeshExportModel->SetParentModel(this);

  // Global prefs model
  m_GlobalPreferencesModel = GlobalPreferencesModel::New();
  m_GlobalPreferencesModel->SetParentModel(this);

  // Quick list of color labels
  m_ColorLabelQuickListModel = ColorLabelQuickListModel::New();
  m_ColorLabelQuickListModel->SetParentModel(this);

  // Set up the cursor position model
  m_CursorPositionModel = wrapGetterSetterPairAsProperty(
        this,
        &Self::GetCursorPositionValueAndRange,
        &Self::SetCursorPosition);

  // The model needs to rebroadcast cusror change events as value changes. This
  // is because unlike other more specific models, GlobalUIModel does not fire
  // ModelUpdateEvent objects.
  m_CursorPositionModel->Rebroadcast(
        this, CursorUpdateEvent(), ValueChangedEvent());
  m_CursorPositionModel->Rebroadcast(
        m_Driver, MainImageDimensionsChangeEvent(), DomainChangedEvent());

  // ROI size and index models
  m_SnakeROIIndexModel = wrapGetterSetterPairAsProperty(
        this,
        &Self::GetSnakeROIIndexValueAndRange,
        &Self::SetSnakeROIIndexValue);

  m_SnakeROIIndexModel->Rebroadcast(
        m_Driver->GetGlobalState()->GetSegmentationROISettingsModel(),
        ValueChangedEvent(), ValueChangedEvent());

  m_SnakeROIIndexModel->Rebroadcast(
        m_Driver, MainImageDimensionsChangeEvent(), DomainChangedEvent());

  m_SnakeROISizeModel = wrapGetterSetterPairAsProperty(
        this,
        &Self::GetSnakeROISizeValueAndRange,
        &Self::SetSnakeROISizeValue);

  m_SnakeROISizeModel->Rebroadcast(
        m_Driver->GetGlobalState()->GetSegmentationROISettingsModel(),
        ValueChangedEvent(), ValueChangedEvent());

  m_SnakeROISizeModel->Rebroadcast(
        m_Driver, MainImageDimensionsChangeEvent(), DomainChangedEvent());


  // Segmentation opacity models
  m_SegmentationOpacityModel = wrapGetterSetterPairAsProperty(
        this,
        &Self::GetSegmentationOpacityValueAndRange,
        &Self::SetSegmentationOpacityValue);

  m_SegmentationOpacityModel->RebroadcastFromSourceProperty(
        m_Driver->GetGlobalState()->GetSegmentationAlphaModel());

  m_SegmentationVisibilityModel =
      NewNumericPropertyToggleAdaptor(m_SegmentationOpacityModel.GetPointer(), 0, 50);

  // Listen to state changes from the slice coordinator
  Rebroadcast(m_SliceCoordinator, LinkedZoomUpdateEvent(), LinkedZoomUpdateEvent());
  Rebroadcast(m_SliceCoordinator, LinkedZoomUpdateEvent(), StateMachineChangeEvent());

  // Rebroadcast cursor change events
  Rebroadcast(m_Driver, CursorUpdateEvent(), CursorUpdateEvent());

  // Rebroadcast image layer change events
  Rebroadcast(m_Driver, LayerChangeEvent(), LayerChangeEvent());
  Rebroadcast(m_Driver, LayerChangeEvent(), StateMachineChangeEvent());

  // Rebroadcast image layer change events
  Rebroadcast(m_Driver, WrapperMetadataChangeEvent(), StateMachineChangeEvent());

  // Rebroadcast toolbar model change events (TODO: needed?)
  Rebroadcast(m_Driver->GetGlobalState()->GetToolbarModeModel(),
              ValueChangedEvent(), ToolbarModeChangeEvent());

  // All the events that result in the voxel under the cursor changing
  Rebroadcast(this, CursorUpdateEvent(), LabelUnderCursorChangedEvent());
  Rebroadcast(m_Driver->GetColorLabelTable(), SegmentationLabelChangeEvent(),
              LabelUnderCursorChangedEvent());

  Rebroadcast(m_Driver, SegmentationChangeEvent(), LabelUnderCursorChangedEvent());
  Rebroadcast(m_Driver, SegmentationChangeEvent(), StateMachineChangeEvent());

  // Segmentation ROI event
  Rebroadcast(m_Driver->GetGlobalState()->GetSegmentationROISettingsModel(),
              ValueChangedEvent(), SegmentationROIChangedEvent());

  // The initial reporter delegate is NULL
  m_ProgressReporterDelegate = NULL;

  // Initialize the progress reporting command
  SmartPtr<itk::MemberCommand<Self> > progcmd = itk::MemberCommand<Self>::New();
  progcmd->SetCallbackFunction(this, &GlobalUIModel::ProgressCallback);
  m_ProgressCommand = progcmd.GetPointer();


}

GlobalUIModel::~GlobalUIModel()
{
}

bool GlobalUIModel::CheckState(UIState state)
{
  // TODO: implement all the other cases

  // TODO: the flag values need to be cached and updated in response to incoming
  // events. Otherwise, there are just too many of these calls happening. Alternatively,
  // each state could be handled as a separate model.
  switch(state)
    {
    case UIF_BASEIMG_LOADED:
      return m_Driver->IsMainImageLoaded();
    case UIF_IRIS_WITH_BASEIMG_LOADED:
      return m_Driver->IsMainImageLoaded() && !m_Driver->IsSnakeModeActive();
    case UIF_IRIS_MODE:
      return !m_Driver->IsSnakeModeActive();
    case UIF_IRIS_WITH_OVERLAY_LOADED:
      return m_Driver->IsMainImageLoaded() && !m_Driver->IsSnakeModeActive()
          && m_Driver->GetCurrentImageData()->GetNumberOfOverlays() > 0;
    case UIF_ROI_VALID:
      break;
    case UIF_LINKED_ZOOM:
      return m_SliceCoordinator->GetLinkedZoom();
    case UIF_UNDO_POSSIBLE:
      return m_Driver->IsUndoPossible();
    case UIF_REDO_POSSIBLE:
      return m_Driver->IsRedoPossible();
    case UIF_UNSAVED_CHANGES:
      break;
    case UIF_MESH_SAVEABLE:
      break;
    case UIF_OVERLAY_LOADED:
      return m_Driver->GetCurrentImageData()->IsOverlayLoaded();
    case UIF_SNAKE_MODE:
      return m_Driver->IsSnakeModeActive();
    case UIF_LEVEL_SET_ACTIVE:
      return m_Driver->IsSnakeModeLevelSetActive();
    case UIF_MULTIPLE_BASE_LAYERS:
      {
      LayerIterator it = m_Driver->GetCurrentImageData()->GetLayers(
                           MAIN_ROLE | OVERLAY_ROLE | SNAP_ROLE);
      int n = 0;
      for(; !it.IsAtEnd(); ++it)
        if(it.GetLayer() && !it.GetLayer()->IsSticky())
          ++n;

      return n > 1;
      }
    }

  return false;
}

void GlobalUIModel::AutoContrastAllLayers()
{
  GenericImageData *id = m_Driver->GetCurrentImageData();
  for(LayerIterator it = id->GetLayers(MAIN_ROLE | OVERLAY_ROLE); !it.IsAtEnd(); ++it)
    {
    ImageWrapperBase *layer = it.GetLayer();
    AbstractContinuousImageDisplayMappingPolicy *policy =
        dynamic_cast<AbstractContinuousImageDisplayMappingPolicy *>(layer->GetDisplayMapping());
    if(policy)
      policy->AutoFitContrast();
    }
}

void GlobalUIModel::ResetContrastAllLayers()
{
  GenericImageData *id = m_Driver->GetCurrentImageData();
  for(LayerIterator it = id->GetLayers(MAIN_ROLE | OVERLAY_ROLE); !it.IsAtEnd(); ++it)
    {
    ImageWrapperBase *layer = it.GetLayer();
    AbstractContinuousImageDisplayMappingPolicy *policy =
        dynamic_cast<AbstractContinuousImageDisplayMappingPolicy *>(layer->GetDisplayMapping());
    if(policy && policy->GetIntensityCurve())
      policy->GetIntensityCurve()->Reset();
    }
}

void GlobalUIModel::ToggleOverlayVisibility()
{
  // Are we in tiled mode or in stack mode?
  GenericImageData *id = m_Driver->GetCurrentImageData();

  // Remember what layer is current in the general properties model
  ImageWrapperBase *curr_layer = m_LayerGeneralPropertiesModel->GetLayer();

  // Apply the toggle for all overlays
  for(LayerIterator it = id->GetLayers(MAIN_ROLE | OVERLAY_ROLE | SNAP_ROLE); !it.IsAtEnd(); ++it)
    {
    // In stack mode, every overlay is affected. In tile mode, only stickly layers
    // are affected
    if(it.GetLayer()->IsSticky())
      {
      m_LayerGeneralPropertiesModel->SetLayer(it.GetLayer());
      m_LayerGeneralPropertiesModel->GetLayerVisibilityModel()->SetValue(
            !m_LayerGeneralPropertiesModel->GetLayerVisibilityModel()->GetValue());
      }
    }

  // Restore the active layer
  m_LayerGeneralPropertiesModel->SetLayer(curr_layer);
}

void GlobalUIModel::AdjustOverlayOpacity(int delta)
{
  // Are we in tiled mode or in stack mode?
  GenericImageData *id = m_Driver->GetCurrentImageData();

  // Remember what layer is current in the general properties model
  ImageWrapperBase *curr_layer = m_LayerGeneralPropertiesModel->GetLayer();

  // Apply the toggle for all overlays
  for(LayerIterator it = id->GetLayers(MAIN_ROLE | OVERLAY_ROLE | SNAP_ROLE); !it.IsAtEnd(); ++it)
    {
    // In stack mode, every overlay is affected. In tile mode, only stickly layers
    // are affected
    if(it.GetLayer()->IsSticky())
      {
      m_LayerGeneralPropertiesModel->SetLayer(it.GetLayer());
      int op = m_LayerGeneralPropertiesModel->GetLayerOpacityModel()->GetValue();
      int op_new = std::min(100, std::max(0, op + delta));
      m_LayerGeneralPropertiesModel->GetLayerOpacityModel()->SetValue(op_new);
      }
    }

  // Restore the active layer
  m_LayerGeneralPropertiesModel->SetLayer(curr_layer);
}


void GlobalUIModel::SetGlobalDisplaySettings(
    const GlobalDisplaySettings *settings)
{
  // Check if the settings affecting the slice RAI codes have changed
  std::string raiOld[3], raiNew[3];
  m_GlobalDisplaySettings->GetAnatomyToDisplayTransforms(raiOld[0], raiOld[1], raiOld[2]);
  settings->GetAnatomyToDisplayTransforms(raiNew[0], raiNew[1], raiNew[2]);

  // Update the global display settings
  m_GlobalDisplaySettings->DeepCopy(settings);

  // React to the change in RAI codes
  if(raiOld[0] != raiNew[0] || raiOld[1] != raiNew[1] || raiOld[2] != raiNew[2])
    {
    // Update the RAI codes in all slice views
    m_Driver->SetDisplayGeometry(IRISDisplayGeometry(raiNew[0], raiNew[1], raiNew[2]));

    // Update the cursor location
    if(m_Driver->IsMainImageLoaded())
      {
      // Update the cursor position (forced)
      m_Driver->SetCursorPosition(m_Driver->GetCursorPosition(), true);

      // Reinitialize all the slice views
      for(int i = 0; i < 3; i++)
        m_SliceModel[i]->InitializeSlice(m_Driver->GetCurrentImageData());

      // Recenter all views
      m_SliceCoordinator->ResetViewToFitInAllWindows();
      }
    }
}

SystemInterface * GlobalUIModel::GetSystemInterface() const
{
  return m_Driver->GetSystemInterface();
}

GlobalState * GlobalUIModel::GetGlobalState() const
{
  return m_Driver->GetGlobalState();
}

#include "SynchronizationModel.h"

void GlobalUIModel::LoadUserPreferences()
{
  SystemInterface *si = m_Driver->GetSystemInterface();

  DefaultBehaviorSettings *dbs =
      m_Driver->GetGlobalState()->GetDefaultBehaviorSettings();

  // Load the user preferences from the file system
  si->LoadUserPreferences();

  // Read the appearance settings
  m_AppearanceSettings->LoadFromRegistry(
        si->Folder("UserInterface.Appearance"));

  // Read the default behaviors
  dbs->ReadFromRegistry(
        si->Folder("UserInterface.DefaultBehavior"));

  // Read the global display properties
  m_GlobalDisplaySettings->ReadFromRegistry(
        si->Folder("SliceView.DisplaySettings"));

  // Read the 3D mesh options
  m_Driver->GetGlobalState()->GetMeshOptions()->ReadFromRegistry(
        si->Folder("View3D.MeshOptions"));

  // At this point we should check if the color map preset in the prefs
  // is still a valid preset, and if it isn't, replace it with a default
  if(!m_Driver->GetColorMapPresetManager()->IsValidPreset(
       dbs->GetOverlayColorMapPreset()))
    {
    dbs->SetOverlayColorMapPreset(ColorMap::GetPresetName(ColorMap::COLORMAP_GREY));
    }

  // Apply the default startup behaviors
  m_SliceCoordinator->SetLinkedZoom(dbs->GetLinkedZoom());
  m_SynchronizationModel->SetSyncEnabled(dbs->GetSynchronization());
  m_SynchronizationModel->SetSyncCursor(dbs->GetSyncCursor());
  m_SynchronizationModel->SetSyncZoom(dbs->GetSyncZoom());
  m_SynchronizationModel->SetSyncPan(dbs->GetSyncPan());
  m_Model3D->SetContinuousUpdate(dbs->GetContinuousMeshUpdate());
  m_Driver->GetGlobalState()->SetSliceViewLayerLayout(dbs->GetOverlayLayout());
}

void GlobalUIModel::SaveUserPreferences()
{
  SystemInterface *si = m_Driver->GetSystemInterface();

  // Read the appearance settings
  m_AppearanceSettings->SaveToRegistry(
        si->Folder("UserInterface.Appearance"));

  // Read the default behaviors
  m_Driver->GetGlobalState()->GetDefaultBehaviorSettings()->WriteToRegistry(
        si->Folder("UserInterface.DefaultBehavior"));

  // Read the global display properties
  m_GlobalDisplaySettings->WriteToRegistry(
        si->Folder("SliceView.DisplaySettings"));

  // Read the 3D mesh options
  m_Driver->GetGlobalState()->GetMeshOptions()->WriteToRegistry(
        si->Folder("View3D.MeshOptions"));

  // Save the preferences
  si->SaveUserPreferences();
}

bool GlobalUIModel::GetCursorPositionValueAndRange(
    Vector3ui &value, NumericValueRange<Vector3ui> *range)
{
  if(m_Driver->IsMainImageLoaded())
    {
    value = m_Driver->GetCursorPosition() + 1u;
    if(range)
      {
      range->Set(Vector3ui(1u),
                 m_Driver->GetCurrentImageData()->GetMain()->GetSize(),
                 Vector3ui(1u));
      }
    return true;
    }

  return false;
}

void GlobalUIModel::SetCursorPosition(Vector3ui value)
{
  m_Driver->SetCursorPosition(value - 1u);
}

bool GlobalUIModel::GetSnakeROIIndexValueAndRange(
    Vector3ui &value, NumericValueRange<Vector3ui> *range)
{
  // There has to be an image
  if(!m_Driver->IsMainImageLoaded())
    return false;

  // Get the image size
  Vector3ui imsize =
      m_Driver->GetCurrentImageData()->GetImageRegion().GetSize();

  // Get the system's region of interest
  GlobalState::RegionType roiSystem =
      m_Driver->GetGlobalState()->GetSegmentationROI();

  // Populate the return value
  for(int i = 0; i < 3; i++)
    {
    value[i] = roiSystem.GetIndex()[i] + 1;
    if(range)
      {
      range->Minimum[i] = 1;
      range->Maximum[i] = imsize[i] - 1;
      range->StepSize[i] = 1;
      }
    }

  return true;
}

void GlobalUIModel::SetSnakeROIIndexValue(Vector3ui value)
{
  // Get the image size
  Vector3ui imsize =
      m_Driver->GetCurrentImageData()->GetImageRegion().GetSize();

  // Get the system's region of interest
  GlobalState::RegionType roi =
      m_Driver->GetGlobalState()->GetSegmentationROI();

  // Index changed, clamp the size
  for(int i = 0; i < 3; i++)
    {
    roi.SetIndex(i, value[i] - 1);
    roi.SetSize(i, std::min(value[i], imsize[i] - value[i]));
    }

  m_Driver->GetGlobalState()->SetSegmentationROI(roi);
}

bool GlobalUIModel::GetSnakeROISizeValueAndRange(
    Vector3ui &value, NumericValueRange<Vector3ui> *range)
{
  // There has to be an image
  if(!m_Driver->IsMainImageLoaded())
    return false;

  // Get the image size
  Vector3ui imsize =
      m_Driver->GetCurrentImageData()->GetImageRegion().GetSize();

  // Get the system's region of interest
  GlobalState::RegionType roiSystem =
      m_Driver->GetGlobalState()->GetSegmentationROI();

  // Populate the return value
  for(int i = 0; i < 3; i++)
    {
    value[i] = roiSystem.GetSize()[i];
    if(range)
      {
      range->Minimum[i] = 1;
      range->Maximum[i] = imsize[i];
      range->StepSize[i] = 1;
      }
    }

  return true;
}

void GlobalUIModel::SetSnakeROISizeValue(Vector3ui value)
{
  // Get the image size
  Vector3ui imsize =
      m_Driver->GetCurrentImageData()->GetImageRegion().GetSize();

  // Get the system's region of interest
  GlobalState::RegionType roi =
      m_Driver->GetGlobalState()->GetSegmentationROI();

  // Size changed, clamp the index
  for(int i = 0; i < 3; i++)
    {
    roi.SetSize(i, value[i]);
    if(value[i] + roi.GetIndex(i) > imsize[i])
      roi.SetIndex(i, imsize[i] - value[1]);
    }

  m_Driver->GetGlobalState()->SetSegmentationROI(roi);
}

bool
GlobalUIModel::GetSegmentationOpacityValueAndRange(
    int &value, NumericValueRange<int> *domain)
{
  // Round the current alpha value to the nearest integer
  double alpha = m_Driver->GetGlobalState()->GetSegmentationAlpha();
  value = (int)(alpha * 100 + 0.5);

  // Set the domain
  if(domain)
    domain->Set(0, 100, 5);

  return true;
}

void GlobalUIModel::SetSegmentationOpacityValue(int value)
{
  m_Driver->GetGlobalState()->SetSegmentationAlpha(value / 100.0);
}


std::vector<std::string>
GlobalUIModel::GetRecentHistoryItems(const char *historyCategory, unsigned int k, bool global_history)
{
  // Load the list of recent files from the history file
  const HistoryManager::HistoryListType &history =
      global_history
      ? this->GetSystemInterface()->GetHistoryManager()->GetGlobalHistory(historyCategory)
      : this->GetSystemInterface()->GetHistoryManager()->GetLocalHistory(historyCategory);

  std::vector<std::string> recent;

  // Take the five most recent items and create menu items
  for(unsigned int i = 0; i < k; i++)
    {
    if(i < history.size())
      {
      recent.push_back(history[history.size() - (i+1)]);
      }
    }

  return recent;
}

bool GlobalUIModel::IsHistoryEmpty(const char *historyCategory)
{
  // Load the list of recent files from the history file
  const HistoryManager::HistoryListType &history =
      this->GetSystemInterface()->GetHistoryManager()->GetGlobalHistory(historyCategory);

  return history.size() == 0;
}

GlobalUIModel::AbstractHistoryModel *
GlobalUIModel::GetHistoryModel(const std::string &category)
{
  return m_Driver->GetHistoryManager()->GetGlobalHistoryModel(category);
}

std::string GlobalUIModel::GenerateScreenshotFilename()
{
  // Get the last screen shot filename used
  std::string last = m_LastScreenshotFileName;
  if(last.length() == 0)
    return "snapshot0001.png";

  // Count how many digits there are at the end of the filename
  std::string noext =
    itksys::SystemTools::GetFilenameWithoutExtension(last);
  unsigned int digits = 0;
  for(int i = noext.length() - 1; i >= 0; i--)
    {
    if(isdigit(noext[i]))
      digits++;
    else break;
    }

  // If there are no digits, return the filename
  if(digits == 0) return last;

  // Get the number at the end of the string
  std::string snum = noext.substr(noext.length() - digits);
  std::istringstream iss(snum);
  unsigned long num = 0;
  iss >> num;

  // Increment the number by one and convert to another string, padding with zeros
  std::ostringstream oss;
  oss << itksys::SystemTools::GetFilenamePath(last);
  oss << "/";
  oss << noext.substr(0, noext.length() - digits);
  oss << std::setw(digits) << std::setfill('0') << (num + 1);
  oss << itksys::SystemTools::GetFilenameExtension(last);
  return oss.str();
}

SmartPtr<ImageIOWizardModel>
GlobalUIModel::CreateIOWizardModelForSave(ImageWrapperBase *layer, LayerRole role)
{
  // Create save delegate for this layer
  SmartPtr<AbstractSaveImageDelegate> delegate =
      m_Driver->CreateSaveDelegateForLayer(layer, role);

  // Figure out the category name
  std::string category;
  switch(role)
    {
    case MAIN_ROLE:
      category = "Image";
      break;
    case OVERLAY_ROLE:
      category = "Image";
      break;
    case SNAP_ROLE:
      if(dynamic_cast<SpeedImageWrapper *>(layer))
        category = "Speed Image";
      else if(dynamic_cast<LevelSetImageWrapper *>(layer))
        category = "Level Set Image";
      break;
    case LABEL_ROLE:
      category = "Segmentation Image";
      break;
    case NO_ROLE:
    case ALL_ROLES:
      break;
    }

  // Create a model for IO
  SmartPtr<ImageIOWizardModel> modelIO = ImageIOWizardModel::New();
  modelIO->InitializeForSave(this, delegate, category.c_str());

  return modelIO;
}

void GlobalUIModel::AnimateLayerComponents()
{
  // TODO: For now all the layers tagged as animating animate. This is the
  // dumbest possible way to do animation, but it's fine for one layer or
  // multiple layers with the same number of components.
  for(LayerIterator it = m_Driver->GetCurrentImageData()->GetLayers();
      !it.IsAtEnd(); ++it)
    {
    if(it.GetLayer()->GetNumberOfComponents() > 1)
      {
      AbstractMultiChannelDisplayMappingPolicy *dp = dynamic_cast<
          AbstractMultiChannelDisplayMappingPolicy *>(
            it.GetLayer()->GetDisplayMapping());

      if(dp && dp->GetAnimate())
        {
        MultiChannelDisplayMode mode = dp->GetDisplayMode();
        if(mode.SelectedScalarRep == SCALAR_REP_COMPONENT)
          {
          mode.SelectedComponent =
              (mode.SelectedComponent + 1) % it.GetLayer()->GetNumberOfComponents();
          dp->SetDisplayMode(mode);
          }
        }
      }
    }
}

void GlobalUIModel::IncrementDrawingColorLabel(int delta)
{
  ColorLabelTable *clt = m_Driver->GetColorLabelTable();
  LabelType current = m_Driver->GetGlobalState()->GetDrawingColorLabel();
  ColorLabelTable::ValidLabelConstIterator pos =
      clt->GetValidLabels().find(current);
  if(delta == 1)
    ++pos;
  else if(delta == -1)
    --pos;

  if(pos != clt->GetValidLabels().end())
    m_Driver->GetGlobalState()->SetDrawingColorLabel(pos->first);
}

void GlobalUIModel::IncrementDrawOverColorLabel(int delta)
{
  // Handle basic cases
  DrawOverFilter dof = m_Driver->GetGlobalState()->GetDrawOverFilter();
  if(dof.CoverageMode == PAINT_OVER_ALL && delta == 1)
    {
    dof.CoverageMode = PAINT_OVER_VISIBLE;
    }
  else if(dof.CoverageMode == PAINT_OVER_VISIBLE && delta == 1)
    {
    dof.CoverageMode = PAINT_OVER_ONE;
    dof.DrawOverLabel = 0;
    }
  else if(dof.CoverageMode == PAINT_OVER_VISIBLE && delta == -1)
    {
    dof.CoverageMode = PAINT_OVER_ALL;
    }
  else if(dof.CoverageMode == PAINT_OVER_ONE && delta == -1 && dof.DrawOverLabel == 0)
    {
    dof.CoverageMode = PAINT_OVER_VISIBLE;
    dof.DrawOverLabel = 0;
    }
  else if(dof.CoverageMode == PAINT_OVER_ONE)
    {
    ColorLabelTable *clt = m_Driver->GetColorLabelTable();
    ColorLabelTable::ValidLabelConstIterator pos =
        clt->GetValidLabels().find(dof.DrawOverLabel);
    if(delta == 1)
      ++pos;
    else if(delta == -1)
      --pos;

    if(pos != clt->GetValidLabels().end())
      dof.DrawOverLabel = pos->first;
    }
  else
    {
    return;
    }

  m_Driver->GetGlobalState()->SetDrawOverFilter(dof);
}

void
GlobalUIModel
::ProgressCallback(itk::Object *source, const itk::EventObject &event)
{
  if(m_ProgressReporterDelegate)
    {
    itk::ProcessObject *po = static_cast<itk::ProcessObject *>(source);
    m_ProgressReporterDelegate->SetProgressValue(po->GetProgress());
    }
}
