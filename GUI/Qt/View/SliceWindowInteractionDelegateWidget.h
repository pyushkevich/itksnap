#ifndef SLICEWINDOWINTERACTIONDELEGATEWIDGET_H
#define SLICEWINDOWINTERACTIONDELEGATEWIDGET_H

#include <QtInteractionDelegateWidget.h>

class GenericSliceView;

class SliceWindowInteractionDelegateWidget : public QtInteractionDelegateWidget
{
  Q_OBJECT

public:
  explicit SliceWindowInteractionDelegateWidget(GenericSliceView *parent = 0);

  irisSetMacro(ParentModel, GenericSliceModel *)

protected:

  // Slice coordinates of the event
  Vector3d m_LastPressXSlice, m_XSlice;

  // Parent model
  GenericSliceModel *m_ParentModel;

  // Parent widget
  GenericSliceView *m_ParentView;

  void preprocessEvent(QEvent *ev);

};

#endif // SLICEWINDOWINTERACTIONDELEGATEWIDGET_H
