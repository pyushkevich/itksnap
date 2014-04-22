#include "CursorInspectionModel.h"
#include "GlobalUIModel.h"
#include "IRISApplication.h"
#include "GenericImageData.h"
#include "ColorLabelTable.h"
#include "IntensityCurveModel.h"
#include "ColorMapModel.h"

#include <QtTableWidgetCoupling.h>

#include <iomanip>

/**
  The domain used to present intensity information about the voxel under
  the cursor

  TODO: eventually, it would be nice to have a tree display for multi-component
  data, so that we can see the values of all components at once.
  */
CurrentVoxelInfoItemSetDomain
::CurrentVoxelInfoItemSetDomain(
    IRISApplication *app, int role_filter)
  : Superclass(app, role_filter), m_Driver(app)
{
}

LayerCurrentVoxelInfo
CurrentVoxelInfoItemSetDomain
::GetDescription(const LayerIterator &it) const
{
  // The output structure
  LayerCurrentVoxelInfo vox;

  // Set the name
  vox.LayerName = it.GetLayer()->GetNickname().c_str();

  // Make sure that the layer is initialized
  if(it.GetLayer()->IsInitialized())
    {
    // Get the cursor position
    Vector3ui cursor = m_Driver->GetCursorPosition();

    // Create a string output
    std::ostringstream oss;

    // Get the intensity or intensities that the user is seeing and the RGB
    vnl_vector<double> v;
    ImageWrapperBase::DisplayPixelType disprgb;
    it.GetLayer()->GetVoxelUnderCursorDisplayedValueAndAppearance(v, disprgb);

    // Print with varying degrees of precision
    if(v.size() == 1)
      {
      oss << std::setprecision(4);
      oss << v[0];
      }
    else if(v.size() == 3)
      {
      oss << std::setprecision(2);
      oss << v[0] << "," << v[1] << "," << v[2];
      }

    vox.IntensityValue = oss.str();

    // Get the displayed color
    vox.Color = Vector3ui(disprgb[0], disprgb[1], disprgb[2]);

    // Return the description
    return vox;
    }
  else
    {
    vox.IntensityValue = "";
    vox.Color = Vector3ui(0);
    }

  return vox;
}

CursorInspectionModel::CursorInspectionModel()
{
  // Create the child models
  m_LabelUnderTheCursorIdModel = wrapGetterSetterPairAsProperty(
        this, &CursorInspectionModel::GetLabelUnderTheCursorIdValue);

  m_LabelUnderTheCursorTitleModel = wrapGetterSetterPairAsProperty(
        this, &CursorInspectionModel::GetLabelUnderTheCursorTitleValue);

  // Create the child model for the intensity list
  m_VoxelAtCursorModel = ConcreteLayerVoxelAtCursorModel::New();
}

void CursorInspectionModel::SetParentModel(GlobalUIModel *parent)
{
  // Set the parent
  m_Parent = parent;

  // Get the app
  IRISApplication *app = parent->GetDriver();

  // Initialize the layer voxel list model
  int role =
      MAIN_ROLE |
      OVERLAY_ROLE |
      SNAP_ROLE;

  CurrentVoxelInfoItemSetDomain dom(app, role);
  m_VoxelAtCursorModel->SetDomain(dom);

  // Make this model listen to events that affect its domain
  m_VoxelAtCursorModel->Rebroadcast(
        app, LayerChangeEvent(), DomainChangedEvent());
  m_VoxelAtCursorModel->Rebroadcast(
        this, ModelUpdateEvent(), DomainDescriptionChangedEvent());
  m_VoxelAtCursorModel->Rebroadcast(
        app, WrapperDisplayMappingChangeEvent(), DomainDescriptionChangedEvent());
  m_VoxelAtCursorModel->Rebroadcast(
        app, WrapperMetadataChangeEvent(), DomainDescriptionChangedEvent());

  // Rebroadcast events from the parent as model update events. This could
  // have a little more granularity, but for the moment, mapping all these
  // events to a ModelUpdateEvent seems fine.
  Rebroadcast(app, CursorUpdateEvent(), ModelUpdateEvent());
  Rebroadcast(app, LayerChangeEvent(), ModelUpdateEvent());
  Rebroadcast(app->GetColorLabelTable(),
              SegmentationLabelChangeEvent(), ModelUpdateEvent());
  Rebroadcast(app, SegmentationChangeEvent(), ModelUpdateEvent());


}

bool CursorInspectionModel::GetLabelUnderTheCursorIdValue(LabelType &value)
{
  IRISApplication *app = m_Parent->GetDriver();
  GenericImageData *id = app->GetCurrentImageData();
  if(id->IsSegmentationLoaded())
    {
    value = id->GetSegmentation()->GetVoxel(app->GetCursorPosition());
    return true;
    }
  return false;
}

bool CursorInspectionModel::GetLabelUnderTheCursorTitleValue(std::string &value)
{
  IRISApplication *app = m_Parent->GetDriver();
  GenericImageData *id = m_Parent->GetDriver()->GetCurrentImageData();
  if(id->IsSegmentationLoaded())
    {
    LabelType label = id->GetSegmentation()->GetVoxel(app->GetCursorPosition());
    value = app->GetColorLabelTable()->GetColorLabel(label).GetLabel();
    return true;
    }
  return false;
}

AbstractRangedUIntVec3Property * CursorInspectionModel::GetCursorPositionModel() const
{
  return m_Parent->GetCursorPositionModel();
}


