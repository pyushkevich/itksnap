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

  /** Get and Set the layer id of the currently active layer */
  irisGetSetMacro(ActiveLayerId, MeshLayerIdType)

protected:
  ImageMeshLayers();
  virtual ~ImageMeshLayers() = default;

  std::map<MeshLayerIdType, SmartPtr<MeshWrapperBase>> m_Layers;

  // Id of the mesh layer that is currently active
  MeshLayerIdType m_ActiveLayerId;
};

#endif // IMAGEMESHLAYERS_H
