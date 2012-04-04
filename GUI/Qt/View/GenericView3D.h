#ifndef GENERICVIEW3D_H
#define GENERICVIEW3D_H

#include <SNAPQGLWidget.h>
#include <SNAPCommon.h>

class Generic3DModel;
class Generic3DRenderer;
class vtkGenericRenderWindowInteractor;
class vtkObject;


class GenericView3D : public SNAPQGLWidget
{
  Q_OBJECT

public:
  explicit GenericView3D(QWidget *parent = 0);
  virtual ~GenericView3D();

  void SetModel(Generic3DModel *model);

  AbstractRenderer *GetRenderer() const;

  bool event(QEvent *);

  void resizeGL(int w, int h);

  void RendererCallback(vtkObject *src, unsigned long event, void *data);
signals:

public slots:

  void onModelUpdate(const EventBucket &bucket);

protected:

  // The model in charge
  Generic3DModel *m_Model;

  // The renderer (we own it)
  SmartPtr<Generic3DRenderer> m_Renderer;

  bool m_Dragging;
  Qt::MouseButton m_DragButton;

  vtkGenericRenderWindowInteractor *iren;
};

#endif // GENERICVIEW3D_H
