#include "ImageMeshLayers.h"
#include "Rebroadcaster.h"
#include "SNAPEvents.h"
#include "SNAPImageData.h"
#include "IRISImageData.h"
#include "IRISApplication.h"
#include "SegmentationMeshWrapper.h"

ImageMeshLayers::ImageMeshLayers()
{

}

void
ImageMeshLayers::Initialize(GenericImageData *imageData)
{
  m_ImageData = imageData;

  m_IsSNAP = (dynamic_cast<SNAPImageData*>(m_ImageData.GetPointer()) != nullptr);
}

void
ImageMeshLayers::AddLayer(MeshWrapperBase *meshLayer)
{
  unsigned long id = meshLayer->GetUniqueId();
  m_Layers[id] = meshLayer;

  Rebroadcaster::Rebroadcast(meshLayer, itk::ModifiedEvent(),
                             this, ValueChangedEvent());
  Rebroadcaster::Rebroadcast(meshLayer, ValueChangedEvent(),
                             this, ActiveLayerChangeEvent());
  Rebroadcaster::Rebroadcast(meshLayer, WrapperDisplayMappingChangeEvent(),
                             this, WrapperDisplayMappingChangeEvent());

  // Invoke to trigger initial rendering of the mesh
  // Always make the latest added layer active
  m_ActiveLayerId = meshLayer->GetUniqueId();
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
  MeshWrapperBase *ret = nullptr;

  if (m_Layers.count(id))
    ret = m_Layers[id];
  else
    std::cout << "[ImageMeshLayers] Layer with id=" << id
              << "requested but does not exist" << std::endl;

  return ret;
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

int
ImageMeshLayers::UpdateActiveMeshLayer(itk::Command *progressCmd)
{
  assert(progressCmd);
  std::cout << "[ImageMeshLayers] UpdateActiveMeshLayer " << std::endl;

  auto app = m_ImageData->GetParent();

  if (m_IsSNAP)
    {}
  else
    {
    std::cout << "[ImageMeshLayers] IRIS Seg Mesh Update" << std::endl;

    // Get the active segmentation image layer id
    auto segImg = app->GetSelectedSegmentationLayer();

    if (m_SegmentationImageToMeshMap.count(segImg->GetUniqueId()))
      {
      // If the layer already exist, update the layer
      auto segMesh = static_cast<SegmentationMeshWrapper*>
          (m_SegmentationImageToMeshMap[segImg->GetUniqueId()]);

      std::cout << "[ImageMeshLayers] seg mesh found for imgid="
                << segImg->GetUniqueId() << std::endl;

      segMesh->UpdateMeshes(progressCmd, app->GetCursorTimePoint());
      }
    else
      {
      // If the layer doesn't exist yet, add a segmentation layer
      std::cout << "[ImageMeshLayers] seg mesh not found for imgid="
                << segImg->GetUniqueId() << std::endl;
      std::cout << "[ImageMeshLayers] creating new mesh layer" << std::endl;
      auto meshLayer = AddSegmentationMeshLayer(segImg);
      meshLayer->UpdateMeshes(progressCmd, app->GetCursorTimePoint());
      }
    }

  return 0;
}

SegmentationMeshWrapper*
ImageMeshLayers::
AddSegmentationMeshLayer(LabelImageWrapper* segImg)
{
  assert(segImg);

  auto app = m_ImageData->GetParent();

  // Create a new segmentation mesh wrapper
  auto segMesh = SegmentationMeshWrapper::New();
  segMesh->Initialize(segImg, app->GetGlobalState()->GetMeshOptions());

  // Add to layer map and segmentation mesh map
  AddLayer(segMesh);
  m_SegmentationImageToMeshMap[segImg->GetUniqueId()] = segMesh;

  return segMesh;
}

bool
ImageMeshLayers::IsActiveMeshLayerDirty()
{
  auto app = m_ImageData->GetParent();
  auto tp = app->GetCursorTimePoint();

  if (m_IsSNAP)
    {
    // Dirty check for LevelSet Image

    }
  else
    {
    // Dirty check for IRIS Image

    // Get the active segmentation layer
    auto wrapper = app->GetSelectedSegmentationLayer();

    // An image does not exist cannot be dirty
    if (!wrapper)
      return false;

    // Now check if the corresponding mesh layer exist
    auto segId = wrapper->GetUniqueId();
    if (HasSegmentationMesh(segId))
      {
      // If mesh layer exist: return mesh_layer->IsMeshDirty();
      return m_SegmentationImageToMeshMap[segId]->IsMeshDirty(tp);
      }
    else
      {
      // If mesh layer doesn't exist: return true because we want to
      // create a new mesh layer
      return true;
      }
    }


  auto active_layer = GetLayer(m_ActiveLayerId);

  return false;
}

bool
ImageMeshLayers::HasSegmentationMesh(unsigned long id) const
{
  return m_SegmentationImageToMeshMap.count(id);
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
