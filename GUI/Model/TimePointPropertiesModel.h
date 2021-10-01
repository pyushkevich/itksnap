#ifndef TIMEPOINTPROPERTIESMODEL_H
#define TIMEPOINTPROPERTIESMODEL_H

#include "AbstractPropertyContainerModel.h"
#include "PropertyModel.h"
#include "GlobalUIModel.h"

class TimePointProperty : public AbstractPropertyContainerModel
{
public:
  irisITKObjectMacro(TimePointProperty, AbstractPropertyContainerModel)

  irisSimplePropertyAccessMacro(Nickname, std::string)
protected:
  TimePointProperty();
  virtual ~TimePointProperty() {};

  SmartPtr<ConcreteSimpleStringProperty> m_NicknameModel;
  TagList tags;
};

class TimePointPropertiesModel : public AbstractModel
{
public:
  irisITKObjectMacro(TimePointPropertiesModel, AbstractModel)

  void SetParentModel(GlobalUIModel *parent);

  void OnUpdate() ITK_OVERRIDE;

  // Write current values to registry
  void WriteToRegistry(Registry *reg) const;

  // Read from registry and populate propertie maps
  void ReadFromRegistry(Registry *reg);

  // Reset properties
  void Clear();

  // Create new properties if no project file present
  void CreateNewProperties();

  // Get Property by timepoint
  SmartPtr<TimePointProperty> GetProperty(unsigned int timepoint) const;

  unsigned int GetTimePointOnLoad() const { return this->m_TimePointOnLoad; }
  void SetTimePointOnLoad(unsigned int timepoint);

protected:
  TimePointPropertiesModel();
  virtual ~TimePointPropertiesModel();

  std::map<unsigned int, SmartPtr<TimePointProperty>> m_TimePointProperties;

  // Display this timepoint when loading the image
  unsigned int m_TimePointOnLoad = 1u;

  SmartPtr<GlobalUIModel> m_ParentModel;
};

#endif // TIMEPOINTPROPERTIESMODEL_H
