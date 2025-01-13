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

  void InsertWithCollinearCheck(const Vector2d &head);
  void BuildBrush(AbstractRenderContext *context);

  // Paintbrush settings corresponding to the current walk
  PaintbrushSettings m_CachedBrushSettings;

  AbstractRenderContext::Path2DPtr m_BrushOutlinePath;
  AbstractRenderContext::VertexVector m_BrushOutline;
  PaintbrushModel *m_Model;
};


#endif // PAINTBRUSHRENDERER_H
