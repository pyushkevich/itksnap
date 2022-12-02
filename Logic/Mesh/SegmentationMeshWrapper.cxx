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
  // Get the image from current tp and feed the pipeline
  m_Pipeline->SetImage(img);
  m_Pipeline->SetMeshOptions(options);

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

  // Set Default nickname
  std::ostringstream oss;
	oss << m_NicknamePrefix << segImg->GetNickname();
  SetDefaultNickname(oss.str());

	auto metaCallback = SegmentationMetadataUpdateCallback::New();
	metaCallback->Initialize(this);

	segImg->AddObserver(WrapperMetadataChangeEvent(), metaCallback);

  // Configure display mapping policy
  m_DisplayMapping = LabelMeshDisplayMappingPolicy::New();
  m_DisplayMapping->SetMesh(this);
  m_DisplayMapping->SetColorLabelTable(segImg->GetDisplayMapping()->GetLabelColorTable());

  m_Initialized = true;
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
    auto optionMTime = m_MeshOptions->GetMTime();

    if (imgMTime > pipeMTime || optionMTime >= pipeMTime)
      return true;
    }
  else
    return true;

  return false;
}

void
SegmentationMeshWrapper::
CreateNewAssembly(unsigned int timepoint)
{
  m_MeshAssemblyMap[timepoint] = SegmentationMeshAssembly::New().GetPointer();

  MeshAssembly *assembly = m_MeshAssemblyMap[timepoint];

  Rebroadcaster::Rebroadcast(assembly, itk::ModifiedEvent()
                             ,this, ValueChangedEvent());
}

void
SegmentationMeshWrapper::UpdateMeshes(itk::Command *progressCmd, unsigned int timepoint)
{
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

void
SegmentationMeshWrapper
::SaveToRegistry(Registry &)
{

}

void
SegmentationMeshWrapper
::LoadFromRegistry(Registry &, std::string &, std::string &)
{

}

unsigned long
SegmentationMeshWrapper
::GetAssemblyMTime(unsigned int tp)
{
  auto assembly = GetMeshAssembly(tp);
  if (!assembly)
    return 0;

  auto segAssembly = static_cast<SegmentationMeshAssembly*>(assembly);
  return segAssembly->GetMTime();
}
