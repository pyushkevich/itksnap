// issue #24: Add label smoothing feature

#include "SmoothLabelsModel.h"
#include "GlobalUIModel.h"
#include "IRISApplication.h"

SmoothLabelsModel::SmoothLabelsModel()
{
  // Create a new instance of the model
  m_CurrentLabelModel = ConcreteColorLabelPropertyModel::New();

  // When the value in the model changes, we need to rebroadcast this
  // as a change in the model, so the GUI can update itself
  Rebroadcast(m_CurrentLabelModel, ValueChangedEvent(), ModelUpdateEvent());

  // The model update events should also be rebroadcast as state changes
  Rebroadcast(this, ModelUpdateEvent(), StateMachineChangeEvent());
}

void SmoothLabelsModel::SetParentModel(GlobalUIModel *parent)
{
  m_Parent = parent;
  m_LabelTable = parent->GetDriver()->GetColorLabelTable();

  // Stick the color label information into the domain object
  m_CurrentLabelModel->Initialize(m_LabelTable);
  m_CurrentLabelModel->SetValue(parent->GetDriver()->GetGlobalState()->GetDrawingColorLabel());

  // Listen to events
  Rebroadcast(m_LabelTable,SegmentationChangeEvent(), ModelUpdateEvent());

}

void SmoothLabelsModel::UpdateOnShow()
{

}

void SmoothLabelsModel::Smooth()
{

}
