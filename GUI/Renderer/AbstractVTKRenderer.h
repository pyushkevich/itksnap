#ifndef ABSTRACTVTKRENDERER_H
#define ABSTRACTVTKRENDERER_H

#include "AbstractRenderer.h"
#include <vtkSmartPointer.h>
#include "UIReporterDelegates.h"

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

  /**
   * This method will be called when the renderer is attached to a widget.
   * The widget maintains a vtkRenderWindow and passes a pointer to it to
   * the renderer. The default implementation adds the rendered and
   * interactor to the render window
   */
  virtual void SetRenderWindow(vtkRenderWindow *rwin);

  /** Get the render window pointer */
  vtkRenderWindow *GetRenderWindow();

  vtkRenderer *GetRenderer();
  vtkRenderWindowInteractor *GetRenderWindowInteractor();

  void SetInteractionStyle(InteractionStyle style);

  /**
   * Synchronizes camera with another instance of this class. Internally,
   * this copies the camera pointer from the reference object.
   */
  void SyncronizeCamera(Self *reference);

  virtual void SetBackgroundColor(Vector3d color);
  virtual Vector3d GetBackgroundColor() const;

  virtual void paintGL() override {}

protected:

  // Render window object used to render VTK stuff
  vtkSmartPointer<vtkRenderWindow> m_RenderWindow;
  vtkSmartPointer<vtkRenderer> m_Renderer;

  // The interactor
  vtkSmartPointer<vtkRenderWindowInteractor> m_Interactor;

  // The device pixel ratio (1 for regular screens, 2 for retina)
  // This is updated in resizeGL
  int m_DevicePixelRatio;

  // Virtual method called when the device pixel ratio changes, allowing
  // the child classes to update some properties (e.g., font size)
  virtual void OnDevicePixelRatioChange(int old_ratio, int new_ratio) {}

  AbstractVTKRenderer();
  virtual ~AbstractVTKRenderer() {}
};

#endif // ABSTRACTVTKRENDERER_H
