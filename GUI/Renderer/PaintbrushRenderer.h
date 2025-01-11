#ifndef PAINTBRUSHRENDERER_H
#define PAINTBRUSHRENDERER_H

#include "GenericSliceRenderer.h"
#include "GenericSliceNewRenderer.h"

class PaintbrushModel;

class PaintbrushRenderer : public SliceRendererDelegate
{
public:

  irisITKObjectMacro(PaintbrushRenderer, SliceRendererDelegate)

  irisGetSetMacro(Model, PaintbrushModel *)

  virtual void AddContextItemsToTiledOverlay(
      vtkAbstractContextItem *parent, ImageWrapperBase *) override;

protected:

  PaintbrushRenderer() {};
  virtual ~PaintbrushRenderer() {}

  PaintbrushModel *m_Model;

};


class PaintbrushNewRenderer : public SliceNewRendererDelegate
{
public:
  irisITKObjectMacro(PaintbrushNewRenderer, SliceNewRendererDelegate)

  void RenderOverTiledLayer(AbstractNewRenderContext *context,
                            ImageWrapperBase         *base_layer,
                            const SubViewport        &vp) override;

  irisGetSetMacro(Model, PaintbrushModel *)

protected:
  PaintbrushNewRenderer() {}
  virtual ~PaintbrushNewRenderer() {}

  void InsertWithCollinearCheck(const Vector2d &head);
  void BuildBrush(AbstractNewRenderContext *context);

  // Paintbrush settings corresponding to the current walk
  PaintbrushSettings m_CachedBrushSettings;

  AbstractNewRenderContext::Path2DPtr m_BrushOutlinePath;
  AbstractNewRenderContext::VertexVector m_BrushOutline;
  PaintbrushModel *m_Model;
};


#endif // PAINTBRUSHRENDERER_H
