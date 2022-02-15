#ifndef SEGMENTATIONMESHWRAPPER_H
#define SEGMENTATIONMESHWRAPPER_H

#include "MeshWrapperBase.h"
#include "DisplayMappingPolicy.h"

class SegmentationMeshWrapper : public MeshWrapperBase
{
public:
  irisITKObjectMacro(SegmentationMeshWrapper, MeshWrapperBase)

  //-----------------------------------------------------
  //  Begin virtual methods implementation

  /**
   * Get the display mapping policy. This policy differs from wrapper to wrapper
   * and may involve using color labels or color maps.
   */
  AbstractDisplayMappingPolicy *GetDisplayMapping() override
  { return m_DisplayMapping; }
  const AbstractDisplayMappingPolicy *GetDisplayMapping() const override
  { return m_DisplayMapping; }

  /** Get the mesh display mapping. This saves renderer a type cast */
  MeshDisplayMappingPolicy *GetMeshDisplayMappingPolicy() const override
  { return m_DisplayMapping; }


  /** Check if the mesh requested is the latest, if not then re-compute */
  SmartPtr<MeshAssembly> GetMeshAssembly(unsigned int timepoint) override;

  bool IsMeshDirty(unsigned int timepoint) const override;

  void UpdateMeshes(unsigned int timepoint) override;

  bool IsA(const char *type) const override;

  //  End of virtual methods implementation
  //-----------------------------------------------------

protected:
  SegmentationMeshWrapper();
  virtual ~SegmentationMeshWrapper() = default;

  SmartPtr<MeshDisplayMappingPolicy> m_DisplayMapping;
};

#endif // SEGMENTATIONMESHWRAPPER_H
