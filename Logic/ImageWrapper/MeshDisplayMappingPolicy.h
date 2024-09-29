#ifndef MESHDISPLAYMAPPINGPOLICY_H
#define MESHDISPLAYMAPPINGPOLICY_H

#include "DisplayMappingPolicy.h"
#include "ColorLabelTable.h"
#include "itkCommand.h"

class vtkScalarBarActor;
class vtkLookupTable;
class vtkActor;
class ActorPool;

/**
 * @brief The parent class for policies that involve mesh display mapping
 */

class MeshDisplayMappingPolicy : public AbstractContinuousImageDisplayMappingPolicy
{
public:
  irisITKAbstractObjectMacro(MeshDisplayMappingPolicy, AbstractContinuousImageDisplayMappingPolicy)

  typedef MeshDataArrayProperty::MeshDataType MeshDataType;
	typedef MeshLayerDataArrayProperty::VectorMode VectorMode;
  typedef Superclass::TDigest TDigest;

  //--------------------------------------------
  // virtual methods implementation

  virtual IntensityCurveInterface *GetIntensityCurve() const override;

  virtual ColorMap *GetColorMap() const override;

  /**
   * Mesh wrapper does not generate display slices
   * Always returns nullptr
   */
  virtual DisplaySlicePointer GetDisplaySlice(unsigned int slice) override;

  virtual void Save(Registry &folder) override;

  virtual void Restore(Registry &folder) override;

  /**
   * @brief Get the intensity range relative to which the contrast mapping
   * curve is constructed. This is primarily used when displaying the curve
   * to the user.
   * @return Vector containing min/max of the curve range (in native units)
   */
  virtual Vector2d GetNativeImageRangeForCurve() override;

  virtual void SetColorMap(ColorMap *map) override;

  /** Configure actor */
	virtual void UpdateAppearance(ActorPool *pool, unsigned int timepoint) = 0;

  /** Configure legend scalar bar */
  virtual void ConfigureLegend(vtkScalarBarActor *legend) = 0;

  /** Build m_LookupTable based on the active array property */
  virtual void UpdateLUT() = 0;

  /** Update actor map with meshes from given timepoint */
  virtual void UpdateActorMap(ActorPool* pool, unsigned int timepoint);

  /** Get the TDigest for the active array (i.e., histogram) */
  virtual TDigest *GetTDigest() override;

  // end of virtual methods implementation
  //--------------------------------------------

  /** Set mesh layer for the policy. */
  // This method intentionally hides parent's Initialize method
  void SetMesh(MeshWrapperBase *mesh_wrapper);

  /** Set intensity curve */
  void SetIntensityCurve(IntensityCurveVTK *curve);

  /** Get Lookup Table */
  vtkLookupTable *GetLookupTable();

  MeshWrapperBase *GetMeshLayer();

protected:
  MeshDisplayMappingPolicy();
  virtual ~MeshDisplayMappingPolicy() = default;

  MeshWrapperBase *m_Wrapper;

  vtkSmartPointer<vtkLookupTable> m_LookupTable;

  SmartPtr<ColorMap> m_ColorMap;

  SmartPtr<IntensityCurveVTK> m_IntensityCurve;

  bool m_Initialized = false;

	unsigned long m_ColorMapObserverTag, m_IntensityCurveObserverTag;
};

/**
 * @brief The GenericMeshDisplayMappingPolicy class
 * Display mapping policy for general meshes that have multiple data arrays stored
 * and rendered
 */
class GenericMeshDisplayMappingPolicy : public MeshDisplayMappingPolicy
{
public:
  irisITKObjectMacro(GenericMeshDisplayMappingPolicy, MeshDisplayMappingPolicy)

  //--------------------------------------------
  // virtual methods implementation

  /** Configure mapper */
	virtual void UpdateAppearance(ActorPool *pool, unsigned int timepoint) override;

  /** Configure legend scalar bar */
  virtual void ConfigureLegend(vtkScalarBarActor *legend) override;

  /** Build m_LookupTable based on the active array property */
  virtual void UpdateLUT() override;

  // end of virtual methods implementation
	//--------------------------------------------
protected:
  GenericMeshDisplayMappingPolicy();
  virtual ~GenericMeshDisplayMappingPolicy();
};

/**
 * @brief The LabelMeshDisplayMappingPolicy class
 * Display mapping policy for meshes with id as label type for coloring
 * and need to build discrete legend based on the color label table
 */
class LabelMeshDisplayMappingPolicy : public MeshDisplayMappingPolicy
{
public:
  irisITKObjectMacro(LabelMeshDisplayMappingPolicy, MeshDisplayMappingPolicy)

  //--------------------------------------------
  // virtual methods implementation

  /** Configure mapper */
	virtual void UpdateAppearance(ActorPool *pool, unsigned int timepoint) override;

  /** Configure legend scalar bar */
  virtual void ConfigureLegend(vtkScalarBarActor *legend) override;

  /** Build m_LookupTable based on the active array property */
  virtual void UpdateLUT() override;

	/** Return null for LabelMesh */
	virtual ColorMap *GetColorMap() const override
	{ return nullptr; }

  // end of virtual methods implementation
  //--------------------------------------------

  /** Set color label table for label color rendering */
  void SetColorLabelTable(ColorLabelTable* labelTable);

  /** Get the color label table */
  ColorLabelTable *GetColorLabelTable()
  { return m_ColorLabelTable; }


protected:
  LabelMeshDisplayMappingPolicy();
  virtual ~LabelMeshDisplayMappingPolicy();

  SmartPtr<ColorLabelTable> m_ColorLabelTable;
};



#endif // MESHDISPLAYMAPPINGPOLICY_H
