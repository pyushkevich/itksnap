#include "MeshWrapperBase.h"
#include "vtkPointData.h"
#include "vtkCellData.h"
#include "vtkDataSetAttributes.h"
#include <itksys/SystemTools.hxx>
#include "MeshDisplayMappingPolicy.h"

// ========================================
//  AbstractMeshDataArrayProperty Implementation
// ========================================
AbstractMeshDataArrayProperty::
AbstractMeshDataArrayProperty()
{

}

AbstractMeshDataArrayProperty::
~AbstractMeshDataArrayProperty()
{
  delete[] m_Name;
}

void
AbstractMeshDataArrayProperty::
Initialize(vtkDataArray *array, MeshDataType type)
{
  // Copy the array name to
  m_Name = new char[strlen(array->GetName())];
  strcpy(m_Name, array->GetName());

  double *range = array->GetRange();
  m_min = range[0];
  m_max = range[1];

  m_Type = type;

  // Populate Component Name
  // -- only populate component name from the initial array
  // -- with assumption that array components are immutable
  const std::string noname = "Component ";
  size_t noname_id = 0u;
  for (int i = 0; i < array->GetNumberOfComponents(); ++i)
    {
    std::string strName;
    auto cName = array->GetComponentName(i);
    if (cName && strlen(cName))
      strName = cName;
    else
      strName = noname + std::to_string(noname_id++);

    m_ComponentNameMap[i] = strName;
    }
}

void
AbstractMeshDataArrayProperty::
Update(vtkDataArray *array)
{
  if (!strcmp(m_Name, array->GetName()))
    return;

  double *range = array->GetRange();
  m_min = range[0];
  m_max = range[1];
}

AbstractMeshDataArrayProperty::MeshDataType
AbstractMeshDataArrayProperty::GetType() const
{
  return m_Type;
}

void
AbstractMeshDataArrayProperty::
Print(ostream &os) const
{
  os << "[AbstractMeshDataArrayProperty]" << std::endl;
  std::cout << "name: " << m_Name << std::endl;
  std::cout << "type: " << m_Type << std::endl;
  std::cout << "min: " << m_min << std::endl;
  std::cout << "max: " << m_max << std::endl;
}

// ========================================
//  MeshDataArrayProperty Implementation
// ========================================
MeshDataArrayProperty::
MeshDataArrayProperty()
{

}

MeshDataArrayProperty::
~MeshDataArrayProperty()
{

}

void
MeshDataArrayProperty::
SetDataPointer(vtkDataArray *array)
{
  m_DataPointer = array;
}


// ============================================
//  MeshLayerDataArrayProperty Implementation
// ============================================



MeshLayerDataArrayProperty::
MeshLayerDataArrayProperty()
{
  m_ColorMap = ColorMap::New();
  m_ColorMap->SetToSystemPreset(
        static_cast<ColorMap::SystemPreset>
        (ColorMap::SystemPreset::COLORMAP_JET));
  m_IntensityCurve = IntensityCurveVTK::New();
  m_IntensityCurve->Initialize();
  m_HistogramFilter = HistogramFilterType::New();
  m_HistogramFilter->SetNumberOfBins(DEFAULT_HISTOGRAM_BINS);
  m_MinMaxFilter = MinMaxFilterType::New();

  m_VectorModeNameMap =
  {
    {0, "Magnitude"},
    {1, "RGBColors"}
  };
}


void
MeshLayerDataArrayProperty::
Initialize(MeshDataArrayProperty *other)
{
  this->m_Name = new char[strlen(other->GetName())];
  strcpy(this->m_Name, other->GetName());
  this->m_min = other->GetMin();
  this->m_max = other->GetMax();
  this->m_Type = other->GetType();

  m_DataPointerList.push_back(other->GetDataPointer());

  // Copy the component name map
  for (auto kv : other->GetComponentNameMap())
    {
    m_ComponentNameMap[kv.first] = kv.second;
    }

  // Update VectorModeNameMap
  UpdateVectorModeNameMap();
}

void
MeshLayerDataArrayProperty
::UpdateVectorModeNameMap()
{
  // Iterate over the component name map, check existence of components,
  // add missing entries
  for (auto kv : m_ComponentNameMap)
    {
    auto shifted = kv.first + m_VectorModeShiftSize;
    if (m_VectorModeNameMap.count(shifted) == 0)
      m_VectorModeNameMap[shifted] = kv.second;
    }
}

void
MeshLayerDataArrayProperty::
Merge(MeshDataArrayProperty *other)
{
  // the name must be same
  assert(!strcmp(this->m_Name, other->GetName()));

  if (other->GetMax() > this->m_max)
    this->m_max = other->GetMax();

  if (other->GetMin() < this->m_min)
    this->m_min = other->GetMin();

  auto it = std::find(m_DataPointerList.begin(), m_DataPointerList.end(),
            other->GetDataPointer());

  if (it == m_DataPointerList.end())
    m_DataPointerList.push_back(other->GetDataPointer());

  // add missing components into the component name map
  for (auto kv : other->GetComponentNameMap())
    {
    // add new component into the componentn name map
    // no shifting because it's not vectormode yet
    if (m_ComponentNameMap.count(kv.first) == 0)
      m_ComponentNameMap[kv.first] = kv.second;
    }

  // Update vector mode name map
  UpdateVectorModeNameMap();
}

void
MeshLayerDataArrayProperty
::SetActiveVectorMode(int mode)
{
  m_ActiveVectorMode = mode;
}

ScalarImageHistogram*
MeshLayerDataArrayProperty::
GetHistogram(size_t nBins) const
{
  if (nBins > 0)
    m_HistogramFilter->SetNumberOfBins(nBins);

  std::vector<double> tmpdata;
  long n = 0;

  for (auto cit = m_DataPointerList.cbegin(); cit != m_DataPointerList.cend(); ++cit)
    {
    auto array = *cit;
    n += array->GetNumberOfTuples();
    }

  DataArrayImageType::Pointer img = DataArrayImageType::New();

  DataArrayImageType::IndexType start;
  start[0] = 0;

  DataArrayImageType::SizeType size;
  size[0] = n;

  DataArrayImageType::RegionType region;
  region.SetSize(size);
  region.SetIndex(start);

  img->SetRegions(region);
  img->Allocate();

  DataArrayImageType::IndexType idx;
  idx[0] = 0;

  for (auto cit = m_DataPointerList.cbegin(); cit != m_DataPointerList.cend(); ++cit)
    {
    auto array = *cit;
    auto nTuple = array->GetNumberOfTuples();
    for (auto i = 0; i < nTuple; ++i)
      {
      auto v = array->GetComponent(i, 0);
      img->SetPixel(idx, v);
      idx[0]++;

      if (idx[0] >= n)
        break;
      }
    if (idx[0] >= n)
      break;
    }

  //std::cout << "[MeshLayerDataArrayProp] img" << img << std::endl;

  m_HistogramFilter->SetInput(img);
  m_MinMaxFilter->SetInput(img);

  m_HistogramFilter->SetRangeInputs(m_MinMaxFilter->GetMinimumOutput(),
                                    m_MinMaxFilter->GetMaximumOutput());

  m_HistogramFilter->Update();

  return m_HistogramFilter->GetHistogramOutput();
}



// ========================================
//  PolyDataWrapper Implementation
// ========================================
void PolyDataWrapper::SetPolyData(vtkPolyData *polydata)
{
  m_PolyData = polydata;
  UpdateDataArrayProperties();
  this->Modified();
}

vtkPolyData*
PolyDataWrapper::GetPolyData()
{
  assert(m_PolyData);
  return m_PolyData;
}

void
PolyDataWrapper::UpdateDataArrayProperties()
{
  assert(m_PolyData);

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
  // Process creation and update
  for (int i = 0; i < data->GetNumberOfArrays(); ++i)
    {
      auto arr = vtkDataArray::SafeDownCast(data->GetAbstractArray(i));

      // Only Process valid vtkdataarray
      if (arr)
        {
        // Get the name of the array
        auto name = arr->GetName();

        // Skip arrays without a name
        if (!name)
          continue;

        if (propMap.count(name))
          {
          // Update the existing entry
          propMap[name]->Update(arr);
          }
        else
          {
          // Create a new property
          auto prop = MeshDataArrayProperty::New();
          prop->Initialize(arr, type);
          prop->SetDataPointer(arr);
          propMap[name] = prop;
          }
        }
    }

  // Remove array props no longer exist
  for (auto it = propMap.begin(); it != propMap.end(); ++it)
    {
    if (!data->HasArray(it->first))
      {
      auto temp = it->second;
      propMap.erase(it);
      temp->Delete();
      }
    }
}

// ========================================
//  MeshAssembly Implementation
// ========================================
void MeshAssembly::AddMesh(PolyDataWrapper *mesh, LabelType id)
{
  m_Meshes[id] = mesh;
}

PolyDataWrapper*
MeshAssembly::GetMesh(LabelType id)
{
  PolyDataWrapper *ret = nullptr;

  if (m_Meshes.count(id))
    ret = m_Meshes[id];

  return ret;
}

bool
MeshAssembly::Exist(LabelType id)
{
  return m_Meshes.count(id);
}

void
MeshAssembly::Erase(LabelType id)
{
  // Do nothing if id not exist
  if (m_Meshes.count(id) == 0)
    return;

  // Erase id from the map
  m_Meshes.erase(m_Meshes.find(id));
}

// ============================================
//  MeshWrapperBase Implementation
// ============================================

MeshWrapperBase::MeshWrapperBase()
{

}

MeshWrapperBase::~MeshWrapperBase()
{
}

void
MeshWrapperBase::
MergeDataProperties(MeshLayerDataArrayPropertyMap &dest, MeshDataArrayPropertyMap &src)
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
      auto newprop = MeshLayerDataArrayProperty::New();
      newprop->Initialize(cit->second);
      dest[cit->first] = newprop;
      m_CombinedDataPropertyMap[m_CombinedPropID++] = newprop;
      }

    }
}

PolyDataWrapper*
MeshWrapperBase::GetMesh(unsigned int timepoint, LabelType id)
{
  PolyDataWrapper *ret = nullptr;

  if (m_MeshAssemblyMap.count(timepoint))
    ret = m_MeshAssemblyMap[timepoint]->GetMesh(id);

  return ret;
}

MeshAssembly*
MeshWrapperBase::GetMeshAssembly(unsigned int timepoint)
{
  MeshAssembly *ret = nullptr;

  if (m_MeshAssemblyMap.count(timepoint))
    ret = m_MeshAssemblyMap[timepoint];

  return ret;
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
  auto prop = GetActiveDataArrayProperty();
  return prop->GetHistogram(nBins);
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

SmartPtr<MeshLayerDataArrayProperty>
MeshWrapperBase::GetActiveDataArrayProperty()
{
  MeshLayerDataArrayProperty *ret = nullptr;
  if (m_CombinedDataPropertyMap.count(m_ActiveDataPropertyId))
    ret = m_CombinedDataPropertyMap[m_ActiveDataPropertyId];

  return ret;
}

void
MeshWrapperBase::
SetActiveMeshLayerDataPropertyId(int id)
{
  if (id < 0 || m_ActiveDataPropertyId == id)
    return;

  m_ActiveDataPropertyId = id;

  // if failed check caller's logic
  assert(m_CombinedDataPropertyMap.count(id));

  // check is point or cell data
  auto prop = m_CombinedDataPropertyMap[id];

  // Change the active array
  if (prop->GetType() == MeshDataArrayProperty::POINT_DATA)
    {
    for (auto cit = m_MeshAssemblyMap.cbegin(); cit != m_MeshAssemblyMap.cend(); ++cit)
      {
      for (auto polyIt = cit->second->cbegin(); polyIt != cit->second->cend(); ++polyIt)
        {

        auto pointData = polyIt->second->GetPolyData()->GetPointData();
        pointData->SetActiveAttribute(prop->GetName(),
                               vtkDataSetAttributes::SCALARS);
        }
      }
    }
  else if (prop->GetType() == MeshDataArrayProperty::CELL_DATA)
    {
    for (auto cit = m_MeshAssemblyMap.cbegin(); cit != m_MeshAssemblyMap.cend(); ++cit)
      for (auto polyIt = cit->second->cbegin(); polyIt != cit->second->cend(); ++polyIt)
        {
        polyIt->second->GetPolyData()->GetCellData()->
            SetActiveAttribute(prop->GetName(),
                               vtkDataSetAttributes::SCALARS);
        }
    }

  auto dmp = GetMeshDisplayMappingPolicy();
  dmp->SetColorMap(prop->GetColorMap());
  dmp->SetIntensityCurve(prop->GetIntensityCurve());

  InvokeEvent(WrapperDisplayMappingChangeEvent());
  InvokeEvent(itk::ModifiedEvent());
}

