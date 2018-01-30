#ifndef SLICEWINDOWINTERACTIONDELEGATEWIDGET_H
#define SLICEWINDOWINTERACTIONDELEGATEWIDGET_H

#include <QtInteractionDelegateWidget.h>

class GenericSliceView;
class ImageWrapperBase;

class SliceWindowInteractionDelegateWidget : public QtInteractionDelegateWidget
{
  Q_OBJECT

public:
  explicit SliceWindowInteractionDelegateWidget(GenericSliceView *parent = 0);

  irisSetMacro(ParentModel, GenericSliceModel *)

protected:

  // We override this method from parent class in order to handle the fact
  // that a slice view may have multiple cells and thus multiple viewports
  virtual Vector3d GetEventWorldCoordinates(QMouseEvent *ev, bool flipY);

  // Slice coordinates of the event
  Vector3d m_LastPressXSlice, m_XSlice;

  // The cell in which the last press was generated
  Vector2ui m_LastPressLayoutCell;

  // Whether the current position of the mouse corresponds to a layer
  ImageWrapperBase *m_HoverOverLayer;

  // Whether the layer that is hovered over is displayed as a thumbnail
  bool m_HoverOverLayerIsThumbnail;

  // Parent model
  GenericSliceModel *m_ParentModel;

  // Parent widget
  GenericSliceView *m_ParentView;

  void preprocessEvent(QEvent *ev);
  void postprocessEvent(QEvent *ev);

  bool IsMouseOverLayerThumbnail();
  bool IsMouseOverFullLayer();
};

#endif // SLICEWINDOWINTERACTIONDELEGATEWIDGET_H
