#ifndef GENERICVIEW3D_H
#define GENERICVIEW3D_H

#include <SNAPQGLWidget.h>
#include <SNAPCommon.h>

class Generic3DModel;
class Generic3DRenderer;

class GenericView3D : public SNAPQGLWidget
{
  Q_OBJECT

public:
  explicit GenericView3D(QWidget *parent = 0);

  irisGetMacro(Renderer, Generic3DRenderer *)

  void SetModel(Generic3DModel *model);

signals:

public slots:

  void onModelUpdate(const EventBucket &bucket);

protected:

  // The model in charge
  Generic3DModel *m_Model;

  // The renderer (we own it)
  SmartPtr<Generic3DRenderer> m_Renderer;

};

#endif // GENERICVIEW3D_H
