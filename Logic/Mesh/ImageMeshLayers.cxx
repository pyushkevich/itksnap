#include "ImageMeshLayers.h"
#include "Rebroadcaster.h"
#include "SNAPEvents.h"
#include "SNAPImageData.h"
#include "IRISImageData.h"
#include "IRISApplication.h"
#include "SegmentationMeshWrapper.h"
#include "LevelSetMeshWrapper.h"

ImageMeshLayers::ImageMeshLayers()
{

}

void
ImageMeshLayers::Initialize(GenericImageData *imageData)
{
  m_ImageData = imageData;
  m_IsSNAP = (dynamic_cast<SNAPImageData*>(imageData) != NULL);
}

void
ImageMeshLayers::AddLayer(MeshWrapperBase *meshLayer, bool notifyInspector)
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
  if (notifyInspector)
    InvokeEvent(LayerChangeEvent());
}

void
ImageMeshLayers::SetActiveLayerId(unsigned long id)
{
  m_ActiveLayerId = id;
  InvokeEvent(ActiveLayerChangeEvent());
}

SmartPtr<MeshWrapperBase>
ImageMeshLayers::GetLayer(unsigned long id)
{
  MeshWrapperBase *ret = nullptr;

  if (m_Layers.count(id))
    ret = m_Layers[id];

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
ImageMeshLayers
::UpdateActiveMeshLayer(itk::Command *progressCmd)
{
  assert(progressCmd);
  std::cout << "[ImageMeshLayers] UpdateActiveMeshLayer " << std::endl;

  auto app = m_ImageData->GetParent();

  if (m_IsSNAP)
    {
    std::cout << "-- SNAP Mode!" << std::endl;
    auto snap = dynamic_cast<SNAPImageData*>(m_ImageData.GetPointer());

    // if failed, check constructor why m_isSNAP is true
    assert(snap);

    auto lsImg = snap->GetSnake();

    if (m_ImageToMeshMap.count(lsImg->GetUniqueId()))
      {
      // If the layer already exist, update the layer
      auto lsMesh = static_cast<LevelSetMeshWrapper*>
          (m_ImageToMeshMap[lsImg->GetUniqueId()]);

      lsMesh->UpdateMeshes(lsImg, app->GetCursorTimePoint(),
                           app->GetSNAPImageData()->GetLevelSetPipelineMutex());
      }
    else
      {
      auto lsMesh = AddLevelSetMeshLayer(lsImg);
      lsMesh->UpdateMeshes(lsImg, app->GetCursorTimePoint(),
            app->GetSNAPImageData()->GetLevelSetPipelineMutex());
      }
    }
  else
    {
    // Get the active segmentation image layer id
    auto segImg = app->GetSelectedSegmentationLayer();

    if (m_ImageToMeshMap.count(segImg->GetUniqueId()))
      {
      // If the layer already exist, update the layer
      auto segMesh = static_cast<SegmentationMeshWrapper*>
          (m_ImageToMeshMap[segImg->GetUniqueId()]);

      segMesh->UpdateMeshes(progressCmd, app->GetCursorTimePoint());
      }
    else
      {
      // If the layer doesn't exist yet, add a segmentation layer
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
  m_ImageToMeshMap[segImg->GetUniqueId()] = segMesh;

  return segMesh;
}

LevelSetMeshWrapper *
ImageMeshLayers
::AddLevelSetMeshLayer(LevelSetImageWrapper *lsImg)
{
  assert(lsImg);

  auto app = m_ImageData->GetParent();

  // Create a new levelset mesh wrapper
  auto lsMesh = LevelSetMeshWrapper::New();
  lsMesh->Initialize(app->GetGlobalState()->GetMeshOptions());

  this->AddLayer(lsMesh, false);

  return lsMesh.GetPointer();
}

bool
ImageMeshLayers::IsActiveMeshLayerDirty()
{
  auto app = m_ImageData->GetParent();
  auto tp = app->GetCursorTimePoint();

  if (m_IsSNAP)
    {
    // Dirty check for LevelSet Image
    auto snap = static_cast<SNAPImageData*>(m_ImageData.GetPointer());

    assert(snap);

    auto lsId = snap->GetSnake()->GetUniqueId();

    if (HasMeshForImage(lsId))
      return m_ImageToMeshMap[lsId]->IsMeshDirty(tp);
    else
      {
      // If mesh layer doesn't exist: return true because we want to
      // create a new mesh layer
      return true;
      }
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
    if (HasMeshForImage(segId))
      {
      // If mesh layer exist: return mesh_layer->IsMeshDirty();
      return m_ImageToMeshMap[segId]->IsMeshDirty(tp);
      }
    else
      {
      // If mesh layer doesn't exist: return true because we want to
      // create a new mesh layer
      return true;
      }
    }

  return false;
}

bool
ImageMeshLayers::HasMeshForImage(unsigned long image_id) const
{
  return m_ImageToMeshMap.count(image_id);
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
