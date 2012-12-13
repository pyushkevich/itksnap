#ifndef ORIENTATIONGRAPHICRENDERER_H
#define ORIENTATIONGRAPHICRENDERER_H

#include "AbstractVTKRenderer.h"
#include "vtkSmartPointer.h"
#include "PropertyModel.h"

class vtkSphereSource;

class OrientationGraphicRenderer : public AbstractVTKRenderer
{
public:

  typedef vnl_matrix<double> DirectionMatrix;
  typedef AbstractPropertyModel<DirectionMatrix> DirectionMatrixModel;

  irisITKObjectMacro(OrientationGraphicRenderer, AbstractVTKRenderer)

  void SetModel(DirectionMatrixModel *model);

  void OnUpdate();

protected:

  OrientationGraphicRenderer();
  virtual ~OrientationGraphicRenderer() {}

  DirectionMatrixModel *m_Model;

  // A sphere to render
  vtkSmartPointer<vtkSphereSource> m_Dummy;

};

#endif // ORIENTATIONGRAPHICRENDERER_H
