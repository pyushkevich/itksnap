#ifndef SNAKEMODERENDERER_H
#define SNAKEMODERENDERER_H

#include "SNAPCommon.h"
#include "GenericSliceRenderer.h"

class SnakeWizardModel;

class SnakeModeRenderer : public SliceRendererDelegate
{
public:
  irisITKObjectMacro(SnakeModeRenderer, SliceRendererDelegate)

  void RenderOverTiledLayer(AbstractRenderContext *context,
                            ImageWrapperBase         *base_layer,
                            const SubViewport        &vp) override;

  irisGetSetMacro(Model, GenericSliceModel *)

protected:
  SnakeModeRenderer() {}
  virtual ~SnakeModeRenderer() {}

  void DrawBubbles(AbstractRenderContext *context);

  GenericSliceModel *m_Model;
};

#endif // SNAKEMODERENDERER_H
