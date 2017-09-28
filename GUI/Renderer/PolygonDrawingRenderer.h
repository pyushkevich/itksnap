#ifndef POLYGONDRAWINGRENDERER_H
#define POLYGONDRAWINGRENDERER_H

#include "SNAPCommon.h"
#include "GenericSliceRenderer.h"

class PolygonDrawingModel;

class PolygonDrawingRenderer : public SliceRendererDelegate
{
public:

  irisITKObjectMacro(PolygonDrawingRenderer, SliceRendererDelegate)

  void paintGL() ITK_OVERRIDE;

  irisGetMacro(Model, PolygonDrawingModel *)
  irisSetMacro(Model, PolygonDrawingModel *)


protected:

  PolygonDrawingRenderer();
  virtual ~PolygonDrawingRenderer() {}

  void DrawBox(const vnl_vector_fixed<double, 4> &box,
               double border_x = 0.0, double border_y = 0.0);

  PolygonDrawingModel *m_Model;

  // Colors used to draw polygon
  const static double m_DrawingModeColor[];
  const static double m_EditModeSelectedColor[];
  const static double m_EditModeNormalColor[];
};

#endif // POLYGONDRAWINGRENDERER_H
