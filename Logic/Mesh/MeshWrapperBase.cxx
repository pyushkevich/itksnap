#include "MeshWrapperBase.h"
#include "vtkPolyData.h"

MeshWrapperBase::MeshLayerIdType GlobalMeshLayerId = 0ul;

MeshWrapperBase::MeshWrapperBase()
{
  m_Id = GlobalMeshLayerId++;
}

MeshWrapperBase::~MeshWrapperBase()
{
}

void
MeshWrapperBase::SetMeshCollection(MeshCollection collection, unsigned int timepoint)
{
  m_MeshCollectionMap[timepoint] = collection;
}

void
MeshWrapperBase::SetMesh(vtkSmartPointer<vtkPolyData> mesh, unsigned int timepoint, LabelType id)
{
  if (m_MeshCollectionMap.count(timepoint))
    m_MeshCollectionMap[timepoint][id] = mesh;
  else
    {
      std::cout << "[MeshWrapperBase.SetMesh()] creating new timepoint entry" << std::endl;
      MeshCollection mc;
      mc[id] = mesh;
      m_MeshCollectionMap[timepoint] = mc;
    }
}

MeshWrapperBase::MeshCollection
MeshWrapperBase::GetMeshCollection(unsigned int timepoint)
{
  return m_MeshCollectionMap[timepoint];
}

vtkSmartPointer<vtkPolyData>
MeshWrapperBase::GetMesh(unsigned int timepoint, LabelType id)
{
  assert(timepoint);

  vtkPolyData *ret = nullptr;

  if (m_MeshCollectionMap.count(timepoint))
    ret = m_MeshCollectionMap[timepoint][id];

  return ret;
}

bool
MeshWrapperBase::IsA(const char *type) const
{
  return strcmp("MeshWrapperBase", type) == 0;
}

MeshWrapperBase::MeshLayerIdType
MeshWrapperBase::GetId() const
{
  return m_Id;
}

