#ifndef SLICEWINDOWDECORATIONRENDERER_H
#define SLICEWINDOWDECORATIONRENDERER_H

#include "SNAPCommon.h"
#include "GenericSliceRenderer.h"

class GenericSliceModel;

class SliceWindowDecorationRenderer : public SliceRendererDelegate
{
public:
  irisITKObjectMacro(SliceWindowDecorationRenderer, SliceRendererDelegate)

  void RenderOverMainViewport(AbstractRenderContext *context) override;
  void RenderOverTiledLayer(AbstractRenderContext *context,
                            ImageWrapperBase         *base_layer,
                            const SubViewport        &vp) override;

  irisGetSetMacro(Model, GenericSliceModel *)

protected:
  SliceWindowDecorationRenderer() {}
  virtual ~SliceWindowDecorationRenderer() {}

  void DrawRuler(AbstractRenderContext *context);
  void DrawNicknames(AbstractRenderContext *context);
  void DrawOrientationLabels(AbstractRenderContext *context);
  std::list<std::string> GetDisplayText(ImageWrapperBase *layer);
  std::string        CapStringLength(const std::string &str, size_t max_size);

  GenericSliceModel *m_Model;
};


#endif // SLICEWINDOWDECORATIONRENDERER_H
