#ifndef GENERICVIEW3D_H
#define GENERICVIEW3D_H

#include <QtVTKRenderWindowBox.h>
#include <SNAPCommon.h>

#include <vtkSmartPointer.h>

class Generic3DModel;
class Generic3DRenderer;
class vtkGenericRenderWindowInteractor;
class vtkObject;
class CursorPlacementInteractorStyle;


class GenericView3D : public QtVTKRenderWindowBox
{
  Q_OBJECT

public:
  explicit GenericView3D(QWidget *parent = 0);
  virtual ~GenericView3D();

  void SetModel(Generic3DModel *model);

signals:

public slots:

  void onModelUpdate(const EventBucket &bucket);

protected:

  // The model in charge
  Generic3DModel *m_Model;

  // The renderer (we own it)
  SmartPtr<Generic3DRenderer> m_Renderer;

  vtkSmartPointer<CursorPlacementInteractorStyle> m_CursorPlacementStyle;
};

#endif // GENERICVIEW3D_H
