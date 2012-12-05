#ifndef ORIENTATIONGRAPHICRENDERER_H
#define ORIENTATIONGRAPHICRENDERER_H

#include "AbstractVTKRenderer.h"
#include "vtkSmartPointer.h"

class vtkSphereSource;

class ReorientImageModel;

class OrientationGraphicRenderer : public AbstractVTKRenderer
{
public:
  irisITKObjectMacro(OrientationGraphicRenderer, AbstractVTKRenderer)

  void SetModel(ReorientImageModel *model);

  void OnUpdate();

protected:

  OrientationGraphicRenderer();
  virtual ~OrientationGraphicRenderer() {}

  ReorientImageModel *m_Model;

  // A sphere to render
  vtkSmartPointer<vtkSphereSource> m_Dummy;

};

#endif // ORIENTATIONGRAPHICRENDERER_H
