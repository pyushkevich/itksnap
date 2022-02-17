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
      // Merge with existing
      dest[cit->first]->Merge(cit->second);
      }
    else
      {
      // Add new entry
      auto newprop = MeshDataArrayProperty::New();
      cit->second->DeepCopy(newprop);
      dest[cit->first] = newprop;
      m_CombinedDataPropertyMap[++m_CombinedPropID] = newprop;
      }

    }
}

void
MeshWrapperBase::SetMesh(vtkSmartPointer<vtkPolyData> mesh, unsigned int timepoint, LabelType id)
{
  std::cout << "[MeshWrapperBase] SetMesh called" << std::endl;

  auto wrapper = PolyDataWrapper::New();
  wrapper->SetPolyData(mesh);

  auto pointDataProps = wrapper->GetPointDataProperties();
  auto cellDataProps = wrapper->GetCellDataProperties();

  std::cout << "[MeshWrapperBase] pointData size=" << pointDataProps.size() << std::endl;

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
  if (m_CombinedDataPropertyMap.count(m_ActiveDataPropertyId))
    ret = m_CombinedDataPropertyMap[m_ActiveDataPropertyId];

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
  UpdateDataArrayProperties();
}

vtkSmartPointer<vtkPolyData>
PolyDataWrapper::GetPolyData()
{
  assert(m_PolyData);
  return m_PolyData;
}

void
PolyDataWrapper::UpdateDataArrayProperties()
{
  assert(m_PolyData);

  std::cout << "[PolyDataWrapper] Update Array Props" << std::endl;

  // Process Point Data
  auto pointData = m_PolyData->GetPointData();
  UpdatePropertiesFromVTKData(m_PointDataProperties, pointData,
                              MeshDataType::POINT_DATA);

  // Process Cell Data
  auto cellData = m_PolyData->GetCellData();
  UpdatePropertiesFromVTKData(m_CellDataProperties, cellData,
                              MeshDataType::CELL_DATA);
}

void
PolyDataWrapper::
UpdatePropertiesFromVTKData(MeshDataArrayPropertyMap &propMap,
                            vtkDataSetAttributes *data,
                            MeshDataType type) const
{
  std::cout << "[PolyDataWrapper] Number of Array=" << data->GetNumberOfArrays() << std::endl;
  std::cout << "[PolyDataWrapper] propMap size=" << propMap.size() << std::endl;

  for (int i = 0; i < data->GetNumberOfArrays(); ++i)
    {
      auto arr = vtkDataArray::SafeDownCast(data->GetAbstractArray(i));

      // Only Process valid vtkdataarray
      if (arr)
        {
        // Get the name of the array
        auto name = arr->GetName();
        if (propMap.count(name))
          {
          // Update the existing entry
          propMap[name]->Update(arr);
          }
        else
          {
          // Create a new property
          auto prop = MeshDataArrayProperty::New();
          std::cout << "[PolyDataWrapper] new prop created" << std::endl;
          prop->Initialize(arr, type);
          std::cout << "[PolyDataWrapper] new prop initialized" << std::endl;
          propMap[name] = prop;
          }
        }
    }
}

// ========================================
//  MeshDataArrayProperty Implementation
// ========================================
MeshDataArrayProperty::
~MeshDataArrayProperty()
{
  delete[] m_Name;
}

void
MeshDataArrayProperty::
Initialize(vtkSmartPointer<vtkDataArray> array, MeshDataType type)
{
  // Copy the array name to
  m_Name = new char[strlen(array->GetName())];
  strcpy(m_Name, array->GetName());

  double *range = array->GetRange();
  m_min = range[0];
  m_max = range[1];

  m_Type = type;

  m_ColorMap = ColorMap::New();
  m_IntensityCurve = IntensityCurveVTK::New();
}

void
MeshDataArrayProperty::
Update(vtkSmartPointer<vtkDataArray> array)
{
  if (!strcmp(m_Name, array->GetName()))
    return;

  double *range = array->GetRange();
  m_min = range[0];
  m_max = range[1];
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

void
MeshDataArrayProperty::
DeepCopy(MeshDataArrayProperty *other) const
{
  other->m_Name = new char[strlen(this->m_Name)];
  strcpy(other->m_Name, this->m_Name);
  other->m_min = this->m_min;
  other->m_max = this->m_max;
  other->m_Type = this->m_Type;
}



