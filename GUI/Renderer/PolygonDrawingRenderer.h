#ifndef POLYGONDRAWINGRENDERER_H
#define POLYGONDRAWINGRENDERER_H

#include "SNAPCommon.h"
#include "GenericSliceRenderer.h"
#include "PolygonDrawingModel.h"


class PolygonDrawingModel;

class PolygonDrawingRenderer : public SliceRendererDelegate
{
public:
  irisITKObjectMacro(PolygonDrawingRenderer, SliceRendererDelegate)

  void RenderOverTiledLayer(AbstractRenderContext *context,
                            ImageWrapperBase         *base_layer,
                            const SubViewport        &vp) override;

  irisGetSetMacro(Model, PolygonDrawingModel *)

protected:
  PolygonDrawingRenderer() {}
  virtual ~PolygonDrawingRenderer() {}

  PolygonDrawingModel *m_Model;

  void DrawVertices(AbstractRenderContext              *context,
                    const PolygonDrawingModel::VertexList &vx,
                    bool                                   closed = false);

  void DrawSelectionBox(AbstractRenderContext           *context,
                        const PolygonDrawingModel::BoxType &box,
                        double                              border_x,
                        double                              border_y);
};

#endif // POLYGONDRAWINGRENDERER_H
