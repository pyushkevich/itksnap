#ifndef POLYGONDRAWINGRENDERER_H
#define POLYGONDRAWINGRENDERER_H

#include "SNAPCommon.h"
#include "GenericSliceRenderer.h"

class PolygonDrawingModel;

class PolygonDrawingRenderer : public SliceRendererDelegate
{
public:

  irisITKObjectMacro(PolygonDrawingRenderer, SliceRendererDelegate)

  void paintGL();

  irisGetMacro(Model, PolygonDrawingModel *)
  irisSetMacro(Model, PolygonDrawingModel *)


protected:

  PolygonDrawingRenderer();
  virtual ~PolygonDrawingRenderer() {}

  void DrawBox(const vnl_vector_fixed<float, 4> &box,
               float border_x = 0.0f, float border_y = 0.0f);

  PolygonDrawingModel *m_Model;

  // Colors used to draw polygon
  const static float m_DrawingModeColor[];
  const static float m_EditModeSelectedColor[];
  const static float m_EditModeNormalColor[];
};

#endif // POLYGONDRAWINGRENDERER_H
