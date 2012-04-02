#ifndef POLYGONDRAWINGINTERACTIONMODE_H
#define POLYGONDRAWINGINTERACTIONMODE_H

#include <SliceWindowInteractionDelegateWidget.h>
#include <SNAPCommon.h>

class GenericSliceModel;
class GenericSliceView;
class PolygonDrawingModel;
class PolygonDrawingRenderer;

class QMenu;
class QAction;

class PolygonDrawingInteractionMode : public SliceWindowInteractionDelegateWidget
{
  Q_OBJECT

public:
  explicit PolygonDrawingInteractionMode(GenericSliceView *parent = 0);
  ~PolygonDrawingInteractionMode();

  irisGetMacro(Model, PolygonDrawingModel *)
  void SetModel(PolygonDrawingModel *m_Model);

  irisGetMacro(Renderer, PolygonDrawingRenderer *)


  void mousePressEvent(QMouseEvent *ev);
  void mouseMoveEvent(QMouseEvent *ev);
  void mouseReleaseEvent(QMouseEvent *ev);
  void enterEvent(QEvent *);
  void leaveEvent(QEvent *);

  void contextMenuEvent(QContextMenuEvent *);

signals:

  void contextMenuRequested();

public slots:

  void onPastePolygon();
  void onAcceptPolygon();
  void onSplitSelected();
  void onDeleteSelected();
  void onClearPolygon();
  void onCloseLoopAndEdit();
  void onCloseLoopAndAccept();
  void onUndoLastPoint();
  void onCancelDrawing();

  void onModelUpdate(const EventBucket &bucket);


protected:

  PolygonDrawingModel *m_Model;
  SmartPtr<PolygonDrawingRenderer> m_Renderer;

};

#endif // POLYGONDRAWINGINTERACTIONMODE_H
