#include "StandaloneMeshWrapper.h"

StandaloneMeshWrapper::StandaloneMeshWrapper()
{

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
  if (Superclass::IsA(type))
    return true;

  return strcmp("StandaloneMeshWrapper", type) == 0;
}
