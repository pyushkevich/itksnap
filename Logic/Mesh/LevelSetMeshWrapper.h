#ifndef LEVELSETMESHWRAPPER_H
#define LEVELSETMESHWRAPPER_H

#include "MeshWrapperBase.h"
#include "MeshDisplayMappingPolicy.h"
#include "MultiLabelMeshPipeline.h"
#include "MeshOptions.h"
#include "LevelSetMeshPipeline.h"

class LevelSetMeshAssembly : public MeshAssembly
{
public:
  irisITKObjectMacro(LevelSetMeshAssembly, MeshAssembly);

  typedef LevelSetMeshPipeline::InputImageType InputImageType;
  typedef InputImageType::Pointer InputImagePointer;

  LevelSetMeshPipeline *GetPipeline();

  void UpdateMeshAssembly(LabelType id, std::mutex *mutex = nullptr);

  void SetMeshOptions(const MeshOptions *options);

  void SetImage(InputImageType *image);

  bool IsAssemblyDirty() const;

protected:
  LevelSetMeshAssembly();
  virtual ~LevelSetMeshAssembly();

  const MeshOptions *m_MeshOptions;

  SmartPtr<LevelSetMeshPipeline> m_Pipeline;

  InputImagePointer m_Image;
};


class LevelSetMeshWrapper : public MeshWrapperBase
{
public:
  irisITKObjectMacro(LevelSetMeshWrapper, MeshWrapperBase)

  //-----------------------------------------------------
  //  Begin virtual methods implementation

  /**
   * Get the display mapping policy. This policy differs from wrapper to wrapper
   * and may involve using color labels or color maps.
   */
  AbstractDisplayMappingPolicy *GetDisplayMapping() override
  { return m_DisplayMapping.GetPointer(); }
  const AbstractDisplayMappingPolicy *GetDisplayMapping() const override
  { return m_DisplayMapping.GetPointer(); }

  /** Get the mesh display mapping. This saves renderer a type cast */
  MeshDisplayMappingPolicy *GetMeshDisplayMappingPolicy() const override
  { return m_DisplayMapping.GetPointer(); }

  bool IsMeshDirty(unsigned int timepoint) override;

  void SetMesh(vtkPolyData *mesh, unsigned int timepoint, LabelType id) override;

  /** Save the layer to registry */
  virtual void SaveToRegistry(Registry &folder) override;

  /** Build the layer from registry */
  virtual void LoadFromRegistry(Registry &folder, std::string &orig_dir,
                                std::string &crnt_dir, unsigned int nT) override;

  /** Get Pipeline Modified Time */
  unsigned long GetAssemblyMTime(unsigned int tp);
  //  End of virtual methods implementation
  //-----------------------------------------------------

  // Layer level method should always handle timepoint
  void UpdateMeshes(LevelSetImageWrapper *lsImg, unsigned int timepoint, LabelType id, std::mutex *mutex);

  void Initialize(MeshOptions* meshOptions, ColorLabelTable *colorTable);

protected:
  LevelSetMeshWrapper();
  virtual ~LevelSetMeshWrapper() = default;

  void CreateNewAssembly(LevelSetImageWrapper *lsImg, unsigned int timepoint);

  SmartPtr<LabelMeshDisplayMappingPolicy> m_DisplayMapping;

  SmartPtr<MeshOptions> m_MeshOptions;

};

#endif // LEVELSETMESHWRAPPER_H
