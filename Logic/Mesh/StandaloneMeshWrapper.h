#ifndef STANDALONEMESHWRAPPER_H
#define STANDALONEMESHWRAPPER_H

#include "MeshWrapperBase.h"
#include "MeshDisplayMappingPolicy.h"

class StandaloneMeshAssembly : public MeshAssembly
{
public:
  irisITKObjectMacro(StandaloneMeshAssembly, MeshAssembly);

protected:
  StandaloneMeshAssembly() {}
  virtual ~StandaloneMeshAssembly() {}
};

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
  { return m_DisplayMapping.GetPointer(); }
  const AbstractDisplayMappingPolicy *GetDisplayMapping() const override
  { return m_DisplayMapping.GetPointer(); }

  MeshDisplayMappingPolicy *GetMeshDisplayMappingPolicy() const override
  { return m_DisplayMapping.GetPointer(); }

  bool IsMeshDirty(unsigned int timepoint) override;

  void SetMesh(vtkPolyData* mesh, unsigned int timepoint, LabelType id) override;

  bool IsExternalLoadable () const override
  { return true; }

  /** Save the layer to registry */
  virtual void SaveToRegistry(Registry &folder) override;

  /** Build the layer from registry */
  virtual void LoadFromRegistry(Registry &folder, std::string &orig_dir,
                                std::string &crnt_dir, unsigned int nT) override;

  // End of virtual methods definition
  //-------------------------------------------------


protected:
  StandaloneMeshWrapper();
  virtual ~StandaloneMeshWrapper() = default;

  SmartPtr<GenericMeshDisplayMappingPolicy> m_DisplayMapping;
};

#endif // STANDALONEMESHWRAPPER_H
