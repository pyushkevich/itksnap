#ifndef ABSTRACTVTKSCENERENDERER_H
#define ABSTRACTVTKSCENERENDERER_H

#include "AbstractVTKRenderer.h"
#include <vtkSmartPointer.h>

class vtkRenderer;
class vtkRenderWindow;
class vtkGenericOpenGLRenderWindow;
class vtkContextView;
class vtkChart;

/**
  This class provides support for rendering a VTK scene in OpenGL. It sets
  up the basic infrastructure, but the child class is the one responsible
  for maintaining and updating the scene.

  This class should be used for rendering 2D scenes (charts, etc), not 3D
  data such as vtkPolyData. For that, @see AbstractVTKRenderer

  The child class should initialize the scene (vtkProps) in its constructor
  and add these props to the m_Renderer.

  The child class should override the OnUpdate() method inherited from
  AbstractModel(), and in that method, ensure that the scene is up to date,
  based on the events currently present in the EventBucket
  */
class AbstractVTKSceneRenderer : public AbstractVTKRenderer
{
public:

  irisITKObjectMacro(AbstractVTKSceneRenderer, AbstractVTKRenderer)

  virtual void paintGL();

  irisGetSetMacro(BackgroundColor, Vector3d)

protected:

  // Render window object used to render VTK stuff
  vtkSmartPointer<vtkContextView> m_ContextView;

  // Background color
  Vector3d m_BackgroundColor;

  AbstractVTKSceneRenderer();
  virtual ~AbstractVTKSceneRenderer() {}

  // Helper method for updating device pixel ratio (i.e. retina mode)
  // for VTK charts
  virtual void UpdateChartDevicePixelRatio(vtkChart *chart, int old_ratio, int new_ratio);
};

#endif // ABSTRACTVTKSCENERENDERER_H
