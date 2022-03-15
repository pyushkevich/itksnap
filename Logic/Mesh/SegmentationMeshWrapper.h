#ifndef SEGMENTATIONMESHWRAPPER_H
#define SEGMENTATIONMESHWRAPPER_H

#include "MeshWrapperBase.h"
#include "MeshDisplayMappingPolicy.h"
#include "MultiLabelMeshPipeline.h"
#include "MeshOptions.h"
#include "LabelImageWrapper.h"


class SegmentationMeshAssembly : public MeshAssembly
{
public:
  irisITKObjectMacro(SegmentationMeshAssembly, MeshAssembly);

  typedef LabelImageWrapper::ImagePointer ImagePointer;

  MultiLabelMeshPipeline *GetPipeline();

  void UpdateMeshAssembly(itk::Command *progress, ImagePointer img, MeshOptions *options);

protected:
  SegmentationMeshAssembly();
  virtual ~SegmentationMeshAssembly();

  SmartPtr<MultiLabelMeshPipeline> m_Pipeline;
};

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
  { return m_DisplayMapping.GetPointer(); }
  const AbstractDisplayMappingPolicy *GetDisplayMapping() const override
  { return m_DisplayMapping.GetPointer(); }

  /** Get the mesh display mapping. This saves renderer a type cast */
  MeshDisplayMappingPolicy *GetMeshDisplayMappingPolicy() const override
  { return m_DisplayMapping.GetPointer(); }

  bool IsMeshDirty(unsigned int timepoint) override;

  bool IsA(const char *type) const override;

  void SetMesh(vtkPolyData *mesh, unsigned int timepoint, LabelType id) override;

  //  End of virtual methods implementation
  //-----------------------------------------------------

  void UpdateMeshes(itk::Command *progressCmd, unsigned int timepoint);

  void Initialize(LabelImageWrapper *segImg, MeshOptions* meshOptions);

  /** Add a new blank segmentation mesh assembly to the assembly map*/
  void CreateNewAssembly(unsigned int timepoint);

protected:
  SegmentationMeshWrapper();
  virtual ~SegmentationMeshWrapper() = default;

  SmartPtr<LabelMeshDisplayMappingPolicy> m_DisplayMapping;

  SmartPtr<LabelImageWrapper> m_ImagePointer;

  SmartPtr<MeshOptions> m_MeshOptions;
};

#endif // SEGMENTATIONMESHWRAPPER_H
