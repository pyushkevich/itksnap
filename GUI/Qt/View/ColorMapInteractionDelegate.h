#ifndef COLORMAPINTERACTIONDELEGATE_H
#define COLORMAPINTERACTIONDELEGATE_H

#include <QtInteractionDelegateWidget.h>
#include <SNAPEvents.h>
#include <ColorMapRenderer.h>

class ColorMapModel;
class ColorMapInspector;

class ColorMapInteractionDelegate : public QtInteractionDelegateWidget
{
  Q_OBJECT

public:
  explicit ColorMapInteractionDelegate(QWidget *parent = 0);

  void mousePressEvent(QMouseEvent *);
  void mouseReleaseEvent(QMouseEvent *);
  void mouseMoveEvent(QMouseEvent *);
  void mouseDoubleClickEvent(QMouseEvent *);

  // Set the model (state) for this widget
  irisGetSetMacro(Model, ColorMapModel *)

  // Set the inspector widget
  irisGetSetMacro(InspectorWidget, ColorMapInspector *)

private:

  ColorMapModel *m_Model;
  ColorMapInspector *m_InspectorWidget;
};



#endif // COLORMAPINTERACTIONDELEGATE_H
