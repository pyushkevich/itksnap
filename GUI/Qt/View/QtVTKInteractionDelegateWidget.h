#ifndef QTVTKINTERACTIONDELEGATEWIDGET_H
#define QTVTKINTERACTIONDELEGATEWIDGET_H

#include "QtInteractionDelegateWidget.h"
#include "vtkSmartPointer.h"

class vtkRenderWindowInteractor;

/**
  This interaction delegate simply passes selected events from Qt to
  events in the vtkRenderWindowInteractor class.
  */
class QtVTKInteractionDelegateWidget : public QtInteractionDelegateWidget
{
  Q_OBJECT
public:
  explicit QtVTKInteractionDelegateWidget(QWidget *parent = 0);

  void mousePressEvent(QMouseEvent *);
  void mouseReleaseEvent(QMouseEvent *);
  void mouseMoveEvent(QMouseEvent *);

  void SetVTKInteractor(vtkRenderWindowInteractor *iren);
  vtkRenderWindowInteractor *GetVTKInteractor() const;

signals:

public slots:


protected:

  void SetVTKEventState(QMouseEvent *ev);

  // The vtkRenderWindowInteractor that receives our events
  vtkSmartPointer<vtkRenderWindowInteractor> m_VTKInteractor;
};

#endif // QTVTKINTERACTIONDELEGATEWIDGET_H
