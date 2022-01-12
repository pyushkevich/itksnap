#include "MeshWrapperBase.h"
#include "vtkPolyData.h"
#include "vtkPointData.h"
#include "vtkCellData.h"
#include "vtkDataSetAttributes.h"
#include <itksys/SystemTools.hxx>

MeshWrapperBase::MeshWrapperBase()
{

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

void
MeshWrapperBase::SetFileName(const std::string &name)
{
  m_FileName = name;
  m_FileNameShort = itksys::SystemTools::GetFilenameWithoutExtension(
        itksys::SystemTools::GetFilenameName(name));
  this->InvokeEvent(WrapperMetadataChangeEvent());
}

const ScalarImageHistogram *
MeshWrapperBase::GetHistogram(size_t nBins)
{
  // placeholder
  return nullptr;
}

void
MeshWrapperBase::SetCustomNickname(const std::string &nickname)
{
  // Make sure the nickname is real
  if(nickname == m_FileNameShort)
    m_CustomNickname.clear();
  else
    m_CustomNickname = nickname;

  this->InvokeEvent(WrapperMetadataChangeEvent());
}

const std::string&
MeshWrapperBase::GetNickname() const
{
  if(m_CustomNickname.length())
    return m_CustomNickname;

  else if(m_FileName.length())
    return m_FileNameShort;

  else return m_DefaultNickname;
}

MeshWrapperBase::MeshDataArrayNameMap
MeshWrapperBase::GetMeshDataArrayNameMap(unsigned int timepoint, LabelType id)
{
  MeshDataArrayNameMap ret;

  // Make sure the mesh exists, to avoid segmentation fault
  if (m_MeshCollectionMap.count(timepoint) && m_MeshCollectionMap[timepoint].count(id))
    {
    vtkDataSetAttributes *data;

    if (m_ActiveMeshDataType == POINT_DATA)
      {
      std::cout << "[MWB] Getting Point Data Names " << std::endl;
      data = GetMesh(timepoint, id)->GetPointData();
      }

    else
      {
      std::cout << "[MWB] Getting Cell Data Names " << std::endl;
      data = GetMesh(timepoint, id)->GetCellData();
      }


    for (auto i = 0; i < data->GetNumberOfArrays(); ++i)
      ret[i] = data->GetArrayName(i);
    }

  return ret;
};

int
MeshWrapperBase::
GetActiveMeshDataArrayId(unsigned int, LabelType)
{
  return m_ActiveMeshDataArrayId;
}


void
MeshWrapperBase::
SetActiveMeshDataArrayId(int index, unsigned int timepoint, LabelType id)
{
  std::cout << "[MWB] Setting Active Data ArrayID=" << index << std::endl;

  if (m_MeshCollectionMap.count(timepoint) && m_MeshCollectionMap[timepoint].count(id))
    {
    vtkDataSetAttributes *data;
    if (m_ActiveMeshDataType == POINT_DATA)
      data = GetMesh(timepoint, id)->GetPointData();
    else
      data = GetMesh(timepoint, id)->GetCellData();


    data->SetActiveAttribute(index, 0);
    m_ActiveMeshDataArrayId = index;
    }

}
