#include "ImageMeshLayers.h"
#include "Rebroadcaster.h"
#include "SNAPEvents.h"
#include "SNAPImageData.h"
#include "IRISImageData.h"
#include "IRISApplication.h"
#include "StandaloneMeshWrapper.h"
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
  m_ImageToMeshMap.clear();

  for (auto it = m_Layers.begin(); it != m_Layers.end(); ++it)
    it->second->Delete();

  m_Layers.clear();
}

int
ImageMeshLayers
::UpdateActiveMeshLayer(itk::Command *progressCmd)
{
  assert(progressCmd);

  auto app = m_ImageData->GetParent();

  if (m_IsSNAP)
    {
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
                           app->GetGlobalState()->GetDrawingColorLabel(),
                           app->GetSNAPImageData()->GetLevelSetPipelineMutex());
      }
    else
      {
      auto lsMesh = AddLevelSetMeshLayer(lsImg);
      lsMesh->UpdateMeshes(lsImg, app->GetCursorTimePoint(),
                           app->GetGlobalState()->GetDrawingColorLabel(),
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

void
ImageMeshLayers
::AddLayerFromFiles(std::vector<std::string> &fn_list, FileFormat format,
                    bool startFromFirstTP)
{
  GuidedMeshIO IO;

  // Create a mesh wrapper
  auto wrapper = StandaloneMeshWrapper::New();
  SmartPtr<MeshWrapperBase> baseWrapper = wrapper.GetPointer();

  auto app = m_ImageData->GetParent();

  size_t tp = startFromFirstTP ? 0 : app->GetCursorTimePoint();
  size_t nt = nt = app->GetNumberOfTimePoints();

  // We pick the first filename as placeholder of the wrapper filename
  wrapper->SetFileName(*fn_list.begin());

  // Load one file per time point until final time point is reached
  for (auto &fn : fn_list)
    {
    if (tp >= nt)
      break;

    // Execute loading
    IO.LoadMesh(fn.c_str(), format, baseWrapper, tp++, 0u);
    }

  // Install the wrapper to the application
  this->AddLayer(baseWrapper);
}

void
ImageMeshLayers
::LoadFromRegistry(Registry &project, std::string &project_dir_orig,
                          std::string &project_dir_crnt)
{
  if (!project.HasFolder("MeshLayers"))
    return;

  auto folder_layers = project.Folder("MeshLayers");

  // Whether the project file has been moved
  bool moved = (project_dir_orig != project_dir_crnt);

  std::string key;

  for (int i = 0; folder_layers.HasFolder(key = Registry::Key("Layer[%03d]", i)); ++i)
    {
    // Load MeshAssmbly Time Points
    auto folder_layer = folder_layers.Folder(key);

    // Create a new mesh wrapper
    auto mesh_wrapper = StandaloneMeshWrapper::New();

    // Restore nicknames and tags
    mesh_wrapper->SetCustomNickname(folder_layer["NickName"][""]);

    TagList tags;
    folder_layer["Tags"].GetList(tags);

    // Restore mesh timepoint assembly
    auto folder_assembly = folder_layer.Folder("MeshTimePoints");

    // Create a mesh IO for executing the actual loading from file
    GuidedMeshIO io;

    std::string key_tp;
    bool fnSet = false;

    for (int j = 1;
         folder_assembly.HasFolder(key_tp = Registry::Key("TimePoint[%03d]", j));
         ++j)
      {
      auto folder_tp = folder_assembly.Folder(key_tp);

      std::string key_poly;
      for (int k = 0;
           folder_tp.HasFolder(key_poly = Registry::Key("PolyData[%03d]", k));
           ++k)
        {
        auto folder_poly = folder_tp.Folder(key_poly);

        std::string poly_file_full = folder_poly["AbsolutePath"][""];

        if (moved)
          poly_file_full = IRISApplication::GetMovedFilePath(
                project_dir_orig, project_dir_crnt, poly_file_full);

        // Set first filename as placeholder
        if (!fnSet)
          {
          mesh_wrapper->SetFileName(poly_file_full);
          fnSet = true;
          }

        FileFormat format = folder_poly["Format"]
            .GetEnum(GuidedMeshIO::GetEnumFileFormat(), FileFormat::FORMAT_COUNT);

        // Load with tp = j-1. The storeing of time point index should always be zero-based
        io.LoadMesh(poly_file_full.c_str(), format, mesh_wrapper.GetPointer(), j-1, k);
        }
      }

    // Load Data Array Properties
    // -- Lookup array properties by name and type
    // -- Restore color map and intensity curve
    // -- Only load matched entries since mesh data array might be changed externally
    auto folder_data = folder_layer.Folder("DataArrayProperties");

    std::string key_data;
    for (int array_id = 0;
         folder_data.HasFolder(key_data = Registry::Key("DataArray[%03d]", array_id));
         ++array_id)
      {
        auto folder_crnt = folder_data.Folder(key_data);

        // Get Array Name and Array Type
        auto array_name = folder_crnt["ArrayName"][""];
        auto array_type = folder_crnt["ArrayType"].
            GetEnum(AbstractMeshDataArrayProperty::GetMeshDataTypeEnumMap(),
                    AbstractMeshDataArrayProperty::POINT_DATA);

        // Search the data array by array name and array type
        // If found, restore color map and intensity curve
        auto &array_map = mesh_wrapper->GetCombinedDataProperty();

        for (auto kv : array_map)
          {
          if (strcmp(kv.second->GetName(), array_name) == 0 &&
              kv.second->GetType() == array_type)
            {
            if (folder_crnt.HasFolder("ColorMap"))
              {
              auto folder_cm = folder_crnt.Folder("ColorMap");
              kv.second->GetColorMap()->LoadFromRegistry(folder_cm);
              }

            if (folder_crnt.HasFolder("IntensityCurve"))
              {
              auto folder_ic = folder_crnt.Folder("IntensityCurve");
              kv.second->GetIntensityCurve()->LoadFromRegistry(folder_ic);
              }
            }
          }

        // We need to call set active to refresh the display mapping policy
        mesh_wrapper->SetActiveMeshLayerDataPropertyId(0);
      }

    AddLayer(mesh_wrapper, true);
    }
}

void
ImageMeshLayers
::SaveToRegistry(Registry &folder)
{
  MeshLayerIterator mesh_it(this);

  for (int mesh_cnt = 0; !mesh_it.IsAtEnd(); ++mesh_it)
    {
    auto mesh_layer = mesh_it.GetLayer();

    // We current only save standalone meshes
    auto standalone_mesh = dynamic_cast<StandaloneMeshWrapper*>(mesh_layer);
    if (standalone_mesh)
      {
      Registry &folder_crnt = folder.Folder(Registry::Key("MeshLayers.Layer[%03d]", mesh_cnt++));
      mesh_layer->SaveToRegistry(folder_crnt);
      }
    }
}

MeshWrapperBase *
ImageMeshLayers
::GetMeshForImage(unsigned long image_id)
{
  MeshWrapperBase *ret = nullptr;
  if (HasMeshForImage(image_id))
    {
    ret = m_ImageToMeshMap[image_id];
    }
  return ret;
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
  lsMesh->Initialize(app->GetGlobalState()->GetMeshOptions(),
                     app->GetColorLabelTable());

  this->AddLayer(lsMesh, false);
  m_ImageToMeshMap[lsImg->GetUniqueId()] = lsMesh;

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

    // If snake has not loaded yet, mesh is not dirty
    // -- Adding this to avoid assertion failure on GetSnake()
    if (!snap->IsSnakeLoaded())
      return false;

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

void
ImageMeshLayers
::LoadFileToLayer(const char *filename, FileFormat format,
                unsigned long layer_id, unsigned int timepoint, LabelType mesh_id)
{
  // Create a new IO for loading
  GuidedMeshIO IO;

  // Get Mesh Layer
  auto layer = GetLayer(layer_id);

  // if failed, check the layer_id passed from the caller
  assert(layer);

  // Execute loading
  IO.LoadMesh(filename, format, layer, timepoint, mesh_id);
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
