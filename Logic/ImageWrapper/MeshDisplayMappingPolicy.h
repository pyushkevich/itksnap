#ifndef MESHDISPLAYMAPPINGPOLICY_H
#define MESHDISPLAYMAPPINGPOLICY_H

#include "DisplayMappingPolicy.h"

class vtkPolyDataMapper;
class vtkScalarBarActor;
class vtkLookupTable;

/**
 * @brief The parent class for policies that involve mesh display mapping
 */

class MeshDisplayMappingPolicy : public AbstractContinuousImageDisplayMappingPolicy
{
public:
  irisITKObjectMacro(MeshDisplayMappingPolicy, AbstractContinuousImageDisplayMappingPolicy)

  typedef MeshDataArrayProperty::MeshDataType MeshDataType;

  //--------------------------------------------
  // virtual methods implementation

  virtual IntensityCurveInterface *GetIntensityCurve() const override;

  virtual ColorMap *GetColorMap() const override;

  /**
   * @brief Mesh wrapper does not generate display slices for now, the method will
   * @return An exception will be raised and a nullptr will be returned
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

  /**
   * @brief Get the histogram associated with the current state of the display
   * policy. For single-component layers, this method just returns the
   * component's histogram. For multi-component layers, it may return the
   * pooled histogram, e.g., when the display is in RGB mode
   * @param nBins Number of bins desired in the histogram
   */
  virtual ScalarImageHistogram *GetHistogram(int nBins) override;

  virtual void SetColorMap(ColorMap *map) override;

  /** Configure mapper */
  virtual void ConfigurePolyDataMapper(vtkPolyDataMapper *mapper);

  /** Configure legend scalar bar */
  virtual void ConfigureLegend(vtkScalarBarActor *legend);

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

  // Build m_LookupTable based on the active array property
  void UpdateLUT();
};

#endif // MESHDISPLAYMAPPINGPOLICY_H
