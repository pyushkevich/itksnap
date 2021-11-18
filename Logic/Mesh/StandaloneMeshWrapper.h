#ifndef STANDALONEMESHWRAPPER_H
#define STANDALONEMESHWRAPPER_H

#include "MeshWrapperBase.h"

class StandaloneMeshWrapper : public MeshWrapperBase
{
public:
  irisITKObjectMacro(StandaloneMeshWrapper, MeshWrapperBase)

  void UpdateMeshes(unsigned int timepoint) override;

  bool IsMeshDirty(unsigned int timepoint) const override;

  bool IsA(const char *type) const override;

protected:
  StandaloneMeshWrapper();
  virtual ~StandaloneMeshWrapper() = default;
};

#endif // STANDALONEMESHWRAPPER_H
