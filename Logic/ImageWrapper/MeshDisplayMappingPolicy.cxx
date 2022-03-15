#include "MeshDisplayMappingPolicy.h"
#include "IRISException.h"
#include "vtkScalarBarActor.h"
#include "vtkPolyDataMapper.h"
#include "vtkLookupTable.h"
#include "Generic3DRenderer.h"
#include "vtkActor.h"
#include "vtkPolyData.h"
#include "Generic3DRenderer.h"
#include "Rebroadcaster.h"

// ==================================================
//  MeshDisplayMappingPolicy Implementation
// ==================================================

MeshDisplayMappingPolicy::MeshDisplayMappingPolicy()
{

}

IntensityCurveInterface *
MeshDisplayMappingPolicy::GetIntensityCurve() const
{
  return m_IntensityCurve;
}

ColorMap *
MeshDisplayMappingPolicy::GetColorMap() const
{
  return m_ColorMap;
}

ImageWrapperBase::DisplaySlicePointer
MeshDisplayMappingPolicy::GetDisplaySlice(unsigned int)
{
  return nullptr;
}

void
MeshDisplayMappingPolicy::Save(Registry &)
{

}

void
MeshDisplayMappingPolicy::Restore(Registry &)
{

}

Vector2d
MeshDisplayMappingPolicy::GetNativeImageRangeForCurve()
{
  // Get Active prop
  auto prop = m_Wrapper->GetActiveDataArrayProperty();

  return Vector2d(prop->GetMin(), prop->GetMax());
}

ScalarImageHistogram *
MeshDisplayMappingPolicy::GetHistogram(int nBins)
{
  return m_Wrapper->GetActiveDataArrayProperty()->GetHistogram(nBins);
}

void
MeshDisplayMappingPolicy::SetColorMap(ColorMap *map)
{
  m_ColorMap = map;

  std::cout << "[MeshDMP] color map set" << std::endl;
  Rebroadcaster::Rebroadcast(m_ColorMap, itk::ModifiedEvent(),
                             m_Wrapper, WrapperDisplayMappingChangeEvent());
}

void
MeshDisplayMappingPolicy::SetIntensityCurve(IntensityCurveVTK *curve)
{
  m_IntensityCurve = curve;

  Rebroadcaster::Rebroadcast(m_IntensityCurve, itk::ModifiedEvent(),
                             m_Wrapper, WrapperDisplayMappingChangeEvent());
}

#include "vtkPointData.h"
#include "vtkCellData.h"

void
MeshDisplayMappingPolicy::SetMesh(MeshWrapperBase *mesh_wrapper)
{
  // Set Wrapper
  m_Wrapper = mesh_wrapper;

  // Lookup Table
  m_LookupTable = vtkLookupTable::New();

  // Color Map
  auto cMap = ColorMap::New();
  cMap->SetToSystemPreset(ColorMap::SystemPreset::COLORMAP_WINTER);
  this->SetColorMap(cMap);

  // Intensity Curve
  auto curve = IntensityCurveVTK::New();
  curve->Initialize();
  this->SetIntensityCurve(curve);

  m_Initialized = true;
}

vtkLookupTable*
MeshDisplayMappingPolicy::
GetLookupTable()
{
  UpdateLUT();
  return m_LookupTable;
}

MeshWrapperBase *
MeshDisplayMappingPolicy::GetMeshLayer()
{
  return m_Wrapper;
}

// ==================================================
//  GenericMeshDisplayMappingPolicy Implementation
// ==================================================
GenericMeshDisplayMappingPolicy::
GenericMeshDisplayMappingPolicy()
{

}

GenericMeshDisplayMappingPolicy::
~GenericMeshDisplayMappingPolicy()
{

}

void
GenericMeshDisplayMappingPolicy::
ConfigureActor(vtkActor *actor)
{
  std::cout << "[GenericDMP] ConfigurePolydataMapper" << std::endl;

  // Get active data array property
  auto prop = m_Wrapper->GetActiveDataArrayProperty();
  auto mapper = static_cast<vtkPolyDataMapper*>(actor->GetMapper());

  if (!prop)
    return;

  // Update lookup table
  UpdateLUT();

  // Configure mapper
  mapper->SetLookupTable(m_LookupTable);
  mapper->UseLookupTableScalarRangeOn();

  // This should never happen, check data property implementation
  assert(prop->GetType() != MeshDataType::COUNT);

  std::cout << "-- Active Prop:" << prop << std::endl;

  // point/cell data specific logic
  if (prop->GetType() == MeshDataType::POINT_DATA)
    {
    mapper->SetScalarModeToUsePointData();
    auto pointData = mapper->GetInput()->GetPointData();
    pointData->SetActiveAttribute(prop->GetName(),
                                  vtkDataSetAttributes::AttributeTypes::SCALARS);
    }
  else if (prop->GetType() == MeshDataType::CELL_DATA)
    {
    mapper->SetScalarModeToUseCellData();
    auto cellData = mapper->GetInput()->GetCellData();
    cellData->SetActiveAttribute(prop->GetName(),
                                 vtkDataSetAttributes::AttributeTypes::SCALARS);
    }


  // Set active attribute
  mapper->SetColorModeToMapScalars();
}

void GenericMeshDisplayMappingPolicy::
ConfigureLegend(vtkScalarBarActor* legend)
{
  legend->SetLookupTable(m_LookupTable);

  auto prop = m_Wrapper->GetActiveDataArrayProperty();

  if (!prop)
    return;

  legend->SetTitle(prop->GetName());

  legend->SetVisibility(true);
}


void
GenericMeshDisplayMappingPolicy::
UpdateLUT()
{
  // Get the active property
  auto prop = m_Wrapper->GetActiveDataArrayProperty();

  if (!prop)
    return;

  // Build lookup table
  auto min = prop->GetMin();
  auto max = prop->GetMax();

  const size_t numClr = 256;
  // div change dividing to multiplying, more efficient
  double numdiv = 1.0/numClr;
  double clrdiv = 1.0/255.0;

  m_LookupTable->SetRange(min, max);
  m_LookupTable->SetNumberOfColors(numClr);
  for (auto i = 0u; i < numClr; ++i)
    {
    auto val = m_IntensityCurve->Evaluate(i * numdiv);
    auto rgbaC = m_ColorMap->MapIndexToRGBA(val).GetDataPointer();

    double rgbaD[4];
    for (auto i = 0; i < 4; ++i)
      rgbaD[i] = rgbaC[i] * clrdiv;


    std::cout << "[GenericDMP] UpdateLUT r=" << rgbaD[0] <<
                 "g=" << rgbaD[1] <<
                 "b=" << rgbaD[2] <<
                 "a=" << rgbaD[3] << std::endl;

    m_LookupTable->SetTableValue(i, rgbaD);
    }
  m_LookupTable->Build();
}



// ==================================================
//  LabelMeshDisplayMappingPolicy Implementation
// ==================================================
LabelMeshDisplayMappingPolicy::
LabelMeshDisplayMappingPolicy()
{

}

LabelMeshDisplayMappingPolicy::
~LabelMeshDisplayMappingPolicy()
{

}

void
LabelMeshDisplayMappingPolicy::
ConfigureActor(vtkActor *actor)
{
  auto mapper = static_cast<vtkPolyDataMapper*>(actor->GetMapper());

  // Always update LUT first
  UpdateLUT();

  mapper->SetLookupTable(m_LookupTable);
  mapper->UseLookupTableScalarRangeOn();

  std::cout << "-- data print: " << std::endl;
  auto polyData = mapper->GetInput();
  polyData->Print(std::cout);

  polyData->GetPointData()->SetActiveAttribute("Label", vtkDataSetAttributes::SCALARS);
  mapper->SetScalarModeToUsePointData();
  mapper->SetColorModeToMapScalars();
}

void
LabelMeshDisplayMappingPolicy::
ConfigureLegend(vtkScalarBarActor* legend)
{
  legend->SetLookupTable(m_LookupTable);
  legend->SetTitle("Label");
  legend->SetNumberOfLabels(m_LookupTable->GetNumberOfColors());

  // Hide the legend for label mesh
  legend->SetVisibility(true);
}

void
LabelMeshDisplayMappingPolicy::
UpdateLUT()
{
  std::cout << "[UpdateLUT] LabelMesh" << std::endl;

  //m_LookupTable->SetIndexedLookup(true);
  size_t numClr = m_ColorLabelTable->GetNumberOfValidLabels();
  std::cout << "-- valid #Label=" << numClr << std::endl;
  m_LookupTable->SetNumberOfColors(numClr);
  m_LookupTable->SetRange(0, 6);

  double rgbaD[4];
  for (auto cit = m_ColorLabelTable->begin();
       cit != m_ColorLabelTable->end(); ++cit)
    {
    auto rgbD = cit->second.GetRGBAsDoubleVector().data_block();

    rgbaD[0] = rgbD[0];
    rgbaD[1] = rgbD[1];
    rgbaD[2] = rgbD[2];
    rgbaD[3] = cit->second.GetAlpha() / 255.0;

    std::cout << "-- label" << cit->first << ": ("
              << rgbaD[0] << ", " << rgbaD[1] << ", " << rgbaD[2] << ", " << rgbaD[3]
              << ")" << std::endl;

    m_LookupTable->SetTableValue(cit->first, rgbaD);
    }
  m_LookupTable->Build();
}


