#include "TimePointProperties.h"
#include "Registry.h"
#include "GenericImageData.h"
#include "IRISApplication.h"

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
}

void
TimePointProperties::Reset()
{
  cout << "[TPData] Reset" << endl;

  this->m_TPPropertiesMap.clear();
}


void
TimePointProperties::CreateNewData()
{
  cout << "[TPData] Create New Data" << endl;

  // Main Image has to be loaded
  assert(m_Parent->GetParent()->IsMainImageLoaded());

  // Clear the map
  this->m_TPPropertiesMap.clear();

  unsigned int nt = m_Parent->GetParent()->GetNumberOfTimePoints();
  for (unsigned int i = 1; i <= nt; ++i)
    {
      m_TPPropertiesMap[i] = TimePointProperty();
    }
}

TimePointProperty*
TimePointProperties::GetProperty(unsigned int tp)
{
  // Map size should always consistent with current main image timepoint #
  assert(tp <= m_TPPropertiesMap.size());

  return &m_TPPropertiesMap[tp];
}



void
TimePointProperties::Load(Registry &folder)
{
  cout << "[TPData] Load from Reg" << endl;
  // Validate number of timepoints

  // Validate version

  // Load data

}

void
TimePointProperties::Save(Registry &folder) const
{
  cout << "[TPData] Save to Reg" << endl;

  // Record format version
  //  Future change of format use this to be backward compatible
  folder["FormatVersion"] << "1";

  // Save array size for loading
  folder["TimePoints.ArraySize"] << m_TPPropertiesMap.size();

  for (auto cit = m_TPPropertiesMap.cbegin();
       cit != m_TPPropertiesMap.cend(); ++cit)
    {
      // Create a folder for each timepoint
      Registry &tp_folder = folder.Folder(folder.Key("TimePoints.Element[%d]", cit->first));

      // Write timepoint properties to the folder
      tp_folder["TimePoint"] << cit->first;
      tp_folder["Nickname"] << cit->second.Nickname;
      tp_folder["Tags"].PutList(cit->second.Tags);
    }
}




