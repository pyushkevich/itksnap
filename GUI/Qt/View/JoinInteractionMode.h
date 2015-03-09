#ifndef JOININTERACTIONMODE_H
#define JOININTERACTIONMODE_H

#include <SliceWindowInteractionDelegateWidget.h>
#include <SNAPCommon.h>
#include "GenericSliceView.h"
#include "JoinModel.h"

class GenericSliceModel;
class GenericSliceView;
class JoinModel;

class JoinInteractionMode : public SliceWindowInteractionDelegateWidget
{
  Q_OBJECT

public:
  JoinInteractionMode(GenericSliceView *parent = 0);
  ~JoinInteractionMode();

  irisGetMacro(Model, JoinModel *)
  void SetModel(JoinModel *m_Model);

  void mousePressEvent(QMouseEvent *ev);

  void enterEvent(QEvent *);
  void leaveEvent(QEvent *);

protected:

  JoinModel *m_Model;
};

#endif // JOININTERACTIONMODE_H
