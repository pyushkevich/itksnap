#ifndef SLICEWINDOWINTERACTIONDELEGATEWIDGET_H
#define SLICEWINDOWINTERACTIONDELEGATEWIDGET_H

#include <QtInteractionDelegateWidget.h>

class SliceWindowInteractionDelegateWidget : public QtInteractionDelegateWidget
{
  Q_OBJECT

public:
  explicit SliceWindowInteractionDelegateWidget(QWidget *parent = 0);

  irisSetMacro(ParentModel, GenericSliceModel *);

protected:

  // Slice coordinates of the event
  Vector3d m_LastPressXSlice, m_XSlice;

  // Parent model
  GenericSliceModel *m_ParentModel;

  void preprocessEvent(QEvent *ev);
};

#endif // SLICEWINDOWINTERACTIONDELEGATEWIDGET_H
