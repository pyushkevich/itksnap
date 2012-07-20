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

  m_CurrentLabelHiddenStateModel = makeChildPropertyModel(
        this,
        &Self::GetCurrentLabelHiddenState,
        &Self::SetCurrentLabelHiddenState);

  m_CurrentLabelColorModel = makeChildPropertyModel(
        this,
        &Self::GetCurrentLabelColor,
        &Self::SetCurrentLabelColor);

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


