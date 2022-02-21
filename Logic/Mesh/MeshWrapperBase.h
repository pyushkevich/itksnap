#ifndef MESHWRAPPERBASE_H
#define MESHWRAPPERBASE_H

#include "SNAPCommon.h"
#include "itkObject.h"
#include "itkObjectFactory.h"
#include "vtkSmartPointer.h"
#include "ImageWrapperBase.h"
#include "IntensityCurveVTK.h"
#include "ColorMap.h"

class AbstractMeshIODelegate;
class vtkPolyData;
class MeshDisplayMappingPolicy;
class MeshAssembly;
class vtkDataArray;
class vtkDataSetAttributes;

class MeshDataArrayProperty : public itk::Object
{
public:
  irisITKObjectMacro(MeshDataArrayProperty, itk::Object);

  enum MeshDataType { POINT_DATA=0, CELL_DATA, COUNT };

  void Initialize(vtkSmartPointer<vtkDataArray> array, MeshDataType type);

  void Update(vtkSmartPointer<vtkDataArray> array);

  /** Merge with another property. Adjusting min/max etc. */
  void Merge(SmartPtr<MeshDataArrayProperty> other);

  /** Get type of the property */
  MeshDataType GetType() const;

  /** Get Color Map */
  ColorMap* GetColorMap()
  { return m_ColorMap; }

  /** Get Intensity Curve */
  IntensityCurveVTK* GetIntensityCurve()
  { return m_IntensityCurve; }

  /** Get min */
  double GetMin() const { return m_min; }

  /** Get max */
  double GetMax() const { return m_max; }

  /** Get name */
  const char* GetName()
  { return m_Name; }

  void Print(std::ostream &os) const;

  /** Deep copy self to the other object */
  void DeepCopy(MeshDataArrayProperty *other) const;

protected:
  MeshDataArrayProperty();
  ~MeshDataArrayProperty();

  char* m_Name;
  double m_min;
  double m_max;

  MeshDataType m_Type;

  SmartPtr<ColorMap> m_ColorMap;
  SmartPtr<IntensityCurveVTK> m_IntensityCurve;
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

  void SetPolyData(vtkSmartPointer<vtkPolyData> polydata);

  vtkSmartPointer<vtkPolyData> GetPolyData();

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
  void AddMesh(SmartPtr<PolyDataWrapper> mesh, LabelType id=0u);

  /** Get the mesh by id */
  SmartPtr<PolyDataWrapper> GetMesh(LabelType id);

  /** Check existence of id */
  bool Exist(LabelType id);

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
  typedef std::map<int, SmartPtr<MeshDataArrayProperty>> MeshLayerDataPropertyMap;

  /** Maps Layer-Level Data Array Properties with name */
  typedef PolyDataWrapper::MeshDataArrayPropertyMap MeshDataArrayPropertyMap;

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
  irisGetMacroWithOverride(Tags, const TagList &)
  irisSetMacroWithOverride(Tags, const TagList &)

  /** Layer transparency */
  irisSetWithEventMacroWithOverride(Alpha, double, WrapperDisplayMappingChangeEvent)
  irisGetMacroWithOverride(Alpha, double)

  /**
   * Is the image initialized?
   */
  irisIsMacroWithOverride(Initialized)

  /** Get the MeshAssembly associated with the time point */
  virtual SmartPtr<MeshAssembly> GetMeshAssembly(unsigned int timepoint);

  /**
   *  Set the mesh and its id for a time point
   *  Implemente in subclasses if wrapper specific logic is needed
   */
  virtual void SetMesh(vtkSmartPointer<vtkPolyData>mesh, unsigned int timepoint, LabelType id);

  /** Return true if the mesh needs update */
  virtual bool IsMeshDirty(unsigned int timepoint) const = 0;

  /** Update the meshes of a time point */
  virtual void UpdateMeshes(unsigned int timepoint) = 0;

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
  SmartPtr<MeshDataArrayProperty> GetActiveDataArrayProperty();

  /** Set data array with the id as active mesh data attribute */
  void SetActiveMeshLayerDataPropertyId (int id);
  int GetActiveMeshLayerDataPropertyId()
  { return m_ActiveDataPropertyId; }

  /** Get mesh layer data property map */
  MeshLayerDataPropertyMap GetCombinedDataProperty()
  { return m_CombinedDataPropertyMap; }

  /** Get mesh for a timepoint and id */
  SmartPtr<PolyDataWrapper> GetMesh(unsigned int timepoint, LabelType id);

  /**
    Helper method to merge two data property map
    We need to merge properties from all polydata in the assemblies and timepoints
    to one in the layer
    */
  void MergeDataProperties(MeshDataArrayPropertyMap &dest, MeshDataArrayPropertyMap &src);

  // Give display mapping policy access to protected members for flexible
  // configuration and data retrieval
  friend class MeshDisplayMappingPolicy;

protected:
  MeshWrapperBase();
  virtual ~MeshWrapperBase();

  MeshAssemblyMapType m_MeshAssemblyMap;

  // Combining Point and Cell Data Properties. Uniqely indexed.
  MeshLayerDataPropertyMap m_CombinedDataPropertyMap;

  // Actual storage of the point data cell data properties
  MeshDataArrayPropertyMap m_PointDataProperties;
  MeshDataArrayPropertyMap m_CellDataProperties;

  // This should be re-set after construction
  int m_ActiveDataPropertyId = -1;

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

  SmartPtr<MeshDisplayMappingPolicy> m_DisplayMapping;

  // Transparency
  double m_Alpha;

  // Data Array Property Id
  int m_CombinedPropID = 0;
};

#endif // MESHWRAPPERBASE_H
