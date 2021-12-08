#include "ImageMeshLayers.h"

ImageMeshLayers::ImageMeshLayers()
{

}

void
ImageMeshLayers::AddLayer(SmartPtr<MeshWrapperBase> meshLayer)
{
  MeshLayerIdType id = meshLayer->GetUniqueId();
  m_Layers[id] = meshLayer;
}

SmartPtr<MeshWrapperBase>
ImageMeshLayers::GetLayer(MeshLayerIdType id)
{
  return m_Layers[id];
}

void
ImageMeshLayers::RemoveLayer(MeshLayerIdType id)
{
  m_Layers.erase(id);
}

std::vector<ImageMeshLayers::MeshLayerIdType>
ImageMeshLayers::GetLayerIds() const
{
  std::vector<ImageMeshLayers::MeshLayerIdType> ret;

  for (auto cit = m_Layers.cbegin(); cit != m_Layers.cend(); ++cit)
    ret.push_back(cit->first);

  return ret;
}

void
ImageMeshLayers::Unload()
{
  for (auto it = m_Layers.begin(); it != m_Layers.end(); ++it)
    it->second->Delete();

  m_Layers.clear();
}

//---------------------------------------
//  MeshLayerIterator Implementation
//---------------------------------------

MeshLayerIterator::MeshLayerIterator(ImageMeshLayers *data)
{
  this->m_Layers = &data->m_Layers;
  m_It = m_Layers->begin();
}

bool
MeshLayerIterator::IsAtEnd() const
{
  return m_It == m_Layers->end();
}

MeshLayerIterator &
MeshLayerIterator::MoveToBegin()
{
  m_It = m_Layers->begin();

  return *this;
}

MeshLayerIterator &
MeshLayerIterator::operator++()
{
  // if reach the end, stop at the end
  if (m_It != m_Layers->end())
    ++m_It;

  return *this;
}

MeshWrapperBase::MeshLayerIdType
MeshLayerIterator::GetUniqueId() const
{
  return m_It->second->GetUniqueId();
}

MeshWrapperBase *
MeshLayerIterator::GetLayer() const
{
  return m_It->second;
}
