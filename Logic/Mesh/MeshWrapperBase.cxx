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
MeshWrapperBase::
MergeDataProperties(MeshDataArrayPropertyMap &dest, MeshDataArrayPropertyMap &src)
{
  for (auto cit = src.cbegin(); cit != src.cend(); ++cit)
    {
    if (dest.count(cit->first))
      {
      dest[cit->first]->Merge(cit->second);
      }
    else
      dest[cit->first] = cit->second;
    }
}

void
MeshWrapperBase::SetMesh(vtkSmartPointer<vtkPolyData> mesh, unsigned int timepoint, LabelType id)
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
      auto assembly = MeshAssembly::New();
      assembly->AddMesh(wrapper, id);
      m_MeshAssemblyMap[timepoint] = assembly;
    }
}

SmartPtr<PolyDataWrapper>
MeshWrapperBase::GetMesh(unsigned int timepoint, LabelType id)
{
  PolyDataWrapper *ret = nullptr;

  if (m_MeshAssemblyMap.count(timepoint))
    ret = m_MeshAssemblyMap[timepoint]->GetMesh(id);

  return ret;
}

SmartPtr<MeshAssembly>
MeshWrapperBase::GetMeshAssembly(unsigned int timepoint)
{
  MeshAssembly *ret = nullptr;

  if (m_MeshAssemblyMap.count(timepoint))
    ret = m_MeshAssemblyMap[timepoint];

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

SmartPtr<MeshDataArrayProperty>
MeshWrapperBase::GetActiveDataArrayProperty()
{
  MeshDataArrayProperty *ret = nullptr;
  if (m_MeshLayerDataPropertyMap.count(m_ActiveDataPropertyId))
    ret = m_MeshLayerDataPropertyMap[m_ActiveDataPropertyId];

  return ret;
}


void
MeshWrapperBase::
SetActiveMeshLayerDataPropertyId(int id)
{
  m_ActiveDataPropertyId = id;
}

// ========================================
//  MeshAssembly Implementation
// ========================================
void MeshAssembly::AddMesh(SmartPtr<PolyDataWrapper> mesh, LabelType id)
{
  m_Meshes[id] = mesh;
}

SmartPtr<PolyDataWrapper>
MeshAssembly::GetMesh(LabelType id)
{
  return m_Meshes[id];
}

bool
MeshAssembly::Exist(LabelType id)
{
  return m_Meshes.count(id);
}

// ========================================
//  PolyDataWrapper Implementation
// ========================================
void PolyDataWrapper::SetPolyData(vtkSmartPointer<vtkPolyData> polydata)
{
  m_PolyData = polydata;
}

vtkSmartPointer<vtkPolyData>
PolyDataWrapper::GetPolyData()
{
  assert(m_PolyData);
  return m_PolyData;
}

// ========================================
//  MeshDataArrayProperty Implementation
// ========================================

void
MeshDataArrayProperty::
Initialize(vtkSmartPointer<vtkDataArray> array)
{
  // Copy the array name to
  std::strcpy(m_Name, array->GetName());

  double *range = array->GetRange();
  m_min = range[0];
  m_max = range[1];

  m_ColorMap = ColorMap::New();
  m_IntensityCurve = IntensityCurveVTK::New();
}

void
MeshDataArrayProperty::
Merge(SmartPtr<MeshDataArrayProperty> other)
{
  // the name must be same
  assert(!strcmp(this->m_Name, other->m_Name));

  if (other->m_max > this->m_max)
    this->m_max = other->m_max;

  if (other->m_min < this->m_min)
    this->m_min = other->m_min;
}

MeshDataArrayProperty::MeshDataType
MeshDataArrayProperty::GetType() const
{
  return m_Type;
}



