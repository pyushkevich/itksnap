// issue #24: Add label smoothing feature

#include "SmoothLabelsModel.h"
#include "GlobalUIModel.h"
#include "IRISApplication.h"

SmoothLabelsModel::SmoothLabelsModel()
{
  // Create a new instance of the model
  m_CurrentLabelModel = ConcreteColorLabelPropertyModel::New();
}

void SmoothLabelsModel::SetParentModel(GlobalUIModel *parent)
{
  m_Parent = parent;
  m_LabelTable = parent->GetDriver()->GetColorLabelTable();

  // Stick the color label information into the domain object
  m_CurrentLabelModel->Initialize(m_LabelTable);
  m_CurrentLabelModel->SetValue(parent->GetDriver()->GetGlobalState()->GetDrawingColorLabel());

  // When label table changed somewhere else, update current model as well
  Rebroadcast(m_LabelTable,SegmentationChangeEvent(), ModelUpdateEvent());
}

void SmoothLabelsModel::UpdateOnShow()
{

}

void SmoothLabelsModel::Smooth(std::vector<LabelType> &labelsToSmooth)
{
  std::cout << "Labels to Smooth: " << endl;
  for(auto cit = labelsToSmooth.cbegin(); cit != labelsToSmooth.cend(); ++cit)
    {
      std::cout << *cit << endl;
    }
}
