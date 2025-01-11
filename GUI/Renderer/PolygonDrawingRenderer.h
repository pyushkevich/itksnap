#ifndef POLYGONDRAWINGRENDERER_H
#define POLYGONDRAWINGRENDERER_H

#include "SNAPCommon.h"
#include "GenericSliceRenderer.h"
#include "GenericSliceNewRenderer.h"
#include "PolygonDrawingModel.h"


class PolygonDrawingModel;

class PolygonDrawingRenderer : public SliceRendererDelegate
{
public:

  irisITKObjectMacro(PolygonDrawingRenderer, SliceRendererDelegate)

  irisGetSetMacro(Model, PolygonDrawingModel *)

  virtual void AddContextItemsToTiledOverlay(
      vtkAbstractContextItem *parent, ImageWrapperBase *) override;

protected:

  PolygonDrawingRenderer();
  virtual ~PolygonDrawingRenderer() {}

  PolygonDrawingModel *m_Model;
};

class PolygonDrawingNewRenderer : public SliceNewRendererDelegate
{
public:
  irisITKObjectMacro(PolygonDrawingNewRenderer, SliceNewRendererDelegate)

  void RenderOverTiledLayer(AbstractNewRenderContext *context,
                            ImageWrapperBase         *base_layer,
                            const SubViewport        &vp) override;

  irisGetSetMacro(Model, PolygonDrawingModel *)

protected:
  PolygonDrawingNewRenderer() {}
  virtual ~PolygonDrawingNewRenderer() {}

  PolygonDrawingModel *m_Model;

  void DrawVertices(AbstractNewRenderContext              *context,
                    const PolygonDrawingModel::VertexList &vx,
                    bool                                   closed = false);

  void DrawSelectionBox(AbstractNewRenderContext           *context,
                        const PolygonDrawingModel::BoxType &box,
                        double                              border_x,
                        double                              border_y);
};

#endif // POLYGONDRAWINGRENDERER_H
