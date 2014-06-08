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
  PaintbrushInteractionMode(GenericSliceView *parent = 0);
  ~PaintbrushInteractionMode();

  irisGetMacro(Model, PaintbrushModel *)
  void SetModel(PaintbrushModel *m_Model);

  irisGetMacro(Renderer, PaintbrushRenderer *)


  void mousePressEvent(QMouseEvent *ev);
  void mouseMoveEvent(QMouseEvent *ev);
  void mouseReleaseEvent(QMouseEvent *ev);

  void enterEvent(QEvent *);
  void leaveEvent(QEvent *);

  void keyPressEvent(QKeyEvent *ev);

public slots:

  void onModelUpdate(const EventBucket &bucket);

protected:

  PaintbrushModel *m_Model;
  SmartPtr<PaintbrushRenderer> m_Renderer;
};

#endif // PAINTBRUSHINTERACTIONMODE_H
