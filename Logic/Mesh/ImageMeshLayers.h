#ifndef IMAGEMESHLAYERS_H
#define IMAGEMESHLAYERS_H

#include "SNAPCommon.h"
#include "itkObject.h"
#include "itkObjectFactory.h"
#include <list>

class IRISApplication;
class MeshWrapperBase;

class ImageMeshLayers : public itk::Object
{
public:
  irisITKObjectMacro(ImageMeshLayers, itk::Object)

protected:
  ImageMeshLayers();
  virtual ~ImageMeshLayers() = default;

  std::list<MeshWrapperBase> m_Layers;
};

#endif // IMAGEMESHLAYERS_H
