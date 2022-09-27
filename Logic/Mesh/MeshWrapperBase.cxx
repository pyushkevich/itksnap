#include "MeshWrapperBase.h"
#include "MeshDisplayMappingPolicy.h"
#include "Rebroadcaster.h"
#include "IRISApplication.h"
#include <vtkPointData.h>
#include <vtkCellData.h>
#include <vtkDataSetAttributes.h>
#include <itksys/SystemTools.hxx>

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

void
MeshAssembly::
GetCombinedBounds(double bounds[6]) const
{
  for (int i = 0; i < 6; ++i)
    bounds[i] = 0.0;

  for (auto mesh : m_Meshes)
    {
    double crnt[6];
    mesh.second->GetPolyData()->GetBounds(crnt);
    bounds[0] = std::min(crnt[0], bounds[0]);
    bounds[1] = std::max(crnt[1], bounds[1]);
    bounds[2] = std::min(crnt[2], bounds[2]);
    bounds[3] = std::max(crnt[3], bounds[3]);
    bounds[4] = std::min(crnt[4], bounds[4]);
    bounds[5] = std::max(crnt[5], bounds[5]);
    }
}

std::string
MeshAssembly
::GetCombinedBoundsString() const
{

  double bounds[6];
  GetCombinedBounds(bounds);

  std::ostringstream oss;
  oss.precision(3);
  oss << '['
      << '[' << bounds[0] << ',' << bounds[1] << ']'
      << '[' << bounds[2] << ',' << bounds[3] << ']'
      << '[' << bounds[4] << ',' << bounds[5] << ']'
      << ']';
  return oss.str();
}

double
MeshAssembly::
GetTotalMemoryInMB() const
{
  double ret = 0;

  for (auto mesh : m_Meshes)
    ret += (mesh.second->GetPolyData()->GetActualMemorySize() / 1024.0);

  return ret;
}

void
MeshAssembly
::SaveToRegistry(Registry &folder)
{
  for (auto kv : m_Meshes)
    {
    Registry &crnt = folder.Folder(Registry::Key("PolyData[%03d]", kv.first));
    crnt["AbsolutePath"] << kv.second->GetFileName();
    crnt["Format"].PutEnum(GuidedMeshIO::GetEnumFileFormat(), kv.second->GetFileFormat());
    }
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

void
MeshWrapperBase
::SetFileName(const char *filename, unsigned int tp, LabelType id)
{
  // Put the very first file name as a reference file name for the layer
  if (tp == 0 && id == 0)
    SetFileName(filename);

  if (m_MeshAssemblyMap.count(tp))
    {
    auto assembly = m_MeshAssemblyMap[tp];
    if (assembly->Exist(id))
      {
      assembly->GetMesh(id)->SetFileName(filename);
      }
    }
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

	// Remove existing observer from previous active prop
	if (m_CombinedDataPropertyMap.count(m_ActiveDataPropertyId))
		{
		auto oldprop = m_CombinedDataPropertyMap[m_ActiveDataPropertyId];
		oldprop->RemoveObserver(m_ActiveMeshDataPropertyObserverTag);
		}


  m_ActiveDataPropertyId = id;

  // if failed check caller's logic
  assert(m_CombinedDataPropertyMap.count(id));

  // check is point or cell data
  auto prop = m_CombinedDataPropertyMap[id];

	// Rebroadcast vector level histogram change event
	m_ActiveMeshDataPropertyObserverTag =
			Rebroadcaster::Rebroadcast(prop, WrapperHistogramChangeEvent(),
																 this, WrapperHistogramChangeEvent());

	// Change Active Property itself is a histogram change event
	InvokeEvent(WrapperHistogramChangeEvent());

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
  dmp->SetIntensityCurve(prop->GetActiveIntensityCurve());
}

void
MeshWrapperBase::
UpdateMetaData()
{
  // Clear and rebuild the map
  m_MetaDataMap.clear();

  if (m_MeshAssemblyMap.size() == 0)
    return;

  // Update Bounding Box
	for (auto &kv : m_MeshAssemblyMap)
    {
    std::ostringstream oss;

    // The display of timepoint index should always be one-based
    oss << "Bound (timepoint " << kv.first + 1 << ")";
    m_MetaDataMap[oss.str()] = kv.second->GetCombinedBoundsString();
    }

  // Update Number of Time Points
  m_MetaDataMap["Number of Time Points"] = std::to_string(m_MeshAssemblyMap.size());

  // Update Memory Usage
  double memory = 0.0;
	for (auto &kv : m_MeshAssemblyMap)
    memory += kv.second->GetTotalMemoryInMB();

  // Use an oss to format string
  std::ostringstream oss;
  oss << std::fixed << std::setprecision(2) << memory;
  m_MetaDataMap["Memory Usage (MB)"] = oss.str();

  // Update Data Array Properties
	for (auto &kv : m_CombinedDataPropertyMap)
    {
    std::ostringstream oss;
    oss.precision(3);
    oss << '[' << kv.second->GetMin() << ',' << kv.second->GetMax() << ']';
    m_MetaDataMap[kv.second->GetName()] = oss.str();
    }
}

size_t
MeshWrapperBase
::GetNumberOfMeshes(unsigned int timepoint)
{
  size_t ret = 0;

  if (m_MeshAssemblyMap.count(timepoint))
    {
    ret = m_MeshAssemblyMap[timepoint]->size();
    }

  return ret;
}

void
MeshWrapperBase
::LoadFromRegistry(Registry &folder, std::string &orig_dir, std::string &crnt_dir)
{
  GuidedMeshIO io;
  bool moved = (orig_dir.compare(crnt_dir) != 0);

  // Load nicknames and tags
  this->SetCustomNickname(folder["NickName"][""]);
  folder["Tags"].GetList(this->m_Tags);

  // Load mesh timepoint assembly
  auto folder_assembly = folder.Folder("MeshTimePoints");
  bool fnSet = false;
  unsigned int crnt_tp = 1;
  std::string key_tp = Registry::Key("TimePoint[%03d]", crnt_tp);

  while (folder_assembly.HasFolder(key_tp))
    {
    auto folder_tp = folder_assembly.Folder(key_tp);
    unsigned int crnt_poly = 0;
    std::string key_poly = Registry::Key("PolyData[%03d]", crnt_poly);

    while (folder_tp.HasFolder(key_poly))
      {
      auto folder_poly = folder_tp.Folder(key_poly);
      std::string poly_file_full = folder_poly["AbsolutePath"][""];
      if (moved)
        poly_file_full = IRISApplication::GetMovedFilePath(orig_dir, crnt_dir, poly_file_full);

      // Set first filename as placeholder
      if (!fnSet)
        {
        this->SetFileName(poly_file_full);
        fnSet = true;
        }

      FileFormat format = folder_poly["Format"]
          .GetEnum(GuidedMeshIO::GetEnumFileFormat(), FileFormat::FORMAT_COUNT);

      // Load with tp = j-1. The storeing of time point index is zero-based
      io.LoadMesh(poly_file_full.c_str(), format, this, crnt_tp - 1, crnt_poly);

      ++crnt_poly;
      key_poly = Registry::Key("TimePoint[%03d]", crnt_poly);
      }

    ++crnt_tp;
    key_tp = Registry::Key("TimePoint[%03d]", crnt_tp);
    }
}

void
MeshWrapperBase
::SaveToRegistry(Registry &)
{

}

