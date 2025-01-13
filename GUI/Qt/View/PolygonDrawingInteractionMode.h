#ifndef POLYGONDRAWINGINTERACTIONMODE_H
#define POLYGONDRAWINGINTERACTIONMODE_H

#include <SliceWindowInteractionDelegateWidget.h>
#include <SNAPCommon.h>

class GenericSliceModel;
class PolygonDrawingModel;

class QMenu;
class QAction;

class PolygonDrawingInteractionMode : public SliceWindowInteractionDelegateWidget
{
  Q_OBJECT

public:
  explicit PolygonDrawingInteractionMode(QWidget *parent, QWidget *canvasWidget);
  ~PolygonDrawingInteractionMode();

  irisGetMacro(Model, PolygonDrawingModel *)
  void SetModel(PolygonDrawingModel *m_Model);

  virtual void mousePressEvent(QMouseEvent *ev) override;
  virtual void mouseMoveEvent(QMouseEvent *ev) override;
  virtual void mouseReleaseEvent(QMouseEvent *ev) override;
  virtual void enterEvent(QEnterEvent *) override;
  virtual void leaveEvent(QEvent *) override;

  virtual void contextMenuEvent(QContextMenuEvent *) override;

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

  void onModelUpdate(const EventBucket &bucket) override;


protected:

  PolygonDrawingModel *m_Model;

};

#endif // POLYGONDRAWINGINTERACTIONMODE_H
