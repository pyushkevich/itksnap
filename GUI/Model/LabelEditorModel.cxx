#include "LabelEditorModel.h"
#include "IRISApplication.h"

LabelEditorModel::LabelEditorModel()
{
  // Initialize the models
  m_CurrentLabelModel = ConcreteColorLabelPropertyModel::New();

  // When the value in the model changes, we need to rebroadcast this
  // as a change in the model, so the GUI can update itself
  Rebroadcast(m_CurrentLabelModel, ValueChangedEvent(), ModelUpdateEvent());

  // Initialize the wrapper models
  m_CurrentLabelDescriptionModel = makeChildPropertyModel(
        this,
        &Self::GetCurrentLabelDescription,
        &Self::SetCurrentLabelDescription);

  m_CurrentLabelIdModel = makeChildPropertyModel(
        this,
        &Self::GetCurrentLabelIdValueAndRange,
        &Self::SetCurrentLabelId);

  m_CurrentLabelOpacityModel = makeChildPropertyModel(
        this,
        &Self::GetCurrentLabelOpacityValueAndRange,
        &Self::SetCurrentLabelOpacity);
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
}

bool LabelEditorModel::GetCurrentLabelDescription(std::string &value)
{
  LabelType sel = m_CurrentLabelModel->GetValue();
  if(m_LabelTable->IsColorLabelValid(sel))
    {
    value = m_LabelTable->GetColorLabel(sel).GetLabel();
    return true;
    }
  return false;
}

void LabelEditorModel::SetCurrentLabelDescription(std::string value)
{
  LabelType sel = m_CurrentLabelModel->GetValue();
  if(m_LabelTable->IsColorLabelValid(sel))
    {
    ColorLabel lab = m_LabelTable->GetColorLabel(sel);
    lab.SetLabel(value.c_str());
    m_LabelTable->SetColorLabel(sel, lab);
    }
}

bool LabelEditorModel::GetCurrentLabelIdValueAndRange(
    int &value, NumericValueRange<int> *domain)
{
  // Get the numeric ID of the current label
  LabelType sel = m_CurrentLabelModel->GetValue();
  if(m_LabelTable->IsColorLabelValid(sel))
    {
    value = sel;
    if(domain)
      domain->Set(1, MAX_COLOR_LABELS, 1);
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
  LabelType sel = m_CurrentLabelModel->GetValue();
  if(m_LabelTable->IsColorLabelValid(sel))
    {
    value = m_LabelTable->GetColorLabel(sel).GetAlpha();
    if(domain)
      domain->Set(0, 255, 1);
    return true;
    }
  return false;
}

void LabelEditorModel::SetCurrentLabelOpacity(int value)
{
  LabelType sel = m_CurrentLabelModel->GetValue();
  if(m_LabelTable->IsColorLabelValid(sel))
    {
    ColorLabel lab = m_LabelTable->GetColorLabel(sel);
    lab.SetAlpha((unsigned char)(value));
    m_LabelTable->SetColorLabel(sel, lab);
    }
}


