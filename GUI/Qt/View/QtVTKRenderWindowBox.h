#ifndef QTVTKRENDERWINDOWBOX_H
#define QTVTKRENDERWINDOWBOX_H

#include <QtSimpleOpenGLBox.h>

class AbstractVTKRenderer;
class QtVTKInteractionDelegateWidget;
class vtkObject;


/**
 * This is a qt widget that interfaces with an AbstractVTKRenderer.
 * The widget includes a delegate for passing events to the interactor
 * stored in the AbstractVTKRenderer.
 */
/*
class QtVTKRenderWindowBox : public QtSimpleOpenGLBox
{
  Q_OBJECT

public:
  explicit QtVTKRenderWindowBox(QWidget *parent = 0);

  virtual void SetRenderer(AbstractRenderer *renderer);

  virtual void initializeGL() override;
  virtual void paintGL() override;
  
signals:
  
protected:

  QtVTKInteractionDelegateWidget *m_InteractionDelegate;

  void RendererCallback(vtkObject *src, unsigned long event, void *data);

};
*/

#include <QVTKOpenGLNativeWidget.h>

class QtVTKRenderWindowBox : public QVTKOpenGLNativeWidget
{
  Q_OBJECT

public:
  explicit QtVTKRenderWindowBox(QWidget *parent = 0);

  void connectITK(itk::Object *src, const itk::EventObject &ev, const char *slot);

  virtual void SetRenderer(AbstractVTKRenderer *renderer);

  virtual void initializeGL() override;
  virtual void resizeGL(int w, int h) override;
  virtual void paintGL() override;

signals:

protected:

  // QtVTKInteractionDelegateWidget *m_InteractionDelegate;

  void RendererCallback(vtkObject *src, unsigned long event, void *data);

protected:

  AbstractVTKRenderer *m_Renderer;

};
#endif // QTVTKRENDERWINDOWBOX_H
