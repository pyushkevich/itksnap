#ifndef COLORMAPBOX_H
#define COLORMAPBOX_H

#include <QtAbstractOpenGLBox.h>
#include <QtInteractionDelegateWidget.h>
#include <SNAPEvents.h>
#include <ColorMapRenderer.h>

class ColorMapModel;
class ColorMapInteractionDelegate;
class ColorMapInspector;

class ColorMapBox : public QtAbstractOpenGLBox
{
  Q_OBJECT

  FIRES(ModelUpdateEvent)

public:
  explicit ColorMapBox(QWidget *parent = 0);
  virtual ~ColorMapBox();

  // Get the model in this widget
  irisGetMacro(Model, ColorMapModel *)

  // Get the renderer for this widget
  irisGetMacro(Renderer, AbstractRenderer *)

  // Get the interaction delegate for this widget
  irisGetMacro(Delegate, ColorMapInteractionDelegate *)

  // Set the model (state) for this widget
  void SetModel(ColorMapModel *model);

public slots:

  void onModelUpdate(const EventBucket &bucket);

private:

  ColorMapModel *m_Model;
  SmartPtr<ColorMapRenderer> m_Renderer;
  ColorMapInteractionDelegate *m_Delegate;
};


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



#endif // COLORMAPBOX_H
