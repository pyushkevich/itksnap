// issue #24: Add label smoothing feature

#include "SmoothLabelsModel.h"
#include "GlobalUIModel.h"
#include "IRISApplication.h"

SmoothLabelsModel::SmoothLabelsModel()
{

}

void SmoothLabelsModel::SetParentModel(GlobalUIModel *parent)
{
  this->m_Parent = parent;
}

void SmoothLabelsModel::UpdateOnShow()
{

}

void SmoothLabelsModel::Smooth()
{

}
