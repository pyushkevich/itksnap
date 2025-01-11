#ifndef SLICEWINDOWDECORATIONRENDERER_H
#define SLICEWINDOWDECORATIONRENDERER_H

#include "SNAPCommon.h"
#include "GenericSliceRenderer.h"
#include "GenericSliceNewRenderer.h"

class GenericSliceRenderer;
class GenericSliceModel;

class SliceWindowDecorationRenderer : public SliceRendererDelegate
{
public:

  irisITKObjectMacro(SliceWindowDecorationRenderer, SliceRendererDelegate)

  virtual void AddContextItemsToGlobalOverlayScene(
      vtkContextScene *) override;

  irisGetSetMacro(Model, GenericSliceModel *)

protected:

  SliceWindowDecorationRenderer();
  virtual ~SliceWindowDecorationRenderer();

  GenericSliceModel *m_Model;
};

class SliceWindowDecorationNewRenderer : public SliceNewRendererDelegate
{
public:
  irisITKObjectMacro(SliceWindowDecorationNewRenderer, SliceNewRendererDelegate)

  void RenderOverMainViewport(AbstractNewRenderContext *context) override;
  void RenderOverTiledLayer(AbstractNewRenderContext *context,
                            ImageWrapperBase         *base_layer,
                            const SubViewport        &vp) override;

  irisGetSetMacro(Model, GenericSliceModel *)

protected:
  SliceWindowDecorationNewRenderer() {}
  virtual ~SliceWindowDecorationNewRenderer() {}

  void DrawRuler(AbstractNewRenderContext *context);
  void DrawNicknames(AbstractNewRenderContext *context);
  void DrawOrientationLabels(AbstractNewRenderContext *context);
  std::list<std::string> GetDisplayText(ImageWrapperBase *layer);
  std::string        CapStringLength(const std::string &str, size_t max_size);

  GenericSliceModel *m_Model;
};


#endif // SLICEWINDOWDECORATIONRENDERER_H
