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

  void SetMesh(vtkPolyData *mesh, unsigned int timepoint, LabelType id) override;

  /** Save the layer to registry */
  virtual void SaveToRegistry(Registry &folder) override;

  /** Build the layer from registry */
  virtual void LoadFromRegistry(Registry &folder, std::string &orig_dir, std::string &crnt_dir) override;

  //  End of virtual methods implementation
  //-----------------------------------------------------

  void UpdateMeshes(itk::Command *progressCmd, unsigned int timepoint);

  void Initialize(LabelImageWrapper *segImg, MeshOptions* meshOptions);

  /** Add a new blank segmentation mesh assembly to the assembly map*/
  void CreateNewAssembly(unsigned int timepoint);

	const char* GetNicknamePrefix() const
	{ return m_NicknamePrefix; }

  unsigned long GetAssemblyMTime(unsigned int tp);

protected:
  SegmentationMeshWrapper();
  virtual ~SegmentationMeshWrapper() = default;

  SmartPtr<LabelMeshDisplayMappingPolicy> m_DisplayMapping;

  SmartPtr<LabelImageWrapper> m_ImagePointer;

  SmartPtr<MeshOptions> m_MeshOptions;

	const char* m_NicknamePrefix = "Mesh-";
};

/**
 * @brief The SegmentationMetadataUpdateCallback class
 * Observe the metadata event from the caller, and update the mesh layer accordingly
 */
class SegmentationMetadataUpdateCallback : public itk::Command
{
public:
	irisITKObjectMacro(SegmentationMetadataUpdateCallback, itk::Command)

	void Execute(itk::Object * caller, const itk::EventObject &) override
	{
		assert(m_SegMesh); // If failed, you forgot to call the Initialize method

		LabelImageWrapper * liw = dynamic_cast<LabelImageWrapper*>(caller);
		if (liw)
			UpdateMeshFromImage(liw);
	}

	void Execute(const itk::Object * , const itk::EventObject & ) override {}

	void Initialize(SegmentationMeshWrapper *segMesh)
	{ m_SegMesh = segMesh; }

protected:
	SegmentationMetadataUpdateCallback() {}
	virtual ~SegmentationMetadataUpdateCallback() {}

private:
	void UpdateMeshFromImage(LabelImageWrapper *liw)
	{
		std::ostringstream oss;
		oss << m_SegMesh->GetNicknamePrefix() << liw->GetNickname();
		m_SegMesh->SetCustomNickname(oss.str());
	}

	SmartPtr<SegmentationMeshWrapper> m_SegMesh;
};

#endif // SEGMENTATIONMESHWRAPPER_H
