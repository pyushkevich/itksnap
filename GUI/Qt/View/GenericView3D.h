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

  void mouseEntered();
  void mouseLeft();
  void resized();

public slots:

  void onToolbarModeChange();

protected:

  // Enter and leave events
  virtual void enterEvent(QEnterEvent *ev) override;
  virtual void leaveEvent(QEvent *ev) override;
  virtual void resizeEvent(QResizeEvent *evt) override;

  // The model in charge
  Generic3DModel *m_Model;

  // The interactor styles corresponding to each mode
  vtkSmartPointer<vtkInteractorStyle> m_InteractionStyle[4];

  vtkSmartPointer<CursorPlacementInteractorStyle> m_CursorPlacementStyle;
};

#endif // GENERICVIEW3D_H
