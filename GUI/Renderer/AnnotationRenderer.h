#ifndef ANNOTATIONRENDERER_H
#define ANNOTATIONRENDERER_H

#include "SNAPCommon.h"
#include "GenericSliceRenderer.h"


class AnnotationModel;

class AnnotationRenderer : public SliceRendererDelegate
{
public:

  irisITKObjectMacro(AnnotationRenderer, SliceRendererDelegate)

  virtual void paintGL() ITK_OVERRIDE;

  irisGetMacro(Model, AnnotationModel *)

  void SetModel(AnnotationModel *model);

protected:

  AnnotationRenderer();
  virtual ~AnnotationRenderer() {}

  AnnotationModel *m_Model;
  void DrawLineLength(const Vector3d &xSlice1, const Vector3d &xSlice2,
                      const Vector3d &color, double alpha);

  void DrawSelectionHandle(const Vector3d &xSlice);
};

#endif // ANNOTATIONRENDERER_H
