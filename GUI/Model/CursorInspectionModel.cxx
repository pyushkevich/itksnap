#include "CursorInspectionModel.h"
#include "GlobalUIModel.h"
#include "IRISApplication.h"
#include "GenericImageData.h"
#include "ColorLabelTable.h"

CursorInspectionModel::CursorInspectionModel()
{
  // Create the child models
  m_LabelUnderTheCursorIdModel = makeChildPropertyModel(
        this, &CursorInspectionModel::GetLabelUnderTheCursorIdValue);

  m_LabelUnderTheCursorTitleModel = makeChildPropertyModel(
        this, &CursorInspectionModel::GetLabelUnderTheCursorTitleValue);

}

void CursorInspectionModel::SetParentModel(GlobalUIModel *parent)
{
  // Set the parent
  m_Parent = parent;

  // Get the app
  IRISApplication *app = parent->GetDriver();

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
