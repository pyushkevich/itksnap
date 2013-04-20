#ifndef GENERICVIEW3D_H
#define GENERICVIEW3D_H

#include <QtVTKRenderWindowBox.h>
#include <SNAPCommon.h>

#include <vtkSmartPointer.h>

class Generic3DModel;
class vtkGenericRenderWindowInteractor;
class vtkObject;
class CursorPlacementInteractorStyle;
class vtkInteractorStyle;

class GenericView3D : public QtVTKRenderWindowBox
{
  Q_OBJECT

public:
  explicit GenericView3D(QWidget *parent = 0);
  virtual ~GenericView3D();

  void SetModel(Generic3DModel *model);

signals:

public slots:

  void onToolbarModeChange();

protected:

  // The model in charge
  Generic3DModel *m_Model;

  // The interactor styles corresponding to each mode
  vtkSmartPointer<vtkInteractorStyle> m_InteractionStyle[4];

  vtkSmartPointer<CursorPlacementInteractorStyle> m_CursorPlacementStyle;
};

#endif // GENERICVIEW3D_H
