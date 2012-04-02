#ifndef ABSTRACTRENDERER_H
#define ABSTRACTRENDERER_H

#include "AbstractModel.h"

/**
  A parent class for ITK-SNAP renderers. A renderer implements all the OpenGL
  drawing code independently of the widget system (Qt).
  */
class AbstractRenderer : public AbstractModel
{
public:

  virtual void initializeGL() {}
  virtual void resizeGL(int w, int h) {}
  virtual void paintGL() = 0;

protected:
  AbstractRenderer();
  virtual ~AbstractRenderer() {}


};

#endif // ABSTRACTRENDERER_H
