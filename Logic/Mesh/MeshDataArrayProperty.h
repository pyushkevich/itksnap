#ifndef MESHDATAARRAYPROPERTY_H
#define MESHDATAARRAYPROPERTY_H

#include "SNAPCommon.h"
#include "IntensityCurveVTK.h"
#include "ImageWrapperBase.h"
#include "itkObject.h"
#include "itkObjectFactory.h"
#include "itkMinimumMaximumImageFilter.h"
#include "vtkDataArray.h"
#include "vtkSmartPointer.h"
#include "vtkScalarsToColors.h"
#include "Registry.h"

template<class TIn> class ThreadedHistogramImageFilter;

class AbstractMeshDataArrayProperty : public itk::Object
{
public:
  irisITKAbstractObjectMacro(AbstractMeshDataArrayProperty, itk::Object);

  typedef std::map<vtkIdType, std::string> ComponentNameMap;

	enum MeshDataType { POINT_DATA=0, CELL_DATA, FIELD_DATA, COUNT };

  void Initialize(vtkDataArray *array, MeshDataType type);

  void Update(vtkDataArray *array);

  /** Get type of the property */
  MeshDataType GetType() const;

  /** Get min */
  double GetMin() const { return m_min; }

  /** Get max */
  double GetMax() const { return m_max; }

  /** Get name */
  const char* GetName()
  { return m_Name; }

  void Print(std::ostream &os) const;

  bool IsMultiComponent()
  { return m_ComponentNameMap.size() > 1; }

  ComponentNameMap &GetComponentNameMap()
  { return m_ComponentNameMap; }

  static RegistryEnumMap<MeshDataType>& GetMeshDataTypeEnumMap()
  { return m_MeshDataTypeEnumMap; }


protected:
  AbstractMeshDataArrayProperty();
  virtual ~AbstractMeshDataArrayProperty();

  char* m_Name;
  double m_min;
  double m_max;

  ComponentNameMap m_ComponentNameMap;

  MeshDataType m_Type;

  static RegistryEnumMap<MeshDataType> m_MeshDataTypeEnumMap;
};

/**
 * @brief The MeshDataArrayProperty class
 * Stores element-level data array property. Has one-to-one relationship to
 * a vtkDataArray.
 */
class MeshDataArrayProperty : public AbstractMeshDataArrayProperty
{
public:
  irisITKObjectMacro(MeshDataArrayProperty, AbstractMeshDataArrayProperty);

  void SetDataPointer(vtkDataArray* pointer);
  vtkDataArray* GetDataPointer()
  { return m_DataPointer; }

protected:
  MeshDataArrayProperty();
  ~MeshDataArrayProperty();

  vtkSmartPointer<vtkDataArray> m_DataPointer;

};

/**
 * @brief The MeshLayerDataArrayProperty class
 * Stores layer-level data array property. It should only be used at
 * MeshWrapperBase (layer) level. It merges mesh assembly lowest level
 * properties into one with a ColorMap and Intensity Curve; It provides
 * the UI ability to set layer-specific display mapping policy for the
 * data arrays of all the mesh elements with same type and name.
 */
class MeshLayerDataArrayProperty : public AbstractMeshDataArrayProperty
{
public:
  irisITKObjectMacro(MeshLayerDataArrayProperty, AbstractMeshDataArrayProperty)

  typedef itk::Image<double, 1> DataArrayImageType;
  typedef ThreadedHistogramImageFilter<DataArrayImageType> HistogramFilterType;
  typedef itk::MinimumMaximumImageFilter<DataArrayImageType> MinMaxFilterType;

  /** Multi-Component (Vector) Display Mode */
  typedef vtkScalarsToColors::VectorModes VectorModes;
  typedef std::map<int, std::string> VectorModeNameMap;


  /** Get Color Map */
  ColorMap* GetColorMap()
  { return m_ColorMap; }

  /** Get Intensity Curve */
  IntensityCurveVTK* GetIntensityCurve()
  { return m_IntensityCurve; }

  /**
    Compute the array histogram. The histogram is cached inside of the
    object, so repeated calls to this function with the same nBins parameter
    will not require additional computation.

    Calling with default parameter (0) will use the same number of bins that
    is currently in the histogram (i.e., return/recompute current histogram).
    If there is no current histogram, a default histogram with 128 entries
    will be generated.
    */
  ScalarImageHistogram* GetHistogram(size_t nBins) const;

  /** Merge another property into this. Adjusting min/max etc. */
  void Merge(MeshDataArrayProperty *other);

  VectorModeNameMap &GetVectorModeNameMap()
  { return m_VectorModeNameMap; }

  int GetActiveVectorMode()
  { return m_ActiveVectorMode; }

  void SetActiveVectorMode(int mode);

  int GetActiveComponentId()
  {
    return (m_ActiveVectorMode > 1 ? m_ActiveVectorMode - m_VectorModeShiftSize : 0);
  }

  /** Deep copy self to the other object */
  void Initialize(MeshDataArrayProperty *other);

protected:
  MeshLayerDataArrayProperty();
  ~MeshLayerDataArrayProperty() {};

  void UpdateVectorModeNameMap();

  SmartPtr<ColorMap> m_ColorMap;
  SmartPtr<IntensityCurveVTK> m_IntensityCurve;
  SmartPtr<HistogramFilterType> m_HistogramFilter;
  SmartPtr<MinMaxFilterType> m_MinMaxFilter;
  std::list<vtkDataArray*> m_DataPointerList;

  // Active Vector Mode
  int m_ActiveVectorMode = VectorModes::MAGNITUDE;

  // The number of VectorModes that are not component-specific
  const int m_VectorModeShiftSize = 2;

  /** A hybrid map consists of both non-component-specific VectorModes
   *  and all single components that can be rendered independently.
   *  The first M entries are always non-component-specific, where M
   *  is defined by the number of supported non-component-specific VectorModes
   *  stored in the const m_VectorModeShiftSize. All component-specific mode
   *  are stored with their index shifted by M.
   */
  VectorModeNameMap m_VectorModeNameMap;
};

#endif // MESHDATAARRAYPROPERTY_H
