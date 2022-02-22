#ifndef STANDALONEMESHWRAPPER_H
#define STANDALONEMESHWRAPPER_H

#include "MeshWrapperBase.h"
#include "MeshDisplayMappingPolicy.h"

class StandaloneMeshWrapper : public MeshWrapperBase
{
public:
  irisITKObjectMacro(StandaloneMeshWrapper, MeshWrapperBase)

  //-------------------------------------------------
  // Begin virtual methods definition

  /**
   * Get the display mapping policy. This policy differs from wrapper to wrapper
   * and may involve using color labels or color maps.
   */
  AbstractDisplayMappingPolicy *GetDisplayMapping() override
  { return m_DisplayMapping; }
  const AbstractDisplayMappingPolicy *GetDisplayMapping() const override
  { return m_DisplayMapping; }

  MeshDisplayMappingPolicy *GetMeshDisplayMappingPolicy() const override
  { return m_DisplayMapping; }


  void UpdateMeshes(unsigned int timepoint) override;

  bool IsMeshDirty(unsigned int timepoint) const override;

  bool IsA(const char *type) const override;


  // End of virtual methods definition
  //-------------------------------------------------





protected:
  StandaloneMeshWrapper();
  virtual ~StandaloneMeshWrapper() = default;
};

#endif // STANDALONEMESHWRAPPER_H
