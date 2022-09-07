#include "TimePointProperties.h"
#include "Registry.h"
#include "GenericImageData.h"
#include "IRISApplication.h"
#include "Rebroadcaster.h"

TimePointProperties::TimePointProperties()
{
}

TimePointProperties::~TimePointProperties()
{
}

void
TimePointProperties::SetParent(GenericImageData * parent)
{
  m_Parent = parent;
	Rebroadcaster::Rebroadcast(this, WrapperGlobalMetadataChangeEvent(),
														 parent, WrapperGlobalMetadataChangeEvent());
}

void
TimePointProperties::Reset()
{
  this->m_TPPropertiesMap.clear();
}


void
TimePointProperties::CreateNewData()
{
  // Main Image has to be loaded
  assert(m_Parent->GetParent()->IsMainImageLoaded());

  // Clear the map
  this->m_TPPropertiesMap.clear();

  unsigned int nt = m_Parent->GetParent()->GetNumberOfTimePoints();
  for (unsigned int i = 1; i <= nt; ++i)
    {
		auto tpp = TimePointProperty::New();
		m_TPPropertiesMap[i] = tpp;
		Rebroadcaster::Rebroadcast(tpp, WrapperGlobalMetadataChangeEvent(),
															 this, WrapperGlobalMetadataChangeEvent());
    }
}

TimePointProperty*
TimePointProperties::GetProperty(unsigned int tp)
{
  // Map size should always consistent with current main image timepoint #
  assert(tp <= m_TPPropertiesMap.size());

	return m_TPPropertiesMap[tp];
}



void
TimePointProperties::Load(Registry &folder)
{
  // Validate version
  string version = folder["FormatVersion"][""];

  // Validate number of timepoints
  unsigned int nt = folder["TimePoints.ArraySize"][0u];

  assert(nt == m_Parent->GetParent()->GetNumberOfTimePoints());

  // Load data (1-based index timepoint array)
  for (unsigned int i = 1u; i <= nt; ++i)
    {
      // Check folder existence
      // To make this robust, create an empty property for a missing entry
      std::string key = Registry::Key("TimePoints.TimePoint[%d]", i);
			auto tpp = TimePointProperty::New();

      if (folder.HasFolder(key))
        {
          Registry &tpFolder = folder.Folder(key);

					tpp->SetNickname(tpFolder["Nickname"][""]);
					tpFolder["Tags"].GetList(tpp->GetModifiableTags());
        }

      this->m_TPPropertiesMap[i] = tpp;
    }

}

void
TimePointProperties::Save(Registry &folder) const
{
  // Record format version
  //  Future change of format use this to be backward compatible
  folder["FormatVersion"] << "1";

  // Save array size for loading
  folder["TimePoints.ArraySize"] << m_TPPropertiesMap.size();

  for (auto cit = m_TPPropertiesMap.cbegin();
       cit != m_TPPropertiesMap.cend(); ++cit)
    {
      // Create a folder for each timepoint
      Registry &tp_folder = folder.Folder(folder.Key("TimePoints.TimePoint[%d]", cit->first));

      // Write timepoint properties to the folder
      tp_folder["TimePoint"] << cit->first;
			tp_folder["Nickname"] << cit->second->GetNickname();
			tp_folder["Tags"].PutList(cit->second->GetTags());
    }
}




