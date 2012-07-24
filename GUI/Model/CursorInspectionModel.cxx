#include "CursorInspectionModel.h"
#include "GlobalUIModel.h"
#include "IRISApplication.h"
#include "GenericImageData.h"
#include "ColorLabelTable.h"

#include <QtTableWidgetCoupling.h>

#include <iomanip>

/**
  The domain used to present intensity information about the voxel under
  the cursor
  */
CurrentVoxelInfoItemSetDomain
::CurrentVoxelInfoItemSetDomain(
    IRISApplication *app, int role_filter)
  : Superclass(app == NULL ? NULL : app->GetCurrentImageData(), role_filter),
    m_Driver(app)
{
}

LayerCurrentVoxelInfo
CurrentVoxelInfoItemSetDomain
::GetDescription(const LayerIterator &it) const
{
  LayerCurrentVoxelInfo vox;

  // Set the name
  vox.LayerName = it.GetDynamicNickname().c_str();

  // Get the cursor position
  Vector3ui cursor = m_Driver->GetCursorPosition();

  // Create a string output
  std::ostringstream oss;

  // See if this layer is grey
  if(GreyImageWrapperBase *giw = it.GetLayerAsGray())
    {
    oss << std::setprecision(4);
    oss << giw->GetVoxelMappedToNative(cursor);
    }
  else if(RGBImageWrapperBase *rgbiw = it.GetLayerAsRGB())
    {
    double rgb[3];
    rgbiw->GetVoxelAsDouble(cursor, rgb);
    oss << rgb[0] << "," << rgb[1] << "," << rgb[2];
    }

  vox.IntensityValue = oss.str();

  return vox;
}

CursorInspectionModel::CursorInspectionModel()
{
  // Create the child models
  m_LabelUnderTheCursorIdModel = makeChildPropertyModel(
        this, &CursorInspectionModel::GetLabelUnderTheCursorIdValue);

  m_LabelUnderTheCursorTitleModel = makeChildPropertyModel(
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
  int role = LayerIterator::MAIN_ROLE | LayerIterator::OVERLAY_ROLE;
  CurrentVoxelInfoItemSetDomain dom(app, role);
  m_VoxelAtCursorModel->SetDomain(dom);

  // Make this model listen to events that affect its domain
  m_VoxelAtCursorModel->Rebroadcast(
        app, LayerChangeEvent(), DomainChangedEvent());
  m_VoxelAtCursorModel->Rebroadcast(
        this, ModelUpdateEvent(), DomainDescriptionChangedEvent());

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


