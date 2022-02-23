#include "ImageMeshLayers.h"
#include "Rebroadcaster.h"
#include "SNAPEvents.h"

ImageMeshLayers::ImageMeshLayers()
{

}

void
ImageMeshLayers::AddLayer(SmartPtr<MeshWrapperBase> meshLayer)
{
  unsigned long id = meshLayer->GetUniqueId();
  m_Layers[id] = meshLayer;

  // if this is the first added layer, set active
  m_ActiveLayerId = meshLayer->GetUniqueId();

  Rebroadcaster::Rebroadcast(meshLayer, itk::ModifiedEvent(),
                             this, ValueChangedEvent());
  Rebroadcaster::Rebroadcast(meshLayer, ValueChangedEvent(),
                             this, ActiveLayerChangeEvent());
  Rebroadcaster::Rebroadcast(meshLayer, WrapperDisplayMappingChangeEvent(),
                             this, WrapperDisplayMappingChangeEvent());

  // Invoke to trigger initial rendering of the mesh
  InvokeEvent(ActiveLayerChangeEvent());
  // Invoke to trigger rebuild of the layer inspector row
  InvokeEvent(LayerChangeEvent());
}

void
ImageMeshLayers::SetActiveLayerId(unsigned long id)
{
  std::cout << "[ImageMeshLayers] SetActiveLayerId=" << id << std::endl;
  m_ActiveLayerId = id;
  InvokeEvent(ActiveLayerChangeEvent());
}

SmartPtr<MeshWrapperBase>
ImageMeshLayers::GetLayer(unsigned long id)
{
  return m_Layers[id];
}

MeshLayerIterator
ImageMeshLayers::GetLayers()
{
  return MeshLayerIterator(this);
}

void
ImageMeshLayers::RemoveLayer(unsigned long id)
{
  m_Layers.erase(id);

  // Notify subscribers about layer changes
  InvokeEvent(LayerChangeEvent());
}

std::vector<unsigned long>
ImageMeshLayers::GetLayerIds() const
{
  std::vector<unsigned long> ret;

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

unsigned long
MeshLayerIterator::GetUniqueId() const
{
  return m_It->second->GetUniqueId();
}

MeshWrapperBase *
MeshLayerIterator::GetLayer() const
{
  return m_It->second;
}
