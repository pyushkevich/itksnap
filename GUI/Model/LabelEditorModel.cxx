#include "LabelEditorModel.h"
#include "IRISApplication.h"

LabelEditorModel::LabelEditorModel()
{
  // Initialize the models
  m_CurrentLabelModel = ConcreteColorLabelPropertyModel::New();
}

void LabelEditorModel::SetParentModel(GlobalUIModel *parent)
{
  // Set the parent model
  m_Parent = parent;

  // Stick the color label information into the domain object
  m_CurrentLabelModel->Initialize(parent->GetDriver()->GetColorLabelTable());

}
