#include "RandomForestClassifier.h"
#include "Library/classification.h"

void RandomForestClassifier::Reset()
{
  if(m_Forest)
    delete m_Forest;

  m_Forest = new RandomForestType(true);
  m_ClassToLabelMapping.clear();
  m_ForegroundClass = 0;
}

LabelType RandomForestClassifier::GetForegroundClassLabel() const
{
  MappingType::const_iterator it = m_ClassToLabelMapping.find(m_ForegroundClass);
  if(it != m_ClassToLabelMapping.end())
    return it->second;

  // Default behavior - clear label
  return 0;
}

void RandomForestClassifier::SetForegroundClassLabel(LabelType label)
{
  MappingType::const_iterator it = m_ClassToLabelMapping.begin();
  for(; it != m_ClassToLabelMapping.end(); ++it)
    {
    if(it->second == label)
      {
      m_ForegroundClass = it->first;
      return;
      }
    }
}

bool RandomForestClassifier::IsValidClassifier() const
{
  return m_ClassToLabelMapping.size() >= 2;
}

RandomForestClassifier::RandomForestClassifier()
{
  m_Forest = new RandomForestType(true);
}

RandomForestClassifier::~RandomForestClassifier()
{
  if(m_Forest)
    delete m_Forest;
}
