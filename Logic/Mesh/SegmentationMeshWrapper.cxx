#include "SegmentationMeshWrapper.h"
#include "MeshWrapperBase.h"
#include "Rebroadcaster.h"

//--------------------------------------------
//  SegmentationMeshAssembly Implementation
//--------------------------------------------
SegmentationMeshAssembly::
SegmentationMeshAssembly()
{
  m_Pipeline = MultiLabelMeshPipeline::New();
}

SegmentationMeshAssembly::
~SegmentationMeshAssembly()
{
}

MultiLabelMeshPipeline *
SegmentationMeshAssembly::
GetPipeline()
{
  return m_Pipeline;
}

void
SegmentationMeshAssembly::
UpdateMeshAssembly(itk::Command *progress, ImagePointer img, MeshOptions *options)
{
  std::cout << "[SegAssembly] UpdateMeshAssembly" << std::endl;

  // Get the image from current tp and feed the pipeline
  m_Pipeline->SetImage(img);
  m_Pipeline->SetMeshOptions(options);

  std::cout << "-- Pipe Image Set" << std::endl;

  // Run the UpdateMesh for the current tp assembly
  m_Pipeline->UpdateMeshes(progress);

  // Post Update. Update mesh assmebly
  auto collection = m_Pipeline->GetMeshCollection();
  // Process creation and update
  for (auto cit = collection.cbegin(); cit != collection.cend(); ++cit)
    {
    if (this->Exist(cit->first))
      this->GetMesh(cit->first)->SetPolyData(cit->second);
    else
      {
      auto polyWrapper = PolyDataWrapper::New();
      polyWrapper->SetPolyData(cit->second);
      this->AddMesh(polyWrapper, cit->first);
      }
    }
  // Process deletion
  for (auto cit = this->cbegin(); cit != this->cend();)
    {
    if (collection.count(cit->first) == 0)
      this->Erase(cit++->first);
    else
      ++cit;
    }

  // Update the modified time stamp
  this->Modified();

  std::cout << "-- completed" << std::endl;
}


//--------------------------------------------
//  SegmentationMeshWrapper Implementation
//--------------------------------------------

SegmentationMeshWrapper::SegmentationMeshWrapper()
{
}


void
SegmentationMeshWrapper::Initialize(LabelImageWrapper *segImg, MeshOptions *meshOptions)
{
  m_ImagePointer = segImg;
  m_MeshOptions = meshOptions;

  // Configure display mapping policy
  m_DisplayMapping = LabelMeshDisplayMappingPolicy::New();
  m_DisplayMapping->SetMesh(this);
  m_DisplayMapping->SetColorLabelTable(segImg->GetDisplayMapping()->GetLabelColorTable());

  m_Initialized = true;

  std::cout << "[SegWrapper] Initialized" << std::endl;
}

void
SegmentationMeshWrapper
::SetMesh(vtkPolyData*, unsigned int, LabelType)
{

}


bool
SegmentationMeshWrapper::IsMeshDirty(unsigned int timepoint)
{
  auto imgMTime = m_ImagePointer->GetImage()->GetMTime();

  auto assembly = dynamic_cast<SegmentationMeshAssembly*>(GetMeshAssembly(timepoint));
  if (assembly)
    {
    auto pipeMTime = assembly->GetMTime();

    std::cout << "[SegWrapper] imgMTime=" << imgMTime
              << " pipMTime=" << pipeMTime << std::endl;

    if (imgMTime > pipeMTime)
      return true;
    }
  else
    {
    std::cout << "[SegWrapper] mesh is dirty because assembly not found" << std::endl;
    return true;
    }

  return false;
}

void
SegmentationMeshWrapper::
CreateNewAssembly(unsigned int timepoint)
{
  std::cout << "[SegWrapper] Create New Assembly" << std::endl;

  m_MeshAssemblyMap[timepoint] = SegmentationMeshAssembly::New().GetPointer();

  MeshAssembly *assembly = m_MeshAssemblyMap[timepoint];

  Rebroadcaster::Rebroadcast(assembly, itk::ModifiedEvent()
                             ,this, ValueChangedEvent());
}

void
SegmentationMeshWrapper::UpdateMeshes(itk::Command *progressCmd, unsigned int timepoint)
{
  std::cout << "[SegWrapper] UpdateMeshes" << std::endl;


  if (!m_MeshAssemblyMap.count(timepoint))
    {
    // If assembly not exist yet, create a new assembly
    CreateNewAssembly(timepoint);
    }

  SegmentationMeshAssembly *assembly =
      static_cast<SegmentationMeshAssembly*>(m_MeshAssemblyMap[timepoint].GetPointer());


  auto img = m_ImagePointer->GetImageByTimePoint(timepoint);
  assembly->UpdateMeshAssembly(progressCmd, img, m_MeshOptions);
}
