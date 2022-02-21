#include "StandaloneMeshWrapper.h"
#include "DisplayMappingPolicy.h"

StandaloneMeshWrapper::StandaloneMeshWrapper()
{
  m_Initialized = true;
}

void
StandaloneMeshWrapper::UpdateMeshes(unsigned int)
{
}

bool
StandaloneMeshWrapper::IsMeshDirty(unsigned int) const
{
  return false;
}

bool
StandaloneMeshWrapper::IsA(const char *type) const
{
  return Superclass::IsA(type) || (strcmp("StandaloneMeshWrapper", type) == 0);
}
