#include "LevelSetMeshWrapper.h"
#include "MeshWrapperBase.h"
#include "Rebroadcaster.h"

//--------------------------------------------
//  LevelSetMeshAssembly Implementation
//--------------------------------------------

LevelSetMeshAssembly
::LevelSetMeshAssembly()
{
  m_Pipeline = LevelSetMeshPipeline::New();
}


LevelSetMeshAssembly
::~LevelSetMeshAssembly()
{

}

LevelSetMeshPipeline *
LevelSetMeshAssembly
::GetPipeline()
{
  return m_Pipeline.GetPointer();
}

void
LevelSetMeshAssembly
::SetImage(InputImageType *image)
{
  m_Image = image;
  m_Pipeline->SetImage(image);
}

void
LevelSetMeshAssembly
::UpdateMeshAssembly(LabelType id, std::mutex *mutex)
{
  // Run the UpdateMesh for the current tp assembly
  m_Pipeline->UpdateMesh(mutex);

  // Post Update. Update mesh assmebly
  vtkPolyData *mesh = m_Pipeline->GetMesh();

  assert(mesh);

  if (m_Meshes.count(id))
    m_Meshes[id]->SetPolyData(mesh);
  else
    {
    auto polyWrapper = PolyDataWrapper::New();
    polyWrapper->SetPolyData(mesh);
    this->AddMesh(polyWrapper, id);
    }

  // Update the modified time stamp
  this->Modified();
}

bool
LevelSetMeshAssembly
::IsAssemblyDirty() const
{
  bool ret = this->GetMTime() < m_Image->GetMTime();

  if (m_MeshOptions && m_MeshOptions->GetMTime() >= this->GetMTime())
    ret = true;

  return ret;
}

void
LevelSetMeshAssembly
::SetMeshOptions(const MeshOptions *options)
{
  m_MeshOptions = options;
  m_Pipeline->SetMeshOptions(options);
}


//--------------------------------------------
//  LevelSetMeshWrapper Implementation
//--------------------------------------------


LevelSetMeshWrapper
::LevelSetMeshWrapper()
{
}

void
LevelSetMeshWrapper
::Initialize(MeshOptions *meshOption, ColorLabelTable *colorTable)
{
  m_MeshOptions = meshOption;
  m_DisplayMapping = LabelMeshDisplayMappingPolicy::New();
  m_DisplayMapping->SetMesh(this);
  m_DisplayMapping->SetColorLabelTable(colorTable);
}

void
LevelSetMeshWrapper
::SetMesh(vtkPolyData*, unsigned int, LabelType)
{

}

bool
LevelSetMeshWrapper
::IsMeshDirty(unsigned int timepoint)
{
  if (m_MeshAssemblyMap.count(timepoint))
    {
    auto lsAssembly = static_cast<LevelSetMeshAssembly*>(m_MeshAssemblyMap[timepoint].GetPointer());
    if (lsAssembly)
      return lsAssembly->IsAssemblyDirty();
    }

  return false;
}

unsigned long
LevelSetMeshWrapper
::GetAssemblyMTime(unsigned int tp)
{
  if (m_MeshAssemblyMap.count(tp))
    {
    auto lsAssembly = static_cast<LevelSetMeshAssembly*>(m_MeshAssemblyMap[tp].GetPointer());
    if (lsAssembly)
      return lsAssembly->GetMTime();
    }

  return 0;
}


void
LevelSetMeshWrapper
::CreateNewAssembly(LevelSetImageWrapper *lsImg, unsigned int timepoint)
{
  auto lsAssembly = LevelSetMeshAssembly::New();

  lsAssembly->SetImage(lsImg->GetModifiableImage());
  lsAssembly->SetMeshOptions(m_MeshOptions);

  m_MeshAssemblyMap[timepoint] = lsAssembly.GetPointer();

  MeshAssembly *assembly = m_MeshAssemblyMap[timepoint];

  Rebroadcaster::Rebroadcast(assembly, itk::ModifiedEvent(),
                             this, ValueChangedEvent());
}

void
LevelSetMeshWrapper
::UpdateMeshes(LevelSetImageWrapper *lsImg, unsigned int timepoint, LabelType id, std::mutex *mutex)
{
  if (!m_MeshAssemblyMap.count(timepoint))
    {
    CreateNewAssembly(lsImg, timepoint);
    }

  auto assembly = static_cast<LevelSetMeshAssembly*>(
        m_MeshAssemblyMap[timepoint].GetPointer());

  assembly->UpdateMeshAssembly(id, mutex);

}

void
LevelSetMeshWrapper
::SaveToRegistry(Registry &)
{

}

void
LevelSetMeshWrapper
::LoadFromRegistry(Registry &, std::string &, std::string &, unsigned int)
{

}

