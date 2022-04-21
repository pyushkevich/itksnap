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
::UpdateMeshAssembly(std::mutex *mutex)
{
  std::cout << "[LSAssembly] UpdateMeshAssembly" << std::endl;

  // Run the UpdateMesh for the current tp assembly
  m_Pipeline->UpdateMesh(mutex);

  // Post Update. Update mesh assmebly
  vtkPolyData *mesh = m_Pipeline->GetMesh();

  assert(mesh);

  const LabelType id = 0;
  if (m_Meshes.count(id))
    m_Meshes[id]->SetPolyData(mesh);
  else
    {
    auto polyWrapper = PolyDataWrapper::New();
    polyWrapper->SetPolyData(mesh);
    this->AddMesh(polyWrapper);
    }

  std::cout << "-- Completed" << std::endl;

  // Update the modified time stamp
  this->Modified();
}

bool
LevelSetMeshAssembly
::IsAssemblyDirty() const
{
  std::cout << "[LSAssembly] PipeMTime=" << this->GetMTime()
            << "; ImageMTime=" << m_Image->GetMTime() << std::endl;

  return this->GetMTime() < m_Image->GetMTime();
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
::Initialize(MeshOptions *meshOption)
{
  m_MeshOptions = meshOption;
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

void
LevelSetMeshWrapper
::CreateNewAssembly(LevelSetImageWrapper *lsImg, unsigned int timepoint)
{
  std::cout << "[LSWrapper] Create New Assembly" << std::endl;

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
::UpdateMeshes(LevelSetImageWrapper *lsImg, unsigned int timepoint, std::mutex *mutex)
{
  std::cout << "[LSWrapper] UpdateMeshes" << std::endl;

  if (!m_MeshAssemblyMap.count(timepoint))
    {
    CreateNewAssembly(lsImg, timepoint);
    }

  auto assembly = static_cast<LevelSetMeshAssembly*>(
        m_MeshAssemblyMap[timepoint].GetPointer());

  assembly->UpdateMeshAssembly(mutex);

}
