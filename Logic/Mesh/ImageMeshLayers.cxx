#include "ImageMeshLayers.h"

ImageMeshLayers::ImageMeshLayers()
{

}

void
ImageMeshLayers::AddLayer(MeshWrapperBase *meshLayer)
{
  MeshLayerIdType id = meshLayer->GetId();
  m_Layers[id] = meshLayer;
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
