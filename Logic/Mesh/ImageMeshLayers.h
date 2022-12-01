#ifndef IMAGEMESHLAYERS_H
#define IMAGEMESHLAYERS_H

#include "SNAPCommon.h"
#include "itkObject.h"
#include "itkObjectFactory.h"
#include "MeshWrapperBase.h"
#include "ImageWrapperTraits.h"
#include "GuidedMeshIO.h"

class MeshLayerIterator;
class GenericImageData;
class LabelImageWrapper;
class SegmentationMeshWrapper;
class LevelSetMeshWrapper;

/**
 * \class ImageMeshLayers
 * \brief The ImageMeshLayers class stores mesh layers for current workspace
 */

class ImageMeshLayers : public itk::Object
{
public:
  irisITKObjectMacro(ImageMeshLayers, itk::Object)

  typedef std::map<unsigned long, SmartPtr<MeshWrapperBase>> LayerMapType;
  typedef typename LayerMapType::const_iterator LayerConstIteratorType;
  typedef typename LayerMapType::iterator LayerIteratorType;
  typedef GuidedMeshIO::FileFormat FileFormat;

  /** Initialize */
  void Initialize(GenericImageData *imageData);

  /** Add a layer.
   *  notifyInspector controls the invoking of event that triggers
   *  the row rebuilding of the layer inspector
   *  Sometime it needs to be turned off, such as when adding a levelset
   *  mesh layer
  */
  void AddLayer(MeshWrapperBase *meshLayer, bool notifyInspector = true);

  /** Add a layer by reading a list of files */
  void AddLayerFromFiles(std::vector<std::string> &fn_list, FileFormat format,
                         unsigned int startFromTP = 1);

  /** Load mesh to an existing layer
   *  Location identified by:
   *  1. layer_id
   *  2. Timepoint
   *  3. mesh_id (default to 0)
  */
  void LoadFileToLayer(const char *filename, FileFormat format,
                         unsigned long layer_id, unsigned int timepoint, LabelType mesh_id = 0);

  /** Add mesh layers from registry */
  void LoadFromRegistry(Registry &project, std::string &project_dir_orig,
                                 std::string &project_dir_crnt);

  /** Save mesh layers to registry */
  void SaveToRegistry(Registry &folder);

  /** Get a layer by id */
  SmartPtr<MeshWrapperBase> GetLayer(unsigned long id);

  /** Return a layer iterator */
  MeshLayerIterator GetLayers();

  /** Remove a layer by id */
  void RemoveLayer(unsigned long id);

  /** Unload the layers */
  void Unload();

  /** Get a vector of all stored mesh layer ids */
  std::vector<unsigned long> GetLayerIds() const;

  /** Get number of layers */
  std::size_t size() { return m_Layers.size(); }

  /** Get and Set the layer id of the currently active layer */
  irisGetMacro(ActiveLayerId, unsigned long)
  void SetActiveLayerId (unsigned long id);

  /** Refresh the active mesh layer
   *  Return 0 if success,
   *  Return 1 if failed or nothing to update, to avoid triggering events
   *  The caller of this method is responsible to check if mesh is necessary
   *  to update (dirty);
   */
  int UpdateActiveMeshLayer(itk::Command *progressCmd);

  /** Return the active layer Modified Time */
  unsigned long GetActiveMeshMTime();

  /** Create a new segmentation mesh layer for a segmentation image
   *  and add it to the layer map
   */
  SegmentationMeshWrapper *AddSegmentationMeshLayer(LabelImageWrapper* segImg);

  /** Create a new levelset mesh layer for a levelset image
   *  and add it to the layer map
   */
  LevelSetMeshWrapper *AddLevelSetMeshLayer(LevelSetImageWrapper *lsImg);


  /** Return true if the active mesh layer needs to be updated */
  bool IsActiveMeshLayerDirty();

  /** Check if an image has a corresponding mesh layer created */
  bool HasMeshForImage(unsigned long image_id) const;

  /** Return the mesh layer for an image. */
  MeshWrapperBase *GetMeshForImage(unsigned long image_id);

  /** Allow an iterator to access protected members */
  friend class MeshLayerIterator;

protected:
  ImageMeshLayers();
  virtual ~ImageMeshLayers() = default;

  LayerMapType m_Layers;

  // Id of the mesh layer that is currently active
  unsigned long m_ActiveLayerId = 0;

  // If the mesh layer belongs to a SNAP Image Data
  bool m_IsSNAP = false;

  SmartPtr<GenericImageData> m_ImageData;

  // set of segmentation image id that map to the related layer pointer
  std::map<unsigned long, MeshWrapperBase*> m_ImageToMeshMap;
};

/**
 * \class MeshLayerIterator
 * \brief Iterator that traverses through all stored mesh layers in the ImageMeshLayers object
 */

class MeshLayerIterator
{
public:
  MeshLayerIterator(ImageMeshLayers *data);
  ~MeshLayerIterator() = default;

  /** If reached the end */
  bool IsAtEnd() const;

  /** Move to begin */
  MeshLayerIterator & MoveToBegin();

  /** Incremental operators */
  MeshLayerIterator & operator++ ();
  MeshLayerIterator & operator++ (int) = delete;

  /** Get the id of the layer */
  unsigned long GetUniqueId() const;

  /** Get the layer */
  MeshWrapperBase * GetLayer() const;

private:
  ImageMeshLayers::LayerMapType *m_Layers;
  ImageMeshLayers::LayerIteratorType m_It;

};

#endif // IMAGEMESHLAYERS_H
