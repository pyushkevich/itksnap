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

struct MeshArrayComponent
{
	MeshArrayComponent() {}

	MeshArrayComponent(const char *_name, double _min, double _max)
		:m_Min(_min), m_Max(_max)
	{

        this->m_Name = new char[strlen(_name) + 1];
        strcpy(this->m_Name, _name);
		this->m_IntensityCurve = IntensityCurveVTK::New();
		this->m_IntensityCurve->Initialize(3);
	}

	~MeshArrayComponent()
	{
        delete[] m_Name;
	}

	MeshArrayComponent(const MeshArrayComponent &other)
	{
        this->m_Name = new char[strlen(other.m_Name) + 1];
		strcpy(this->m_Name, other.m_Name);
		this->m_Min = other.m_Min;
		this->m_Max = other.m_Max;
		if (other.m_IntensityCurve)
			this->m_IntensityCurve = other.m_IntensityCurve;
	}

	MeshArrayComponent &operator=(const MeshArrayComponent &other)
	{
        this->m_Name = new char[strlen(other.m_Name) + 1];
		strcpy(this->m_Name, other.m_Name);
		this->m_Min = other.m_Min;
		this->m_Max = other.m_Max;
		if (other.m_IntensityCurve)
			this->m_IntensityCurve = other.m_IntensityCurve;
		return *this;
	}

	void Print(std::ostream &os)
	{
		os.precision(7);
		os << "MeshArrayComponent ----" << std::endl;
		os << "-- Name: " << m_Name << std::endl;
		os << "-- Range: [" << m_Min << ',' << m_Max << "]" << std::endl;
	}

	char *m_Name;
	double m_Min;
	double m_Max;
	SmartPtr<IntensityCurveVTK> m_IntensityCurve;
};

class AbstractMeshDataArrayProperty : public itk::Object
{
public:
  irisITKAbstractObjectMacro(AbstractMeshDataArrayProperty, itk::Object);

	typedef std::map<vtkIdType, MeshArrayComponent> MeshArrayComponentMap;

	enum MeshDataType { POINT_DATA=0, CELL_DATA, FIELD_DATA, COUNT };

  void Initialize(vtkDataArray *array, MeshDataType type);

  void Update(vtkDataArray *array);

  /** Get type of the property */
  MeshDataType GetType() const;

  /** Get min */
	double GetMin(vtkIdType comp = -1) const;

  /** Get max */
	double GetMax(vtkIdType comp = -1) const;

  /** Get name */
  const char* GetName()
  { return m_Name; }

  void Print(std::ostream &os) const;

	bool IsMultiComponent() const
	{ return m_MeshArrayComponentMap.size() > 1; }

	size_t GetNumberOfComponents() const
	{ return m_MeshArrayComponentMap.size(); }

	MeshArrayComponentMap &GetMeshArrayComponentMap()
	{ return m_MeshArrayComponentMap; }

  static RegistryEnumMap<MeshDataType>& GetMeshDataTypeEnumMap()
  { return m_MeshDataTypeEnumMap; }


protected:
  AbstractMeshDataArrayProperty();
  virtual ~AbstractMeshDataArrayProperty();

  char* m_Name;
	double m_MagMin; // min Magnitude
	double m_MagMax; // max Magnitude

	MeshArrayComponentMap m_MeshArrayComponentMap;

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
	enum VectorMode
	{
		MAGNITUDE = 0,
		RGBCOLOR,
		COMPONENT,
		COUNT
	};

  /** Get Color Map */
  ColorMap* GetColorMap()
  { return m_ColorMap; }

  /** Get Active Intensity Curve depending on active vector mode */
  IntensityCurveVTK* GetActiveIntensityCurve();

	/** Set Intensity Curve */
  void SetActiveIntensityCurve(IntensityCurveVTK * curve);

  /**
    Compute the array histogram. The histogram is cached inside of the
    object, so repeated calls to this function with the same nBins parameter
    will not require additional computation.

    Calling with default parameter (0) will use the same number of bins that
    is currently in the histogram (i.e., return/recompute current histogram).
    If there is no current histogram, a default histogram with 128 entries
    will be generated.
    */
	ScalarImageHistogram* GetHistogram(size_t nBins);

  /** Merge another property into this. Adjusting min/max etc. */
  void Merge(MeshDataArrayProperty *other);

	VectorMode GetActiveVectorMode()
  { return m_ActiveVectorMode; }

	void SetActiveVectorMode(int mode, vtkIdType compId = -1);

	vtkIdType GetActiveComponentId()
  {
		return m_ActiveComponentId;
  }

	MeshArrayComponent &GetActiveComponent()
	{
		return m_MeshArrayComponentMap.at(m_ActiveComponentId);
	}

	MeshArrayComponent &GetComponent(vtkIdType id)
	{
		assert(m_MeshArrayComponentMap.count(id));
		return m_MeshArrayComponentMap.at(id);
	}

  /** Deep copy self to the other object */
  void Initialize(MeshDataArrayProperty *other);

protected:
  MeshLayerDataArrayProperty();
  ~MeshLayerDataArrayProperty() {};

  SmartPtr<ColorMap> m_ColorMap;

	// Stores intensity curve for magnitude vector mode
	SmartPtr<IntensityCurveVTK> m_MagnitudeIntensityCurve;

	// Current active intensity curve, for magnitude or specific component
	SmartPtr<IntensityCurveVTK> m_ActiveIntensityCurve;
  SmartPtr<HistogramFilterType> m_HistogramFilter;
  SmartPtr<MinMaxFilterType> m_MinMaxFilter;
  std::list<vtkDataArray*> m_DataPointerList;

  // Active Vector Mode
	VectorMode m_ActiveVectorMode = VectorMode::MAGNITUDE;

	// Active Component
	vtkIdType m_ActiveComponentId = 0;
};

#endif // MESHDATAARRAYPROPERTY_H
