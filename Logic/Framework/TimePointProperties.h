#ifndef TIMEPOINTPROPERTIES_H
#define TIMEPOINTPROPERTIES_H

#include "SNAPCommon.h"
#include "SNAPEvents.h"
#include "itkDataObject.h"
#include "itkObjectFactory.h"
#include "TagList.h"


class Registry;
class GenericImageData;

class TimePointProperty : public itk::DataObject
{
public:
	irisITKObjectMacro(TimePointProperty, itk::DataObject)

	std::string GetNickname() const
	{ return Nickname; }

	void SetNickname(std::string _nickname)
	{
		Nickname = _nickname;
		InvokeEvent(WrapperGlobalMetadataChangeEvent());
	}

	TagList GetTags() const
	{ return Tags; }

	TagList &GetModifiableTags()
	{ return Tags; }

	void AddTag(std::string &tag)
	{
		Tags.AddTag(tag);
	}

	void SetTagList(TagList tlist)
	{
		Tags = tlist;
	}

protected:
	TimePointProperty() {};
	virtual ~TimePointProperty() {}

private:
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
	std::map<unsigned int, SmartPtr<TimePointProperty>> m_TPPropertiesMap;
  GenericImageData *m_Parent;
};

#endif // TIMEPOINTPROPERTIES_H
