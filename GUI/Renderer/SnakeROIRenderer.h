#ifndef SNAKEROIRENDERER_H
#define SNAKEROIRENDERER_H

#include "SNAPCommon.h"
#include "GenericSliceRenderer.h"

class SnakeROIModel;

class SnakeROIRenderer : public SliceRendererDelegate
{
public:

  irisITKObjectMacro(SnakeROIRenderer, SliceRendererDelegate)

  irisGetSetMacro(Model, SnakeROIModel *)

  void paintGL();

protected:

  SnakeROIRenderer();
  virtual ~SnakeROIRenderer() {}

  SnakeROIModel *m_Model;
};

#endif // SNAKEROIRENDERER_H
