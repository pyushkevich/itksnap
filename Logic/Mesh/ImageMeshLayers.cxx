#include "ImageMeshLayers.h"

ImageMeshLayers::ImageMeshLayers()
{

}

void
ImageMeshLayers::AddLayer(SmartPtr<MeshWrapperBase> meshLayer)
{
  MeshLayerIdType id = meshLayer->GetId();
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
