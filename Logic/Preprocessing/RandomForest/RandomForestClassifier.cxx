#include "RandomForestClassifier.h"
#include "Library/classification.h"

void RandomForestClassifier::Reset()
{
  if(m_Forest)
    delete m_Forest;

  m_Forest = new RandomForestType(true);
  m_Mapping.clear();

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
