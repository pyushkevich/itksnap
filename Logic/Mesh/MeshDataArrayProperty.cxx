#include "MeshDataArrayProperty.h"
#include "MeshDisplayMappingPolicy.h"
#include "ColorMap.h"
#include "Rebroadcaster.h"

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

double
AbstractMeshDataArrayProperty
::GetMin(vtkIdType comp) const
{
	if (comp == -1)
		return m_MagMin;
	else
		{
    assert(comp >= 0 && comp < (vtkIdType)this->GetNumberOfComponents());
		return m_MeshArrayComponentMap.at(comp).m_Min;
		}
}

double
AbstractMeshDataArrayProperty
::GetMax(vtkIdType comp) const
{
	if (comp == -1)
		return m_MagMax;
	else
		{
    assert(comp >= 0 && comp < (vtkIdType)this->GetNumberOfComponents());
		return m_MeshArrayComponentMap.at(comp).m_Max;
		}
}


void
AbstractMeshDataArrayProperty::
Initialize(vtkDataArray *array, MeshDataType type)
{
  // Copy the array name to
  m_Name = new char[strlen(array->GetName()) + 1];
	strcpy(m_Name, array->GetName());

	double *range = array->GetRange(-1);
	m_MagMin = range[0];
	m_MagMax = range[1];
  m_Type = type;

  // Populate Component Name
  // -- only populate component name from the initial array
  // -- with assumption that array components are immutable
	const char *default_name = "Component ";
	size_t default_id = 0u;
  for (int i = 0; i < array->GetNumberOfComponents(); ++i)
    {
		// Process the name
		std::ostringstream oss;
    auto cName = array->GetComponentName(i);

    if (cName && strlen(cName))
			oss << cName;
    else
			oss << default_name <<  default_id++;

		// Process min max
		double r[2];
		array->GetRange(r, i);

		// Create the component
		m_MeshArrayComponentMap[i] = MeshArrayComponent(oss.str().c_str(), r[0], r[1]);
    }
}

void
AbstractMeshDataArrayProperty::
Update(vtkDataArray *array)
{
  if (!strcmp(m_Name, array->GetName()))
    return;

	double *range = array->GetRange(-1);
	m_MagMin = range[0];
	m_MagMax = range[1];
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
	os << "AbstractMeshDataArrayProperty -----" << std::endl;
	os << "name: " << m_Name << std::endl;
	os << "type: " << m_Type << std::endl;
	os << "Magnitude Min: " << m_MagMin << std::endl;
	os << "Magnitude Max: " << m_MagMax << std::endl;
	os << "Number of Components: " << m_MeshArrayComponentMap.size() << std::endl;
	if (m_MeshArrayComponentMap.size() > 1)
		{
		for (auto &kv : m_MeshArrayComponentMap)
			{
			os << "-- Component: " << kv.first << std::endl;
			os << "---- Name: " << kv.second.m_Name << std::endl;
			os << "---- Range: [" << kv.second.m_Min << ',' << kv.second.m_Max << "]" << std::endl;
			}
		}
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
	m_MagnitudeIntensityCurve = IntensityCurveVTK::New();
	m_MagnitudeIntensityCurve->Initialize();
  SetActiveIntensityCurve(m_MagnitudeIntensityCurve);
	m_ActiveVectorMode = VectorMode::MAGNITUDE;
	m_ActiveComponentId = 0;
  m_HistogramFilter = HistogramFilterType::New();
  m_HistogramFilter->SetNumberOfBins(DEFAULT_HISTOGRAM_BINS);
  m_MinMaxFilter = MinMaxFilterType::New();
}


void
MeshLayerDataArrayProperty::
Initialize(MeshDataArrayProperty *other)
{
  this->m_Name = new char[strlen(other->GetName()) + 1];
  strcpy(this->m_Name, other->GetName());
	this->m_MagMin = other->GetMin(-1);
	this->m_MagMax = other->GetMax(-1);
  this->m_Type = other->GetType();

  m_DataPointerList.push_back(other->GetDataPointer());

  // Copy the component name map
	for (auto &kv : other->GetMeshArrayComponentMap())
    {
		m_MeshArrayComponentMap[kv.first] = kv.second;
    }
}

IntensityCurveVTK*
MeshLayerDataArrayProperty
::GetActiveIntensityCurve()
{
	return m_ActiveIntensityCurve;
}

void
MeshLayerDataArrayProperty
::SetActiveIntensityCurve(IntensityCurveVTK *curve)
{
	// Add new curve
	m_ActiveIntensityCurve = curve;
}

void
MeshLayerDataArrayProperty::
Merge(MeshDataArrayProperty *other)
{
	// the name must be same
  assert(!strcmp(this->m_Name, other->GetName()));

	if (other->GetMax(-1) > this->m_MagMax)
		this->m_MagMax = other->GetMax(-1);

	if (other->GetMin(-1) < this->m_MagMin)
		this->m_MagMin = other->GetMin(-1);

  auto it = std::find(m_DataPointerList.begin(), m_DataPointerList.end(),
            other->GetDataPointer());

  if (it == m_DataPointerList.end())
    m_DataPointerList.push_back(other->GetDataPointer());

  // add missing components into the component name map
	for (auto &kv : other->GetMeshArrayComponentMap())
    {
		if (m_MeshArrayComponentMap.count(kv.first) == 0)
			m_MeshArrayComponentMap[kv.first] = kv.second;
    }

	InvokeEvent(itk::ModifiedEvent());
}

void
MeshLayerDataArrayProperty
::SetActiveVectorMode(int mode, vtkIdType compId)
{
  assert (mode >= 0 && mode < VectorMode::COUNT);

	if ((VectorMode)mode == m_ActiveVectorMode &&
			(compId < 0 || compId == m_ActiveComponentId))
		return; // No Change to make

	m_ActiveVectorMode = (VectorMode)mode;
	m_ActiveComponentId = compId;

	if (mode == VectorMode::MAGNITUDE)
    SetActiveIntensityCurve(m_MagnitudeIntensityCurve);
	else
    SetActiveIntensityCurve(GetActiveComponent().m_IntensityCurve);

	InvokeEvent(WrapperHistogramChangeEvent());
}

inline double GetMagnitude(double *vector, size_t len)
{
	size_t crnt = 0;
	double sum = 0;
	while (crnt < len)
		{
			float t = *vector;
			sum += t * t;
			++vector;
			++crnt;
		}

	return sqrt(sum);
}

ScalarImageHistogram*
MeshLayerDataArrayProperty::
GetHistogram(size_t nBins)
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

	// get active component
	vtkIdType activeVecComp = -1;
	if (this->GetActiveVectorMode() == VectorMode::COMPONENT)
		activeVecComp = this->GetActiveComponentId();

  for (auto cit = m_DataPointerList.cbegin(); cit != m_DataPointerList.cend(); ++cit)
    {
    auto array = *cit;
    auto nTuple = array->GetNumberOfTuples();
    for (auto i = 0; i < nTuple; ++i)
      {
			double v = 0;
			if (activeVecComp == -1) // use magnitude
				{
				size_t nc = array->GetNumberOfComponents();
				double *vec  = new double[nc];
				for (size_t c = 0; c < nc; ++c)
					vec[c] = array->GetComponent(i, c);
				v = GetMagnitude(vec, nc);
				delete [] vec;
				}
			else
				v = array->GetComponent(i, activeVecComp);
      img->SetPixel(idx, v);
      idx[0]++;

      if (idx[0] >= n)
        break;
      }
    if (idx[0] >= n)
      break;
    }

  m_HistogramFilter->SetInput(img);
  m_MinMaxFilter->SetInput(img);
  m_HistogramFilter->SetRangeInputs(m_MinMaxFilter->GetMinimumOutput(),
                                    m_MinMaxFilter->GetMaximumOutput());
	m_MinMaxFilter->Update();
  m_HistogramFilter->Update();

  return m_HistogramFilter->GetHistogramOutput();
}
