#ifndef ABSTRACTVTKSCENERENDERER_H
#define ABSTRACTVTKSCENERENDERER_H

#include "AbstractRenderer.h"
#include <vtkSmartPointer.h>

class vtkRenderer;
class vtkRenderWindow;
class vtkGenericOpenGLRenderWindow;
class vtkContextView;

/**
  This class provides support for rendering a VTK scene in OpenGL. It sets
  up the basic infrastructure, but the child class is the one responsible
  for maintaining and updating the scene.

  The child class should initialize the scene (vtkProps) in its constructor
  and add these props to the m_Renderer.

  The child class should override the OnUpdate() method inherited from
  AbstractModel(), and in that method, ensure that the scene is up to date,
  based on the events currently present in the EventBucket
  */
class AbstractVTKSceneRenderer : public AbstractRenderer
{
public:

  irisITKObjectMacro(AbstractVTKSceneRenderer, AbstractRenderer)

  virtual void paintGL();
  virtual void resizeGL(int w, int h);
  virtual void initializeGL();

  vtkRenderWindow *GetRenderWindow();

  irisGetSetMacro(BackgroundColor, Vector3d)


protected:

  // Render window object used to render VTK stuff
  vtkSmartPointer<vtkGenericOpenGLRenderWindow> m_RenderWindow;
  vtkSmartPointer<vtkRenderer> m_Renderer;

  vtkSmartPointer<vtkContextView> m_ContextView;

  // Background color
  Vector3d m_BackgroundColor;

  AbstractVTKSceneRenderer();
  virtual ~AbstractVTKSceneRenderer() {}
};

#endif // ABSTRACTVTKSCENERENDERER_H
