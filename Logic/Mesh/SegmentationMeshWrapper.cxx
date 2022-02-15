#include "SegmentationMeshWrapper.h"
#include "MeshWrapperBase.h"

SegmentationMeshWrapper::SegmentationMeshWrapper()
{
  m_DisplayMapping = MeshDisplayMappingPolicy::New();
  m_DisplayMapping->SetMesh(this);
  m_Initialized = true;
}

bool
SegmentationMeshWrapper::IsMeshDirty(unsigned int timepoint) const
{
  assert(timepoint);
  return false;
}

void
SegmentationMeshWrapper::UpdateMeshes(unsigned int timepoint)
{
  assert(timepoint);
}

bool
SegmentationMeshWrapper::IsA(const char *type) const
{
  return Superclass::IsA(type) || (strcmp("SegmentationMeshWrapper", type) == 0);
}

SmartPtr<MeshAssembly>
SegmentationMeshWrapper::GetMeshAssembly(unsigned int timepoint)
{
}
