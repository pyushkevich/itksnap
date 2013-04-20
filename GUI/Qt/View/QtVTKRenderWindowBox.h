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
class QtVTKRenderWindowBox : public QtSimpleOpenGLBox
{
  Q_OBJECT

public:
  explicit QtVTKRenderWindowBox(QWidget *parent = 0);

  virtual void SetRenderer(AbstractRenderer *renderer);
  
signals:
  
protected:

  QtVTKInteractionDelegateWidget *m_InteractionDelegate;

  void RendererCallback(vtkObject *src, unsigned long event, void *data);

};

#endif // QTVTKRENDERWINDOWBOX_H
