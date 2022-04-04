#ifndef MESHWRAPPERBASE_H
#define MESHWRAPPERBASE_H

#include "SNAPCommon.h"
#include "itkObject.h"
#include "itkObjectFactory.h"
#include "vtkSmartPointer.h"
#include "ImageWrapperBase.h"
#include "IntensityCurveVTK.h"
#include "ColorMap.h"
#include "ThreadedHistogramImageFilter.h"
#include "vtkPolyData.h"
#include "vtkDataArray.h"
#include "vtkScalarsToColors.h"
#include "itkMinimumMaximumImageFilter.h"

class AbstractMeshIODelegate;
class MeshDisplayMappingPolicy;
class MeshAssembly;
class vtkDataSetAttributes;
template<class TIn> class ThreadedHistogramImageFilter;

class AbstractMeshDataArrayProperty : public itk::Object
{
public:
  irisITKAbstractObjectMacro(AbstractMeshDataArrayProperty, itk::Object);

  typedef std::map<vtkIdType, std::string> ComponentNameMap;

  enum MeshDataType { POINT_DATA=0, CELL_DATA, COUNT };

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


protected:
  AbstractMeshDataArrayProperty();
  virtual ~AbstractMeshDataArrayProperty();

  char* m_Name;
  double m_min;
  double m_max;

  ComponentNameMap m_ComponentNameMap;

  MeshDataType m_Type;
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

  /** Deep copy self to the other object */
  void Initialize(MeshDataArrayProperty *other);

protected:
  MeshLayerDataArrayProperty();
  ~MeshLayerDataArrayProperty() {};

  SmartPtr<ColorMap> m_ColorMap;
  SmartPtr<IntensityCurveVTK> m_IntensityCurve;
  SmartPtr<HistogramFilterType> m_HistogramFilter;
  SmartPtr<MinMaxFilterType> m_MinMaxFilter;
  std::list<vtkDataArray*> m_DataPointerList;


};

/**
 * @brief The PolyDataWrapper class
 */
class PolyDataWrapper : public itk::Object
{
public:
  irisITKObjectMacro(PolyDataWrapper, itk::Object);

  // comparisonn functor for map with char* key
  struct strcmp
  {
     bool operator()(char const *a, char const *b) const
     {
        return std::strcmp(a, b) < 0;
     }
  };

  /** Mesh Data Property Map */
  typedef std::map<const char*, SmartPtr<MeshDataArrayProperty>, strcmp> MeshDataArrayPropertyMap;

  /** Mesh Data Type */
  typedef MeshDataArrayProperty::MeshDataType MeshDataType;

  void SetPolyData(vtkPolyData *polydata);

  vtkPolyData *GetPolyData();

  MeshDataArrayPropertyMap &GetPointDataProperties()
  { return m_PointDataProperties; }

  MeshDataArrayPropertyMap &GetCellDataProperties()
  { return m_CellDataProperties; }

  friend class MeshDataArrayProperty;
protected:
  PolyDataWrapper() {}
  virtual ~PolyDataWrapper() {}

  // Update point data and cell data properties
  void UpdateDataArrayProperties();

  // Helper method for iterating data arrays in polydata
  void UpdatePropertiesFromVTKData(MeshDataArrayPropertyMap &propMap,
                                   vtkDataSetAttributes *data,
                                   MeshDataType type) const;

  // The actual storage of a poly data object
  vtkSmartPointer<vtkPolyData> m_PolyData;

  // Point Data Properties
  MeshDataArrayPropertyMap m_PointDataProperties;

  // Cell Data Properties
  MeshDataArrayPropertyMap m_CellDataProperties;
};


/**
 *  \class MeshAssembly
 *  \brief An abstract class representing a scene assembled by multiple meshes
 *
 *  The class wraps around a MeshMap, which maps a collection of vtkPolyData
 *  , identified by ids (id is in LabelType).
 */

class MeshAssembly : public itk::Object
{
public:
  irisITKObjectMacro(MeshAssembly, itk::Object);

  typedef std::map<LabelType, SmartPtr<PolyDataWrapper>> MeshAssemblyMap;

  /** Add a mesh to the map. If the id already exist, old mesh will be overwritten */
  void AddMesh(PolyDataWrapper *mesh, LabelType id=0u);

  /** Get the mesh by id */
  PolyDataWrapper *GetMesh(LabelType id);

  /** Check existence of id */
  bool Exist(LabelType id);

  /** Erase an entry, and take care of the deletion of data */
  void Erase(LabelType id);

  /** Get the beginning iterator to the map */
  MeshAssemblyMap::const_iterator cbegin()
  { return m_Meshes.cbegin(); }

  /** Get the end of iterator of the map */
  MeshAssemblyMap::const_iterator cend()
  { return m_Meshes.cend(); }

  MeshAssemblyMap::size_type size()
  { return m_Meshes.size(); }

protected:
  MeshAssembly() {}
  virtual ~MeshAssembly() {}

  // Map storing all the meshes in the assembly
  MeshAssemblyMap m_Meshes;
};

class StandaloneMeshAssembly : public MeshAssembly
{
public:
  irisITKObjectMacro(StandaloneMeshAssembly, MeshAssembly);

protected:
  StandaloneMeshAssembly() {}
  virtual ~StandaloneMeshAssembly() {}
};


/**
 *  \class MeshWrapperBase
 *  \brief An abstract class representing a mesh layer in ITK-SNAP
 *
 *  The class wraps around a MeshAssemblyMap, which maps MeshAssemblies, representing
 *  a scene assembled by meshes, to time points.
 *
 *  MeshWrappers are initialized by MeshIODelegates, store point/cell data and appearance
 *  properties, and can configure actors when called by renderers.
 *
 *  The class is intend to replace the MeshManager class that was solely focused on the
 *  segmentation based meshes.
 */

class MeshWrapperBase : public WrapperBase
{
public:
  irisITKAbstractObjectMacro(MeshWrapperBase, WrapperBase)

  typedef std::map<std::string, SmartPtr<itk::Object>> UserDataMapType;

  /** Mesh assembly map maps MeshAssembly to time points */
  typedef std::map<unsigned int, SmartPtr<MeshAssembly>> MeshAssemblyMapType;

  /** Maps Layer-Level Data Array Properties with unique ids */
  typedef std::map<int, SmartPtr<MeshLayerDataArrayProperty>> MeshLayerCombinedPropertyMap;

  /** Maps Layer-Level Data Array Properties with name */
  typedef std::map<const char*,
    SmartPtr<MeshLayerDataArrayProperty>, PolyDataWrapper::strcmp> MeshLayerDataArrayPropertyMap;

  /** Element level Data Array properties with name */
  typedef PolyDataWrapper::MeshDataArrayPropertyMap MeshDataArrayPropertyMap;

  /** Multi-Component (Vector) Display Mode */
  typedef vtkScalarsToColors::VectorModes VectorModes;
  typedef std::map<VectorModes, std::string> VectorModeNameMap;

  //----------------------------------------------
  // Begin virtual methods definition

  /** Access the filename */
  irisGetStringMacroWithOverride(FileName)
  virtual void SetFileName(const std::string &name) override;

  /**
   * Access the nickname - which may be a custom nickname or derived from the
   * filename if there is no custom nickname
   */
  virtual const std::string &GetNickname() const override;

  // Set the custom nickname - precedence over the filename
  virtual void SetCustomNickname(const std::string &nickname) override;
  irisGetMacroWithOverride(CustomNickname, const std::string &);

  /** Fallback nickname - shown if no filename and no custom nickname set. */
  irisGetSetMacroWithOverride(DefaultNickname, const std::string &)

  /** List of tags assigned to the image layer */
  const TagList &GetTags() const override
  { return m_Tags; }
  void SetTags (const TagList &tags) override
  { m_Tags = tags; }

  /** Layer transparency */
  irisSetWithEventMacroWithOverride(Alpha, double, WrapperDisplayMappingChangeEvent)
  irisGetMacroWithOverride(Alpha, double)

  /**
   * Is the image initialized?
   */
  irisIsMacroWithOverride(Initialized)

  /** Get the MeshAssembly associated with the time point */
  virtual MeshAssembly *GetMeshAssembly(unsigned int timepoint);

  /**
   *  Set the mesh and its id for a time point
   *  Implemente in subclasses
   */
  virtual void SetMesh(vtkPolyData *mesh, unsigned int timepoint, LabelType id) = 0;

  /** Return true if the mesh needs update */
  virtual bool IsMeshDirty(unsigned int timepoint) = 0;

  /** Return true if the object is an instance of the type or a subclass of the type */
  virtual bool IsA(const char *type) const;

  /** Get the mesh display mapping. This saves renderer a type cast */
  virtual MeshDisplayMappingPolicy *GetMeshDisplayMappingPolicy() const = 0;

  /**
    Compute the image histogram. The histogram is cached inside of the
    object, so repeated calls to this function with the same nBins parameter
    will not require additional computation.

    Calling with default parameter (0) will use the same number of bins that
    is currently in the histogram (i.e., return/recompute current histogram).
    If there is no current histogram, a default histogram with 128 entries
    will be generated.

    For multi-component data, the histogram is pooled over all components.
    */
  virtual const ScalarImageHistogram *GetHistogram(size_t nBins) override;

  // End of virtual methods definition
  //------------------------------------------------

  /** Get the active data array property */
  SmartPtr<MeshLayerDataArrayProperty> GetActiveDataArrayProperty();

  /** Set data array with the id as active mesh data attribute */
  void SetActiveMeshLayerDataPropertyId (int id);
  int GetActiveMeshLayerDataPropertyId()
  { return m_ActiveDataPropertyId; }

  /** Get mesh layer data property map */
  MeshLayerCombinedPropertyMap GetCombinedDataProperty()
  { return m_CombinedDataPropertyMap; }

  /** Get mesh for a timepoint and id */
  PolyDataWrapper *GetMesh(unsigned int timepoint, LabelType id);

  /**
    Helper method to merge two data property map
    We need to merge properties from all polydata in the assemblies and timepoints
    to one in the layer
    */
  void MergeDataProperties(MeshLayerDataArrayPropertyMap &dest, MeshDataArrayPropertyMap &src);

  const VectorModeNameMap &GetVectorModeNameMap() const
  { return m_VectorModeNameMap; }

  VectorModes GetActiveVectorMode()
  { return m_ActiveVectorMode; }

  void SetActiveVectorMode(VectorModes mode)
  { this->m_ActiveVectorMode = mode; }

  // Give display mapping policy access to protected members for flexible
  // configuration and data retrieval
  friend class MeshDisplayMappingPolicy;

protected:
  MeshWrapperBase();
  virtual ~MeshWrapperBase();

  MeshAssemblyMapType m_MeshAssemblyMap;

  // Combining Point and Cell Data Properties. Uniqely indexed.
  MeshLayerCombinedPropertyMap m_CombinedDataPropertyMap;

  // Actual storage of the point data cell data properties
  MeshLayerDataArrayPropertyMap m_PointDataProperties;
  MeshLayerDataArrayPropertyMap m_CellDataProperties;

  // Indicate which data property is active
  int m_ActiveDataPropertyId = 0;

  UserDataMapType m_UserDataMap;

  // Each layer has a filename, from which it is belived to have come
  std::string m_FileName, m_FileNameShort;

  // Each layer has a nickname. But this gets complicated, because the nickname
  // can be set by the user, or it can be default for the wrapper, or it can be
  // derived from the filename. The nicknames are used in the following order.
  // - if there is a custom nickname, it is shown
  // - if there is a filename, nickname is set to the shortened filename
  // - if there is no filename, nickname is set to the default nickname
  std::string m_DefaultNickname, m_CustomNickname;

  // Tags for this image layer
  TagList m_Tags;

  bool m_Initialized = false;

  // Transparency
  double m_Alpha;

  // Data Array Property Id
  int m_CombinedPropID = 0;

  // Active Vector Mode
  VectorModes m_ActiveVectorMode = VectorModes::MAGNITUDE;

  const VectorModeNameMap m_VectorModeNameMap =
  {
    {VectorModes::MAGNITUDE, "Magnitude"},
    {VectorModes::COMPONENT, "Component"},
    {VectorModes::RGBCOLORS, "RGBColors"}
  };
};

#endif // MESHWRAPPERBASE_H
