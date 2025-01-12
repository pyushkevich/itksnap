#ifndef SNAKEROIINTERACTIONMODE_H
#define SNAKEROIINTERACTIONMODE_H

#include <SliceWindowInteractionDelegateWidget.h>
#include <SNAPCommon.h>

class GenericSliceModel;
class SnakeROIRenderer;
class SnakeROIModel;

class SnakeROIInteractionMode : public SliceWindowInteractionDelegateWidget
{
  Q_OBJECT

public:
  explicit SnakeROIInteractionMode(QWidget *parent, QWidget *canvasWidget);
  ~SnakeROIInteractionMode();

  irisGetMacro(Renderer, SnakeROIRenderer *)

  void SetModel(SnakeROIModel *model);

  virtual void mousePressEvent(QMouseEvent *ev) override;
  virtual void mouseMoveEvent(QMouseEvent *) override;
  virtual void mouseReleaseEvent(QMouseEvent *) override;
  virtual void enterEvent(QEnterEvent *) override;
  virtual void leaveEvent(QEvent *) override;

protected:

  SmartPtr<SnakeROIRenderer> m_Renderer;

  SnakeROIModel *m_Model;
};

#endif // SNAKEROIINTERACTIONMODE_H
