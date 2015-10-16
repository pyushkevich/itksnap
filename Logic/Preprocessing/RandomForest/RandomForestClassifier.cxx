#include "RandomForestClassifier.h"
#include "Library/classification.h"

void RandomForestClassifier::Reset()
{
  if(m_Forest)
    delete m_Forest;

  m_Forest = new RandomForestType(true);
  m_ClassToLabelMapping.clear();
  m_BiasParameter = 0.5;
  m_PatchRadius.Fill(0);
  m_UseCoordinateFeatures = false;
  m_ClassWeights.clear();
}

void RandomForestClassifier::SetClassWeight(size_t class_id, double weight)
{
  m_ClassWeights[class_id] = weight;
}

bool RandomForestClassifier::IsValidClassifier() const
{
  return m_ClassToLabelMapping.size() >= 2 && m_Forest->GetForestSize() > 0;
}

RandomForestClassifier::RandomForestClassifier()
{
  m_Forest = NULL;
  this->Reset();
}

RandomForestClassifier::~RandomForestClassifier()
{
  if(m_Forest)
    delete m_Forest;
}
