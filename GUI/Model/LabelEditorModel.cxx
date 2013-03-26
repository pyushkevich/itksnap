#include "LabelEditorModel.h"
#include "IRISException.h"
#include "IRISApplication.h"
#include "IRISException.h"

LabelEditorModel::LabelEditorModel()
{
  // Create a new instance of the model
  m_CurrentLabelModel = ConcreteColorLabelPropertyModel::New();

  // When the value in the model changes, we need to rebroadcast this
  // as a change in the model, so the GUI can update itself
  Rebroadcast(m_CurrentLabelModel, ValueChangedEvent(), ModelUpdateEvent());

  // The model update events should also be rebroadcast as state changes
  Rebroadcast(this, ModelUpdateEvent(), StateMachineChangeEvent());

  // Initialize the wrapper models

  m_CurrentLabelDescriptionModel = wrapGetterSetterPairAsProperty(
        this,
        &Self::GetCurrentLabelDescription,
        &Self::SetCurrentLabelDescription);

  m_CurrentLabelIdModel = wrapGetterSetterPairAsProperty(
        this,
        &Self::GetCurrentLabelIdValueAndRange,
        &Self::SetCurrentLabelId);

  m_CurrentLabelOpacityModel = wrapGetterSetterPairAsProperty(
        this,
        &Self::GetCurrentLabelOpacityValueAndRange,
        &Self::SetCurrentLabelOpacity);

  m_CurrentLabelHiddenStateModel = wrapGetterSetterPairAsProperty(
        this,
        &Self::GetCurrentLabelHiddenState,
        &Self::SetCurrentLabelHiddenState);

  m_CurrentLabelColorModel = wrapGetterSetterPairAsProperty(
        this,
        &Self::GetCurrentLabelColor,
        &Self::SetCurrentLabelColor);

  m_IsForegroundBackgroundModel = wrapGetterSetterPairAsProperty(
        this,
        &Self::GetIsForegroundBackground,
        &Self::SetIsForegroundBackground);
}

void LabelEditorModel::SetParentModel(GlobalUIModel *parent)
{
  // Set the parent model
  m_Parent = parent;
  m_LabelTable = parent->GetDriver()->GetColorLabelTable();

  // Stick the color label information into the domain object
  m_CurrentLabelModel->Initialize(m_LabelTable);
  m_CurrentLabelModel->SetValue(
        parent->GetDriver()->GetGlobalState()->GetDrawingColorLabel());

  // Listen to events
  Rebroadcast(m_LabelTable, SegmentationLabelChangeEvent(), ModelUpdateEvent());

  // Listen to changes in the active label event
  m_IsForegroundBackgroundModel->RebroadcastFromSourceProperty(
        m_Parent->GetGlobalState()->GetDrawingColorLabelModel());

  m_IsForegroundBackgroundModel->RebroadcastFromSourceProperty(
        m_Parent->GetGlobalState()->GetDrawOverFilterModel());
}

bool LabelEditorModel::GetCurrentLabelDescription(std::string &value)
{
  if(GetAndStoreCurrentLabel())
    {
    value = m_SelectedColorLabel.GetLabel();
    return true;
    }
  return false;
}

void LabelEditorModel::SetCurrentLabelDescription(std::string value)
{
  if(GetAndStoreCurrentLabel())
    {
    m_SelectedColorLabel.SetLabel(value.c_str());
    m_LabelTable->SetColorLabel(m_SelectedId, m_SelectedColorLabel);
    }
}

bool LabelEditorModel::GetCurrentLabelIdValueAndRange(
    int &value, NumericValueRange<int> *domain)
{
  // Get the numeric ID of the current label
  if(GetAndStoreCurrentLabel())
    {
    value = m_SelectedId;
    if(domain)
      domain->Set(0, MAX_COLOR_LABELS, 1);
    return true;
    }
  return false;
}

void LabelEditorModel::SetCurrentLabelId(int value)
{
  // This actually requires some funky stuff. Save it for later.
}

bool LabelEditorModel::GetCurrentLabelOpacityValueAndRange(
    int &value, NumericValueRange<int> *domain)
{
  if(GetAndStoreCurrentLabel())
    {
    value = m_SelectedColorLabel.GetAlpha();
    if(domain)
      domain->Set(0, 255, 1);
    return true;
    }
  return false;
}

void LabelEditorModel::SetCurrentLabelOpacity(int value)
{
  if(GetAndStoreCurrentLabel())
    {
    m_SelectedColorLabel.SetAlpha((unsigned char)(value));
    m_LabelTable->SetColorLabel(m_SelectedId, m_SelectedColorLabel);
    }
}

bool LabelEditorModel::GetCurrentLabelHiddenState(iris_vector_fixed<bool, 2> &value)
{
  if(GetAndStoreCurrentLabel())
    {
    value[0] = !m_SelectedColorLabel.IsVisible();
    value[1] = !m_SelectedColorLabel.IsVisibleIn3D();
    return true;
    }
  return false;
}

void LabelEditorModel::SetCurrentLabelHiddenState(iris_vector_fixed<bool, 2> value)
{
  if(GetAndStoreCurrentLabel())
    {
    m_SelectedColorLabel.SetVisible(!value[0]);
    m_SelectedColorLabel.SetVisibleIn3D(!value[1]);
    m_LabelTable->SetColorLabel(m_SelectedId, m_SelectedColorLabel);
    }
}

bool LabelEditorModel::GetCurrentLabelColor(Vector3ui &value)
{
  if(GetAndStoreCurrentLabel())
    {
    value[0] = m_SelectedColorLabel.GetRGB(0);
    value[1] = m_SelectedColorLabel.GetRGB(1);
    value[2] = m_SelectedColorLabel.GetRGB(2);
    return true;
    }
  return false;
}

void LabelEditorModel::SetCurrentLabelColor(Vector3ui value)
{
  if(GetAndStoreCurrentLabel())
    {
    m_SelectedColorLabel.SetRGB((unsigned char) value[0],
                                (unsigned char) value[1],
                                (unsigned char) value[2]);
    m_LabelTable->SetColorLabel(m_SelectedId, m_SelectedColorLabel);
    }
}

bool LabelEditorModel::GetIsForegroundBackground(Vector2b &value)
{
  if(GetAndStoreCurrentLabel())
    {
    LabelType fg = m_Parent->GetGlobalState()->GetDrawingColorLabel();
    DrawOverFilter bg = m_Parent->GetGlobalState()->GetDrawOverFilter();

    value[0] = (fg == m_SelectedId);
    value[1] = (bg.CoverageMode == PAINT_OVER_ONE && bg.DrawOverLabel == m_SelectedId);
    return true;
    }
  return false;
}

void LabelEditorModel::SetIsForegroundBackground(Vector2b value)
{
  GlobalState *gs = m_Parent->GetGlobalState();
  if(GetAndStoreCurrentLabel())
    {
    DrawOverFilter bg = gs->GetDrawOverFilter();

    if(value[0])
      {
      gs->SetDrawingColorLabel(m_SelectedId);
      }
    else
      {
      // Do nothing - there is no default label to switch to...
      }

    if(value[1])
      {
      bg.CoverageMode = PAINT_OVER_ONE;
      bg.DrawOverLabel = m_SelectedId;
      gs->SetDrawOverFilter(bg);
      }
    else
      {
      bg.CoverageMode = PAINT_OVER_ALL;
      bg.DrawOverLabel = 0;
      gs->SetDrawOverFilter(bg);
      }
    }
}



bool LabelEditorModel::GetAndStoreCurrentLabel()
{
  m_SelectedId = m_CurrentLabelModel->GetValue();
  if(m_LabelTable->IsColorLabelValid(m_SelectedId))
    {
    m_SelectedColorLabel = m_LabelTable->GetColorLabel(m_SelectedId);
    return true;
    }
  return false;
}

bool LabelEditorModel::CheckState(LabelEditorModel::UIState state)
{
  bool valid = GetAndStoreCurrentLabel();
  switch(state)
    {
    case UIF_EDITABLE_LABEL_SELECTED:
      return valid && m_SelectedId > 0;
    }
  return true;
}

bool LabelEditorModel::MakeNewLabel(bool copyCurrent)
{
  bool valid = GetAndStoreCurrentLabel();
  if(valid)
    {
    // Find the next insertion spot after the current selection
    LabelType insertpos = m_LabelTable->GetInsertionSpot(m_SelectedId);

    // If the insertion spot returned is zero, throw an exception (no room)
    if(insertpos == 0)
      return false;

    // Create a new label at this position and set the selection to it
    m_LabelTable->SetColorLabelValid(insertpos, true);

    // Duplicate current label if needed
    if(copyCurrent)
      {
      // Append ' copy' to the text of the color label
      std::string title = m_SelectedColorLabel.GetLabel();
      if(title.substr(title.size() - 5) != " copy")
        title += " copy";
      m_SelectedColorLabel.SetLabel(title.c_str());
      m_LabelTable->SetColorLabel(insertpos, m_SelectedColorLabel);
      }

    // Select the new label
    m_CurrentLabelModel->SetValue(insertpos);

    return true;
    }
  return false;
}

bool LabelEditorModel::IsLabelDeletionDestructive()
{
  if(GetAndStoreCurrentLabel())
    return m_Parent->GetDriver()->GetNumberOfVoxelsWithLabel(m_SelectedId) > 0;
  else
    return false;
}

void LabelEditorModel::DeleteCurrentLabel()
{
  if(GetAndStoreCurrentLabel() && m_SelectedId > 0)
    {
    // Get the global state
    GlobalState *gs = m_Parent->GetGlobalState();

    // Compute the next available id that will be selected
    LabelType lnext = m_LabelTable->FindNextValidLabel(m_SelectedId, false);

    // Check if the drawing labe is pointing to the current id
    if(gs->GetDrawingColorLabel() == m_SelectedId)
      gs->SetDrawingColorLabel(lnext);

    // Check for the draw over label as well. Here we use 0 as the replacement
    DrawOverFilter dof = gs->GetDrawOverFilter();
    if(dof.DrawOverLabel == m_SelectedId)
      gs->SetDrawOverFilter(DrawOverFilter(dof.CoverageMode, 0));

    // Replace all the voxels with the current label by zero
    size_t nUpdated = m_Parent->GetDriver()->ReplaceLabel(0, m_SelectedId);

    // If some voxels were removed, reset the undo state, because
    // changes to the label metadata are not undoable operations (not yet)
    if(nUpdated > 0)
      {
      // This operation can not be undone!
      m_Parent->GetDriver()->ClearUndoPoints();
      }

    // Change the current selection
    m_CurrentLabelModel->SetValue(lnext);

    // Invalidate the current label
    m_LabelTable->SetColorLabelValid(m_SelectedId, false);
    }
}

void LabelEditorModel::ResetLabels()
{
  m_LabelTable->InitializeToDefaults();
}

bool LabelEditorModel::ReassignLabelId(LabelType newid)
{
  // Check if the ID is taken
  if(m_LabelTable->IsColorLabelValid(newid))
    return false;

  // Do the relabeling
  if(GetAndStoreCurrentLabel() && m_SelectedId > 0)
    {
    // Get the global state
    GlobalState *gs = m_Parent->GetGlobalState();

    // Create a new valid label and copy current label
    m_LabelTable->SetColorLabelValid(newid, true);
    m_LabelTable->SetColorLabel(newid, m_SelectedColorLabel);

    // Check if the drawing label is pointing to the current id
    if(gs->GetDrawingColorLabel() == m_SelectedId)
      gs->SetDrawingColorLabel(newid);

    // Check for the draw over label as well. Here we use 0 as the replacement
    DrawOverFilter dof = gs->GetDrawOverFilter();
    if(dof.DrawOverLabel == m_SelectedId)
      gs->SetDrawOverFilter(DrawOverFilter(dof.CoverageMode, newid));

    // Reassign the ID
    size_t nUpdated = m_Parent->GetDriver()->ReplaceLabel(newid, m_SelectedId);

    // There is no undo for this
    if(nUpdated > 0)
      {
      // This operation can not be undone!
      m_Parent->GetDriver()->ClearUndoPoints();
      }

    // Delete the old label
    m_LabelTable->SetColorLabelValid(m_SelectedId, false);

    // Select the new label
    m_CurrentLabelModel->SetValue(newid);
    }

  return true;
}
