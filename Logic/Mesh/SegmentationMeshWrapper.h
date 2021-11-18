#ifndef SEGMENTATIONMESHWRAPPER_H
#define SEGMENTATIONMESHWRAPPER_H

#include "MeshWrapperBase.h"

class SegmentationMeshWrapper : public MeshWrapperBase
{
public:
  irisITKObjectMacro(SegmentationMeshWrapper, MeshWrapperBase)

  /** Check if the mesh requested is the latest, if not then re-compute */
  MeshCollection GetMeshCollection(unsigned int timepoint) override;

  bool IsMeshDirty(unsigned int timepoint) const override;

  void UpdateMeshes(unsigned int timepoint) override;

  bool IsA(const char *type) const override;

protected:
  SegmentationMeshWrapper();
  virtual ~SegmentationMeshWrapper() = default;
};

#endif // SEGMENTATIONMESHWRAPPER_H
