#ifndef ABSTRACTVTKRENDERER_H
#define ABSTRACTVTKRENDERER_H

#include "AbstractRenderer.h"
#include <vtkSmartPointer.h>

class vtkRenderer;
class vtkRenderWindow;
class vtkGenericOpenGLRenderWindow;
class vtkRenderWindowInteractor;

/**
 * @brief The child of AbstractRenderer that holds a VTK render window
 * and can render VTK objects such as polydata. This class also holds an
 * interactor, which can be connected to a widget's events. However, by
 * default, interaction is disabled, and must be enabled by calling
 * SetInteractionStyle
 */
class AbstractVTKRenderer : public AbstractRenderer
{
public:

  irisITKObjectMacro(AbstractVTKRenderer, AbstractRenderer)

  /**
   * @brief Enums corresponding to different VTK interaction styles
   */
  enum InteractionStyle {
    NO_INTERACTION = 0, TRACKBALL_CAMERA, TRACKBALL_ACTOR, PICKER
  };

  virtual void paintGL();
  virtual void resizeGL(int w, int h);
  virtual void initializeGL();

  vtkRenderWindow *GetRenderWindow();
  vtkRenderWindowInteractor *GetRenderWindowInteractor();

  void SetInteractionStyle(InteractionStyle style);

  /**
   * Synchronizes camera with another instance of this class. Internally,
   * this copies the camera pointer from the reference object.
   */
  void SyncronizeCamera(Self *reference);

  irisGetSetMacro(BackgroundColor, Vector3d)

protected:

  // Render window object used to render VTK stuff
  vtkSmartPointer<vtkGenericOpenGLRenderWindow> m_RenderWindow;
  vtkSmartPointer<vtkRenderer> m_Renderer;

  // The interactor
  vtkSmartPointer<vtkRenderWindowInteractor> m_Interactor;

  // Background color
  Vector3d m_BackgroundColor;

  AbstractVTKRenderer();
  virtual ~AbstractVTKRenderer() {}
};

#endif // ABSTRACTVTKRENDERER_H
