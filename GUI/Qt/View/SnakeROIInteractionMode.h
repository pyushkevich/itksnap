#ifndef SNAKEROIINTERACTIONMODE_H
#define SNAKEROIINTERACTIONMODE_H

#include <SliceWindowInteractionDelegateWidget.h>
#include <SNAPCommon.h>

class GenericSliceModel;
class GenericSliceView;
class SnakeROIRenderer;
class SnakeROIModel;

class SnakeROIInteractionMode : public SliceWindowInteractionDelegateWidget
{
  Q_OBJECT

public:
  explicit SnakeROIInteractionMode(GenericSliceView *parent = NULL);
  ~SnakeROIInteractionMode();

  irisGetMacro(Renderer, SnakeROIRenderer *)

  void SetModel(SnakeROIModel *model);

  void mousePressEvent(QMouseEvent *ev);
  void mouseMoveEvent(QMouseEvent *);
  void mouseReleaseEvent(QMouseEvent *);
  void enterEvent(QEvent *);
  void leaveEvent(QEvent *);

protected:

  SmartPtr<SnakeROIRenderer> m_Renderer;

  SnakeROIModel *m_Model;
};

#endif // SNAKEROIINTERACTIONMODE_H
