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

  // Set Default active array id
  SetActiveMeshLayerDataPropertyId(m_CombinedDataPropertyMap.cbegin()->first);

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
    kv.second->GetIntensityCurve()->SaveToRegistry(crnt.Folder("IntensityCurve"));
    }
}

void
StandaloneMeshWrapper
::ReadFromRegistry(Registry &)
{

}

