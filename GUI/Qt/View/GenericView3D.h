#ifndef GENERICVIEW3D_H
#define GENERICVIEW3D_H

#include <QtAbstractOpenGLBox.h>
#include <SNAPCommon.h>

#include <vtkSmartPointer.h>

class Generic3DModel;
class Generic3DRenderer;
class vtkGenericRenderWindowInteractor;
class vtkObject;
class CursorPlacementInteractorStyle;


class GenericView3D : public QtAbstractOpenGLBox
{
  Q_OBJECT

public:
  explicit GenericView3D(QWidget *parent = 0);
  virtual ~GenericView3D();

  void SetModel(Generic3DModel *model);

  AbstractRenderer *GetRenderer() const;

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

  vtkGenericRenderWindowInteractor *iren;

  vtkSmartPointer<CursorPlacementInteractorStyle> m_CursorPlacementStyle;
};

#endif // GENERICVIEW3D_H
