#include "MeshDataArrayProperty.h"
#include "MeshDisplayMappingPolicy.h"
#include "ColorMap.h"

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

RegistryEnumMap<AbstractMeshDataArrayProperty::MeshDataType>
AbstractMeshDataArrayProperty::m_MeshDataTypeEnumMap =
{
  { AbstractMeshDataArrayProperty::POINT_DATA, "PointData" },
	{ AbstractMeshDataArrayProperty::CELL_DATA, "CellData" },
	{ AbstractMeshDataArrayProperty::FIELD_DATA, "FieldData" }
};


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
    {0, "Magnitude"}
    //,{1, "RGBColors"} // Disabled for now, not working properly
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
