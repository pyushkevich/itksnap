#ifndef PAINTBRUSHRENDERER_H
#define PAINTBRUSHRENDERER_H

#include "GenericSliceRenderer.h"

class PaintbrushModel;

class PaintbrushRenderer : public SliceRendererDelegate
{
public:
  irisITKObjectMacro(PaintbrushRenderer, SliceRendererDelegate)

  void RenderOverTiledLayer(AbstractRenderContext *context,
                            ImageWrapperBase         *base_layer,
                            const SubViewport        &vp) override;

  irisGetSetMacro(Model, PaintbrushModel *)

protected:
  PaintbrushRenderer() {}
  virtual ~PaintbrushRenderer() {}

  void BuildBrush(AbstractRenderContext *context);

  // The path for the brush outline, and the outline it was built from
  AbstractRenderContext::Path2DPtr m_BrushOutlinePath;
  AbstractRenderContext::VertexVector m_PathOutline;
  PaintbrushModel *m_Model;
};


#endif // PAINTBRUSHRENDERER_H
