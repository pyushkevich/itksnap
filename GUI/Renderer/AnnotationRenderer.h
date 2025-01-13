#ifndef ANNOTATIONRENDERER_H
#define ANNOTATIONRENDERER_H

#include "SNAPCommon.h"
#include "GenericSliceRenderer.h"


class AnnotationModel;
class AnnotationContextItem;

class AnnotationRenderer : public SliceRendererDelegate
{
public:
  irisITKObjectMacro(AnnotationRenderer, SliceRendererDelegate)

  void RenderOverTiledLayer(AbstractRenderContext *context,
                            ImageWrapperBase         *base_layer,
                            const SubViewport        &vp) override;

  irisGetSetMacro(Model, AnnotationModel *)

protected:
  AnnotationRenderer() {}
  virtual ~AnnotationRenderer() {}

  void             DrawLineLength(AbstractRenderContext *context,
                                  const Vector3d           &xSlice1,
                                  const Vector3d           &xSlice2,
                                  const Vector3d           &color,
                                  double                    alpha);

  void             DrawSelectionHandle(AbstractRenderContext *context, const Vector3d &xSlice);

  AnnotationModel *m_Model;
};


#endif // ANNOTATIONRENDERER_H
