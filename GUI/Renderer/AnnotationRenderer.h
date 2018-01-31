#ifndef ANNOTATIONRENDERER_H
#define ANNOTATIONRENDERER_H

#include "SNAPCommon.h"
#include "GenericSliceRenderer.h"


class AnnotationModel;

class AnnotationRenderer : public SliceRendererDelegate
{
public:

  irisITKObjectMacro(AnnotationRenderer, SliceRendererDelegate)

  virtual void paintGL();

  irisGetMacro(Model, AnnotationModel *)
  irisSetMacro(Model, AnnotationModel *)

protected:

  AnnotationRenderer();
  virtual ~AnnotationRenderer() {}

  AnnotationModel *m_Model;
  void DrawLineLength(const Vector3f &xSlice1, const Vector3f &xSlice2,
                      const Vector3d &color, double alpha);

  void DrawSelectionHandle(const Vector3f &xSlice);
};

#endif // ANNOTATIONRENDERER_H
