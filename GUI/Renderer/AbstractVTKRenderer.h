#ifndef ABSTRACTVTKRENDERER_H
#define ABSTRACTVTKRENDERER_H

#include "AbstractRenderer.h"
#include <vtkSmartPointer.h>

class vtkRenderer;
class vtkRenderWindow;
class vtkGenericOpenGLRenderWindow;

/**
 * @brief The child of AbstractRenderer that holds a VTK render window
 * and can render VTK objects such as polydata
 */
class AbstractVTKRenderer : public AbstractRenderer
{
public:

  irisITKObjectMacro(AbstractVTKRenderer, AbstractRenderer)

  virtual void paintGL();
  virtual void resizeGL(int w, int h);
  virtual void initializeGL();

  vtkRenderWindow *GetRenderWindow();

  irisGetSetMacro(BackgroundColor, Vector3d)

protected:

  // Render window object used to render VTK stuff
  vtkSmartPointer<vtkGenericOpenGLRenderWindow> m_RenderWindow;
  vtkSmartPointer<vtkRenderer> m_Renderer;

  // Background color
  Vector3d m_BackgroundColor;

  AbstractVTKRenderer();
  virtual ~AbstractVTKRenderer() {}
};

#endif // ABSTRACTVTKRENDERER_H
