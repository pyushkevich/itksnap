#ifndef MESHWRAPPERBASE_H
#define MESHWRAPPERBASE_H

#include "SNAPCommon.h"
#include "itkObject.h"
#include "itkObjectFactory.h"
#include "vtkSmartPointer.h"

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

class MeshWrapperBase : public itk::Object
{
public:
  irisITKAbstractObjectMacro(MeshWrapperBase, itk::Object)

  typedef unsigned long MeshLayerIdType;

  /** MeshCollection maps an instance of vtkPolyData to a LabelType id. */
  typedef std::map<LabelType, vtkPolyData*> MeshCollection;

  /** MeshCollectionMap maps a MeshCollection to a time point */
  typedef std::map<unsigned int, MeshCollection> MeshCollectionMap;

  /** Get the MeshCollection associated with the time point */
  virtual MeshCollection GetMeshCollection(unsigned int timepoint);

  /** Get the mesh associated with the id and the time point */
  virtual vtkPolyData *GetMesh(unsigned int timepoint, LabelType id);

  /** Set a collection of mesh for a time point */
  virtual void SetMeshCollection(MeshCollection collection, unsigned int timepoint);

  /** Set the mesh and its id for a time point */
  virtual void SetMesh(vtkPolyData *mesh, unsigned int timepoint, LabelType id);

  /** Return true if the mesh needs update */
  virtual bool IsMeshDirty(unsigned int timepoint) const = 0;

  /** Update the meshes of a time point */
  virtual void UpdateMeshes(unsigned int timepoint) = 0;

  /** Return true if the object is an instance of the type or a subclass of the type */
  virtual bool IsA(const char *type) const;

  /** Return the unique id for the current object */
  MeshLayerIdType GetId() const;

protected:
  MeshWrapperBase();
  virtual ~MeshWrapperBase();

  MeshCollectionMap m_MeshCollectionMap;

  MeshLayerIdType m_Id;
};

#endif // MESHWRAPPERBASE_H
