#include "StandaloneMeshWrapper.h"
#include "DisplayMappingPolicy.h"

StandaloneMeshWrapper::StandaloneMeshWrapper()
{
  m_DisplayMapping = MeshDisplayMappingPolicy::New();
  m_DisplayMapping->Initialize(this);
}

void
StandaloneMeshWrapper::UpdateMeshes(unsigned int timepoint)
{
  assert(timepoint);
}

bool
StandaloneMeshWrapper::IsMeshDirty(unsigned int timepoint) const
{
  assert(timepoint);
  return false;
}

bool
StandaloneMeshWrapper::IsA(const char *type) const
{
  return Superclass::IsA(type) || (strcmp("StandaloneMeshWrapper", type) == 0);
}
