#ifndef ABSTRACTVTKRENDERER_H
#define ABSTRACTVTKRENDERER_H

#include "AbstractRenderer.h"
#include <vtkSmartPointer.h>
#include "UIReporterDelegates.h"

class vtkRenderer;
class vtkRenderWindow;
class vtkGenericOpenGLRenderWindow;
class vtkRenderWindowInteractor;
class vtkInteractorObserver;

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

  /**
   * This method will be called when the renderer is attached to a widget.
   * The widget maintains a vtkRenderWindow and passes a pointer to it to
   * the renderer. The default implementation adds the rendered and
   * interactor to the render window
   */
  virtual void SetRenderWindow(vtkRenderWindow *rwin);

  /** Get the render window */
  vtkRenderWindow *GetRenderWindow();

  /** Get the render window interactor */
  vtkRenderWindowInteractor *GetRenderWindowInteractor();

  /** Get the renderer */
  vtkRenderer *GetRenderer();

  void SetInteractionStyle(InteractionStyle style);

  /**
   * Synchronizes camera with another instance of this class. Internally,
   * this copies the camera pointer from the reference object.
   */
  void SyncronizeCamera(Self *reference);

  virtual void SetBackgroundColor(Vector3d color);
  virtual Vector3d GetBackgroundColor() const;

  /**
   * Notifies the renderer of change in window size and DPI. This method should
   * be called by the widget displaying this renderer when it is resized or moved
   * from screen to screen. Child classes should override OnDevicePixelRatioChange()
   */
  virtual void OnWindowResize(int w, int h, int vppr);

protected:

  // Render window object used to render VTK stuff
  vtkSmartPointer<vtkRenderWindow> m_RenderWindow;
  vtkSmartPointer<vtkRenderer> m_Renderer;

  // Interaction style
  vtkSmartPointer<vtkInteractorObserver> m_InteractionStyle;

  // The device pixel ratio (1 for regular screens, 2 for retina)
  // This is updated in resizeGL
  int m_DevicePixelRatio;

  // Virtual method called when the device pixel ratio changes, allowing
  // the child classes to update some properties (e.g., font size)
  virtual void OnDevicePixelRatioChange(int itkNotUsed(old_ratio), int itkNotUsed(new_ratio)) {}

  AbstractVTKRenderer();
  virtual ~AbstractVTKRenderer() {}
};

#endif // ABSTRACTVTKRENDERER_H
