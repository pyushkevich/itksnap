#include "SegmentationMeshWrapper.h"

SegmentationMeshWrapper::SegmentationMeshWrapper()
{

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
  if (Superclass::IsA(type))
    return true;

  return strcmp("SegmentationMeshWrapper", type) == 0;
}
