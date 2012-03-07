#ifndef COLORMAPBOX_H
#define COLORMAPBOX_H

#include <SNAPQGLWidget.h>
#include <QtInteractionDelegateWidget.h>
#include <SNAPEvents.h>

class ColorMapModel;
class ColorMapRenderer;
class ColorMapInteractionDelegate;

class ColorMapBox : public SNAPQGLWidget
{
  Q_OBJECT

  FIRES(ModelUpdateEvent)

public:
  explicit ColorMapBox(QWidget *parent = 0);
  virtual ~ColorMapBox();

  // Get the model in this widget
  irisGetMacro(Model, ColorMapModel *)

  // Get the renderer for this widget
  irisGetMacro(Renderer, ColorMapRenderer *)

  // Set the model (state) for this widget
  void SetModel(ColorMapModel *model);

  void paintGL();
  void resizeGL(int w, int h);

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

  // Set the model (state) for this widget
  irisGetSetMacro(Model, ColorMapModel *)

private:

  ColorMapModel *m_Model;
};



#endif // COLORMAPBOX_H
