#include "StandaloneMeshWrapper.h"
#include "DisplayMappingPolicy.h"

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

bool
StandaloneMeshWrapper::IsA(const char *type) const
{
  return Superclass::IsA(type) || (strcmp("StandaloneMeshWrapper", type) == 0);
}
