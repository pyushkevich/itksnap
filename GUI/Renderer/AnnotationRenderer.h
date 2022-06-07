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

  irisGetMacro(Model, AnnotationModel *)
  void SetModel(AnnotationModel *model);

  virtual void AddContextItemsToTiledOverlay(
      vtkAbstractContextItem *parent, ImageWrapperBase *base_layer) override;

protected:

  AnnotationRenderer();
  virtual ~AnnotationRenderer() {}

  AnnotationModel *m_Model;

};

#endif // ANNOTATIONRENDERER_H
