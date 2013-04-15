#ifndef GENERIC3DRENDERER_H
#define GENERIC3DRENDERER_H

#include "AbstractVTKRenderer.h"
#include <vtkSmartPointer.h>

class Generic3DModel;
class vtkGenericOpenGLRenderWindow;
class vtkRenderer;
class vtkRenderWindow;
class vtkLineSource;
class vtkActor;
class vtkPropAssembly;

class Generic3DRenderer : public AbstractVTKRenderer
{
public:

  irisITKObjectMacro(Generic3DRenderer, AbstractVTKRenderer)

  void paintGL();

  void SetModel(Generic3DModel *model);

  virtual void OnUpdate();

  void ResetView();

protected:
  Generic3DRenderer();
  virtual ~Generic3DRenderer() {}

  Generic3DModel *m_Model;

  // Update the actors and mappings for the renderer
  void UpdateRendering();  
  void UpdateAxisRendering();
  void UpdateMeshAppearance();

  // Update the camera
  void UpdateCamera(bool reset);

  // Prop assembly containing all the segmentation meshes
  vtkSmartPointer<vtkPropAssembly> m_MeshAssembly;

  // Line sources for drawing the crosshairs
  vtkSmartPointer<vtkLineSource> m_AxisLineSource[3];
  vtkSmartPointer<vtkActor> m_AxisActor[3];

  // List of actors, properties, mappers for meshes
  //std::vector< vtkSmartPointer<vtkActor> > m_MeshActors;
  //std::vector< vtkSmartPointer<vtkPolyDataMapper> > m_MeshMappers;
  //std::vector< vtkSmartPointer<vtkProperty> > m_MeshProps;
};

#endif // GENERIC3DRENDERER_H
