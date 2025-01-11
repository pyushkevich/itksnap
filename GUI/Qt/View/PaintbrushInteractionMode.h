#ifndef PAINTBRUSHINTERACTIONMODE_H
#define PAINTBRUSHINTERACTIONMODE_H

#include <SliceWindowInteractionDelegateWidget.h>
#include <SNAPCommon.h>

class GenericSliceModel;
class GenericSliceView;
class PaintbrushModel;
class PaintbrushRenderer;

class PaintbrushInteractionMode : public SliceWindowInteractionDelegateWidget
{
  Q_OBJECT

public:
  PaintbrushInteractionMode(QWidget *parent, QWidget *canvasWidget);
  ~PaintbrushInteractionMode();

  irisGetMacro(Model, PaintbrushModel *)
  void SetModel(PaintbrushModel *m_Model);

  irisGetMacro(Renderer, PaintbrushRenderer *)


  virtual void mousePressEvent(QMouseEvent *ev) override;
  virtual void mouseMoveEvent(QMouseEvent *ev) override;
  virtual void mouseReleaseEvent(QMouseEvent *ev) override;

  virtual void enterEvent(QEnterEvent *) override;
  virtual void leaveEvent(QEvent *) override;

  virtual void keyPressEvent(QKeyEvent *ev) override;

public slots:

  virtual void onModelUpdate(const EventBucket &bucket) override;

protected:

  PaintbrushModel *m_Model;
  SmartPtr<PaintbrushRenderer> m_Renderer;
};

#endif // PAINTBRUSHINTERACTIONMODE_H
