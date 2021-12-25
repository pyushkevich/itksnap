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


class QtVTKRenderWindowBox : public QWidget
{
  Q_OBJECT

public:
  explicit QtVTKRenderWindowBox(QWidget *parent = 0);

  void connectITK(itk::Object *src, const itk::EventObject &ev,
                  const char *slot = SLOT(onModelUpdate(const EventBucket &)));

  // Set the renderer assigned to this widget
  virtual void SetRenderer(AbstractVTKRenderer *renderer);

  // Get the render window
  vtkRenderWindow *GetRenderWindow();

  // Get the internal widget that does the actual rendering
  QWidget *GetInternalWidget();

  // Whether to grab keyboard focus when the mouse enters this widget
  irisGetSetMacro(GrabFocusOnEntry, bool)

  /** Take a screenshot, save to a PNG file */
  virtual bool SaveScreenshot(std::string filename);

public slots:

  // Default slot for model updates
  virtual void onModelUpdate(const EventBucket &);

protected:

  // Internal widget - the one that actually does rendering
  QWidget *m_InternalWidget;

  // Capture resize events
  virtual void resizeEvent(QResizeEvent *evt) override;

  // Whether this widget grabs keyboard focus when the mouse enters it
  bool m_GrabFocusOnEntry;

  void RendererCallback(vtkObject *src, unsigned long event, void *data);

  // The class that does the actual rendering for us
  AbstractVTKRenderer *m_Renderer;

  // Enter and leave events
  virtual void enterEvent(QEnterEvent *) override;
  virtual void leaveEvent(QEvent *) override;

  // TODO delete this stuff
  vtkSmartPointer<vtkSphereSource> sphereSource;
  vtkSmartPointer<vtkPolyDataMapper> mapper;
  vtkSmartPointer<vtkActor> actor;
  vtkSmartPointer<vtkRenderer> renderer;
};
#endif // QTVTKRENDERWINDOWBOX_H
