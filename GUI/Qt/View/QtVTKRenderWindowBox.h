#ifndef QTVTKRENDERWINDOWBOX_H
#define QTVTKRENDERWINDOWBOX_H

#include <QVTKOpenGLNativeWidget.h>
#include <SNAPCommon.h>
#include <EventBucket.h>
#include <itkEventObject.h>
#include <itkObject.h>

class AbstractVTKRenderer;
class QtVTKInteractionDelegateWidget;
class vtkObject;
class vtkSphereSource;
class vtkPolyDataMapper;
class vtkActor;


/**
 * This is a qt widget that interfaces with an AbstractVTKRenderer.
 * The widget includes a delegate for passing events to the interactor
 * stored in the AbstractVTKRenderer.
 */


class QtVTKRenderWindowBox : public QVTKOpenGLNativeWidget
{
  Q_OBJECT

public:
  explicit QtVTKRenderWindowBox(QWidget *parent = 0);

  void connectITK(itk::Object *src, const itk::EventObject &ev,
                  const char *slot = SLOT(onModelUpdate(const EventBucket &)));

  // Set the renderer assigned to this widget
  virtual void SetRenderer(AbstractVTKRenderer *renderer);

  // Whether to grab keyboard focus when the mouse enters this widget
  irisGetSetMacro(GrabFocusOnEntry, bool)

  /** Take a screenshot, save to a PNG file */
  virtual bool SaveScreenshot(std::string filename);

public slots:

  // Default slot for model updates
  virtual void onModelUpdate(const EventBucket &);

protected:

  // Interaction delegate
  QtVTKInteractionDelegateWidget *m_InteractionDelegate;

  virtual void paintGL() override;
  virtual void initializeGL() override;
  virtual void resizeGL(int w, int h) override;

  // Whether this widget grabs keyboard focus when the mouse enters it
  bool m_GrabFocusOnEntry;

  // Screenshot request to save the contents on next repaint
  QString m_ScreenshotRequest;

  void RendererCallback(vtkObject *src, unsigned long event, void *data);

  // The class that does the actual rendering for us
  AbstractVTKRenderer *m_Renderer;

  // Enter and leave events
  virtual void enterEvent(QEvent *) override;
  virtual void leaveEvent(QEvent *) override;

  // TODO delete this stuff
  vtkSmartPointer<vtkSphereSource> sphereSource;
  vtkSmartPointer<vtkPolyDataMapper> mapper;
  vtkSmartPointer<vtkActor> actor;
  vtkSmartPointer<vtkRenderer> renderer;
};
#endif // QTVTKRENDERWINDOWBOX_H
