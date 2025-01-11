#ifndef ANNOTATIONRENDERER_H
#define ANNOTATIONRENDERER_H

#include "SNAPCommon.h"
#include "GenericSliceRenderer.h"
#include "GenericSliceNewRenderer.h"


class AnnotationModel;
class AnnotationContextItem;

class AnnotationRenderer : public SliceRendererDelegate
{
public:

  irisITKObjectMacro(AnnotationRenderer, SliceRendererDelegate)

  irisGetMacro(Model, AnnotationModel *)
  void SetModel(AnnotationModel *model);

  virtual void AddContextItemsToTiledOverlay(
      vtkAbstractContextItem *parent, ImageWrapperBase *base_layer) override;

protected:

  AnnotationRenderer();
  virtual ~AnnotationRenderer() {}

  AnnotationModel *m_Model;

};



class AnnotationNewRenderer : public SliceNewRendererDelegate
{
public:
  irisITKObjectMacro(AnnotationNewRenderer, SliceNewRendererDelegate)

  void RenderOverTiledLayer(AbstractNewRenderContext *context,
                            ImageWrapperBase         *base_layer,
                            const SubViewport        &vp) override;

  irisGetSetMacro(Model, AnnotationModel *)

protected:
  AnnotationNewRenderer() {}
  virtual ~AnnotationNewRenderer() {}

  void             DrawLineLength(AbstractNewRenderContext *context,
                                  const Vector3d           &xSlice1,
                                  const Vector3d           &xSlice2,
                                  const Vector3d           &color,
                                  double                    alpha);

  void             DrawSelectionHandle(AbstractNewRenderContext *context, const Vector3d &xSlice);

  AnnotationModel *m_Model;
};


#endif // ANNOTATIONRENDERER_H
