#ifndef DEFORMATIONGRIDRENDERER_H
#define DEFORMATIONGRIDRENDERER_H

#include "DeformationGridModel.h"
#include "GenericSliceRenderer.h"

class DeformationGridRenderer : public SliceRendererDelegate
{
public:
  using Path2D = typename AbstractRenderContext::Path2D;

  irisITKObjectMacro(DeformationGridRenderer, SliceRendererDelegate)

  void RenderOverTiledLayer(AbstractRenderContext *context,
                            ImageWrapperBase         *base_layer,
                            const SubViewport        &vp) override;

  irisGetSetMacro(Model, DeformationGridModel *)

protected:
  DeformationGridRenderer();
  virtual ~DeformationGridRenderer() {}

  void AddLine(AbstractRenderContext *context,
               Path2D *path,
               std::vector<double> &verts,
               size_t skip, size_t l, size_t nv, bool reverse);


  DeformationGridModel *m_Model;
};


#endif // DEFORMATIONGRIDRENDERER_H
