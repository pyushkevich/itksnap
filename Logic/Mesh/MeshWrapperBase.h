#ifndef MESHWRAPPERBASE_H
#define MESHWRAPPERBASE_H

#include "SNAPCommon.h"
#include "itkObject.h"
#include "itkObjectFactory.h"
#include "vtkSmartPointer.h"
#include "ImageWrapperBase.h"

class AbstractMeshIODelegate;
class vtkPolyData;

/**
 *  \class MeshWrapperBase
 *  \brief An abstract class representing a mesh layer in ITK-SNAP
 *
 *  The class wraps around a MeshCollectionMap, which maps a collection of vtkPolyData
 *  , identified by ids (id is in LabelType), to each time points.
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

  /** MeshCollection maps an instance of vtkPolyData to a LabelType id, representing
   *   a collection of vtkPolyData meshes that need to be rendered in a single scene.*/
  typedef std::map<LabelType, vtkSmartPointer<vtkPolyData>> MeshCollection;

  /** MeshCollectionMap maps a MeshCollection to a time point */
  typedef std::map<unsigned int, MeshCollection> MeshCollectionMap;

  /** Mesh data array name map */
  typedef std::map<int, std::string> MeshDataArrayNameMap;

  /** Controls which type of data is currently active for rendering */
  enum ActiveMeshDataType { POINT_DATA = 0, CELL_DATA };

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

  /** Get the MeshCollection associated with the time point */
  virtual MeshCollection GetMeshCollection(unsigned int timepoint);

  /** Get the mesh associated with the id and the time point */
  virtual vtkSmartPointer<vtkPolyData> GetMesh(unsigned int timepoint, LabelType id);

  /** Set a collection of mesh for a time point */
  virtual void SetMeshCollection(MeshCollection collection, unsigned int timepoint);

  /** Set the mesh and its id for a time point */
  virtual void SetMesh(vtkSmartPointer<vtkPolyData>mesh, unsigned int timepoint, LabelType id);

  /** Return true if the mesh needs update */
  virtual bool IsMeshDirty(unsigned int timepoint) const = 0;

  /** Update the meshes of a time point */
  virtual void UpdateMeshes(unsigned int timepoint) = 0;

  /** Return true if the object is an instance of the type or a subclass of the type */
  virtual bool IsA(const char *type) const;

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

  /** Get/Set ActiveMeshDataType */
  irisGetSetMacro(ActiveMeshDataType, ActiveMeshDataType)

  /** Get the data array names for current active mesh data type */
  MeshDataArrayNameMap GetMeshDataArrayNameMap(unsigned int timepoint, LabelType id = 0);

  /** Get the activated data array name of the current active mesh data type */
  int GetActiveMeshDataArrayId(unsigned int timepoint, LabelType id = 0);

  /** Set data array with the id as active mesh data attribute */
  void SetActiveMeshDataArrayId(int index, unsigned int timepoint, LabelType id = 0);


protected:
  MeshWrapperBase();
  virtual ~MeshWrapperBase();

  MeshCollectionMap m_MeshCollectionMap;

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

  // The active mesh data type for rendering
  ActiveMeshDataType m_ActiveMeshDataType = POINT_DATA;

};

#endif // MESHWRAPPERBASE_H
