#ifndef QTSIMPLEOPENGLBOX_H
#define QTSIMPLEOPENGLBOX_H

#include <QtAbstractOpenGLBox.h>

/**
  This is an actual implementation of a SNAPQGLWidget that can be inserted
  into a widget without having to create a separate custom widget. The user
  must supply a renderer, which in turn must fire ModelUpdateEvent() to force
  this widget to redraw.

  TODO: add interaction support
  */
class QtSimpleOpenGLBox : public QtAbstractOpenGLBox
{
  Q_OBJECT

public:

  explicit QtSimpleOpenGLBox(QWidget *parent = 0);

  // Set the renderer in this widget
  void SetRenderer(AbstractRenderer *renderer);

  // Get the renderer for this widget
  irisGetMacro(Renderer, AbstractRenderer *)

public slots:

  virtual void onModelUpdate(const EventBucket &bucket);

protected:

  AbstractRenderer *m_Renderer;

};

#endif // QTSIMPLEOPENGLBOX_H
