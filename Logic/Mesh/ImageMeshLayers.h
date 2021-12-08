#ifndef IMAGEMESHLAYERS_H
#define IMAGEMESHLAYERS_H

#include "SNAPCommon.h"
#include "itkObject.h"
#include "itkObjectFactory.h"
#include "MeshWrapperBase.h"

/**
 * \class ImageMeshLayers
 * \brief The ImageMeshLayers class stores mesh layers for current workspace
 */

class ImageMeshLayers : public itk::Object
{
public:
  irisITKObjectMacro(ImageMeshLayers, itk::Object)

  typedef MeshWrapperBase::MeshLayerIdType MeshLayerIdType;
  typedef std::map<MeshLayerIdType, SmartPtr<MeshWrapperBase>> LayerMapType;
  typedef typename LayerMapType::const_iterator LayerConstIteratorType;
  typedef typename LayerMapType::iterator LayerIteratorType;

  /** Add a layer */
  void AddLayer(SmartPtr<MeshWrapperBase> meshLayer);

  /** Get a layer by id */
  SmartPtr<MeshWrapperBase> GetLayer(MeshLayerIdType id);

  /** Remove a layer by id */
  void RemoveLayer(MeshLayerIdType id);

  /** Unload the layers */
  void Unload();

  /** Get a vector of all stored mesh layer ids */
  std::vector<MeshLayerIdType> GetLayerIds() const;

  /** Get number of layers */
  std::size_t size() { return m_Layers.size(); }

  /** Get and Set the layer id of the currently active layer */
  irisGetSetMacro(ActiveLayerId, MeshLayerIdType)

  /** Allow an iterator to access protected members */
  friend class MeshLayerIterator;

protected:
  ImageMeshLayers();
  virtual ~ImageMeshLayers() = default;

  LayerMapType m_Layers;

  // Id of the mesh layer that is currently active
  MeshLayerIdType m_ActiveLayerId;
};

class MeshLayerIterator
{
public:
  MeshLayerIterator(ImageMeshLayers *data);
  ~MeshLayerIterator() = default;

  /** If reached the end */
  bool IsAtEnd() const;

  /** Move to begin */
  MeshLayerIterator & MoveToBegin();

  /** Incremental operator */
  MeshLayerIterator & operator++ ();

  /** Get the id of the layer */
  MeshWrapperBase::MeshLayerIdType GetUniqueId() const;

  /** Get the layer */
  MeshWrapperBase * GetLayer() const;

private:
  ImageMeshLayers::LayerMapType *m_Layers;
  ImageMeshLayers::LayerIteratorType m_It;

};

#endif // IMAGEMESHLAYERS_H
