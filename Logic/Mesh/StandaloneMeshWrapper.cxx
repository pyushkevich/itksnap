#include "StandaloneMeshWrapper.h"
#include "DisplayMappingPolicy.h"
#include "Rebroadcaster.h"
#include "SNAPRegistryIO.h"
#include "itksys/SystemTools.hxx"

StandaloneMeshWrapper::StandaloneMeshWrapper()
{
  m_DisplayMapping = GenericMeshDisplayMappingPolicy::New();
  m_DisplayMapping->SetMesh(this);
  m_Initialized = true;
}

void
StandaloneMeshWrapper
::SetMesh(vtkPolyData *mesh, unsigned int timepoint, LabelType id)
{
  auto wrapper = PolyDataWrapper::New();
  wrapper->SetPolyData(mesh);

  auto pointDataProps = wrapper->GetPointDataProperties();
  auto cellDataProps = wrapper->GetCellDataProperties();

  // Add or merge data properties
  MergeDataProperties(m_PointDataProperties, pointDataProps);
  MergeDataProperties(m_CellDataProperties, cellDataProps);

  // Add wrapper to mesh assembly
  if (m_MeshAssemblyMap.count(timepoint))
    m_MeshAssemblyMap[timepoint]->AddMesh(wrapper, id);
  else
    {
      auto assembly = StandaloneMeshAssembly::New();
      assembly->AddMesh(wrapper, id);
      m_MeshAssemblyMap[timepoint] = assembly.GetPointer();
    }

  int propId = (m_CombinedDataPropertyMap.size() > 0) ?
        m_CombinedDataPropertyMap.cbegin()->first : -1;

  // Set Default active array id
  SetActiveMeshLayerDataPropertyId(propId);

  InvokeEvent(ValueChangedEvent());
}

bool
StandaloneMeshWrapper::IsMeshDirty(unsigned int)
{
  return false;
}

void
StandaloneMeshWrapper
::SaveToRegistry(Registry &folder)
{
  // What type is this mesh
  folder["MeshType"] << "StandaloneMesh";

  // Absolute path to the file
  auto full_path = itksys::SystemTools::CollapseFullPath(this->GetFileName());
  Registry &path = folder.Folder("MeshTimePoints");
  for (auto kv : m_MeshAssemblyMap)
    {
    Registry &tp = path.Folder(Registry::Key("TimePoint[%03d]", kv.first + 1));
    tp["TimePoint"] << kv.first + 1;
    kv.second->SaveToRegistry(tp);
    }

  // Tags
  folder["Tags"].PutList(this->GetTags());

  // Nick name
  folder["NickName"] << this->GetCustomNickname();

  // Save Display Mapping Associated with Data Arrays
  // -- id/(Array Name + Array Type) => Display Mapping
  Registry &propReg = folder.Folder("DataArrayProperties");

  for (auto kv : m_CombinedDataPropertyMap)
    {
    Registry &crnt = propReg.Folder(Registry::Key("DataArray[%03d]", kv.first));
    crnt["ArrayName"] << kv.second->GetName();
    crnt["ArrayType"].PutEnum(
          AbstractMeshDataArrayProperty::GetMeshDataTypeEnumMap(),
          kv.second->GetType());
    kv.second->GetColorMap()->SaveToRegistry(crnt.Folder("ColorMap"));
    kv.second->GetActiveIntensityCurve()->SaveToRegistry(crnt.Folder("IntensityCurve"));
    }
}

void
StandaloneMeshWrapper
::LoadFromRegistry(Registry &folder, std::string &orig_dir, std::string &crnt_dir,
                   unsigned int nT)
{
  Superclass::LoadFromRegistry(folder, orig_dir, crnt_dir, nT); // Load basic structures using parent method

  auto folder_data = folder.Folder("DataArrayProperties");
  unsigned int array_id = 0;
  std::string key_data = Registry::Key("DataArray[%03d]", array_id);

  while (folder_data.HasFolder(key_data))
    {
    auto folder_crnt = folder_data.Folder(key_data);
    auto array_name = folder_crnt["ArrayName"][""];
    auto array_type = folder_crnt["ArrayType"].
        GetEnum(AbstractMeshDataArrayProperty::GetMeshDataTypeEnumMap(),
                AbstractMeshDataArrayProperty::POINT_DATA);

    // Search the data array by array name and array type
    // If found, restore color map and intensity curve
    auto &array_map = this->GetCombinedDataProperty();

    for (auto &kv : array_map)
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
          kv.second->GetActiveIntensityCurve()->LoadFromRegistry(folder_ic);
          }
        }
      }

    ++array_id;
    key_data = Registry::Key("DataArray[%03d]", array_id);
    }
}

