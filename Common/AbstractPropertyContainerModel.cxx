#include "AbstractPropertyContainerModel.h"

void
AbstractPropertyContainerModel::DeepCopy(
    const AbstractPropertyContainerModel *source)
{
  // This method will only work if both models have the same fields in the
  // same order. The assertions below check that
  assert(m_Properties.size() == source->m_Properties.size());

  PropertyMapCIter itSrc = source->m_Properties.begin();
  PropertyMapIter it = m_Properties.begin();
  while(itSrc != source->m_Properties.end())
    {
    assert(it->first == itSrc->first);
    ConcretePropertyHolderBase *ptr_src = itSrc->second;
    ConcretePropertyHolderBase *ptr_trg = it->second;
    ptr_trg->DeepCopy(ptr_src);
    ++it; ++itSrc;
    }
}

bool AbstractPropertyContainerModel::operator == (
    const AbstractPropertyContainerModel &source)
{
  // This method will only work if both models have the same fields in the
  // same order. The assertions below check that
  assert(m_Properties.size() == source.m_Properties.size());

  PropertyMapCIter itSrc = source.m_Properties.begin();
  PropertyMapIter it = m_Properties.begin();
  while(itSrc != source.m_Properties.end())
    {
    assert(it->first == itSrc->first);
    ConcretePropertyHolderBase *ptr_src = itSrc->second;
    ConcretePropertyHolderBase *ptr_trg = it->second;
    if(!ptr_trg->Equals(ptr_src))
      return false;
    ++it; ++itSrc;
    }

  return true;
}

bool AbstractPropertyContainerModel::operator != (
    const AbstractPropertyContainerModel &source)
{
  return !(*this == source);
}

void AbstractPropertyContainerModel::WriteToRegistry(Registry &folder) const
{
  for(PropertyMapCIter it = m_Properties.begin(); it != m_Properties.end(); it++)
    {
    it->second->Serialize(folder);
    }
}

void AbstractPropertyContainerModel::ReadFromRegistry(Registry &folder)
{
  for(PropertyMapIter it = m_Properties.begin(); it != m_Properties.end(); it++)
    {
    it->second->Deserialize(folder);
    }

  // Flag this object as modified
  this->Modified();
}

