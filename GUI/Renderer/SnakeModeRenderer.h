#ifndef SNAKEMODERENDERER_H
#define SNAKEMODERENDERER_H

#include "SNAPCommon.h"
#include "GenericSliceRenderer.h"
#include "GenericSliceNewRenderer.h"

class SnakeWizardModel;

/**
  This renderer draws SNAP-specific overlays
  */
class SnakeModeRenderer : public SliceRendererDelegate
{
public:

  irisITKObjectMacro(SnakeModeRenderer, SliceRendererDelegate)
  irisGetSetMacro(Model, GenericSliceModel *)

  virtual void AddContextItemsToTiledOverlay(
      vtkAbstractContextItem *parent, ImageWrapperBase *) override;

protected:

  SnakeModeRenderer();
  virtual ~SnakeModeRenderer() {}

  GenericSliceModel *m_Model;

};

class SnakeModeNewRenderer : public SliceNewRendererDelegate
{
public:
  irisITKObjectMacro(SnakeModeNewRenderer, SliceNewRendererDelegate)

  void RenderOverTiledLayer(AbstractNewRenderContext *context,
                            ImageWrapperBase         *base_layer,
                            const SubViewport        &vp) override;

  irisGetSetMacro(Model, GenericSliceModel *)

protected:
  SnakeModeNewRenderer() {}
  virtual ~SnakeModeNewRenderer() {}

  void DrawBubbles(AbstractNewRenderContext *context);

  GenericSliceModel *m_Model;
};

#endif // SNAKEMODERENDERER_H
