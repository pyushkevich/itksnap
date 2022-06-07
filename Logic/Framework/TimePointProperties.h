#ifndef TIMEPOINTPROPERTIES_H
#define TIMEPOINTPROPERTIES_H

#include "SNAPCommon.h"
#include "itkDataObject.h"
#include "itkObjectFactory.h"
#include "TagList.h"


class Registry;
class GenericImageData;

struct TimePointProperty
{
  std::string Nickname;
  TagList Tags;
};

class TimePointProperties : public itk::DataObject
{
public:
  irisITKObjectMacro(TimePointProperties, itk::DataObject)

  /** Get a reference to IRISApplication */
  void SetParent(GenericImageData *parent);

  /** Load data from a registry */
  void Load(Registry &folder);

  /** Save data to a registry */
  void Save(Registry &folder) const;

  /** Unload the object, when unloading the main image */
  void Reset();

  /** Create an empty new map */
  void CreateNewData();

  /** Get property by timepoint */
  TimePointProperty* GetProperty(unsigned int tp);

protected:
  TimePointProperties();
  virtual ~TimePointProperties();

private:
  std::map<unsigned int, TimePointProperty> m_TPPropertiesMap;
  GenericImageData *m_Parent;
};

#endif // TIMEPOINTPROPERTIES_H
