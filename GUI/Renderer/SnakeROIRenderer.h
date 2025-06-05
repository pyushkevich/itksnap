#ifndef SNAKEROIRENDERER_H
#define SNAKEROIRENDERER_H

#include "SNAPCommon.h"
#include "GenericSliceRenderer.h"

class SnakeROIModel;

class SnakeROIRenderer : public SliceRendererDelegate
{
public:
  irisITKObjectMacro(SnakeROIRenderer, SliceRendererDelegate)

  void RenderOverTiledLayer(AbstractRenderContext *context,
                            ImageWrapperBase         *base_layer,
                            const SubViewport        &vp) override;

  irisGetSetMacro(Model, SnakeROIModel *)

protected:
  SnakeROIRenderer() {}
  virtual ~SnakeROIRenderer() {}


  SnakeROIModel *m_Model;
};


#endif // SNAKEROIRENDERER_H
