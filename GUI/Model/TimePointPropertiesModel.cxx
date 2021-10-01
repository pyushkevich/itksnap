#include "TimePointPropertiesModel.h"
#include "IRISApplication.h"

TimePointProperty::
TimePointProperty()
{
  m_NicknameModel = NewSimpleConcreteProperty<std::string>("");
}

TimePointPropertiesModel::
TimePointPropertiesModel()
{
}

TimePointPropertiesModel::
~TimePointPropertiesModel()
{

}


void
TimePointPropertiesModel::
SetParentModel(GlobalUIModel *parent)
{
  this->m_ParentModel = parent;
}

void
TimePointPropertiesModel::
Clear()
{
  this->m_TimePointProperties.clear();
  this->m_TimePointOnLoad = 1u;
}

void
TimePointPropertiesModel::
CreateNewProperties()
{
  // Initialize properties when loading new image
  unsigned int nt = this->m_ParentModel->GetDriver()->GetNumberOfTimePoints();

  for (unsigned int i = 1u; i <= nt; ++i)
    {
      m_TimePointProperties[i] = TimePointProperty::New();
    }
}

void
TimePointPropertiesModel::
SetTimePointOnLoad(unsigned int timepoint)
{
  // Invalid timepoint
  if (timepoint == 0 || timepoint > m_ParentModel->GetDriver()->GetNumberOfTimePoints())
    return;

  this->m_TimePointOnLoad = timepoint;
}

void
TimePointPropertiesModel::
OnUpdate()
{
}
