#ifndef PAINTBRUSHRENDERER_H
#define PAINTBRUSHRENDERER_H

#include "GenericSliceRenderer.h"

class PaintbrushModel;

class PaintbrushRenderer : public SliceRendererDelegate
{
public:

  irisITKObjectMacro(PaintbrushRenderer, SliceRendererDelegate)

  irisGetSetMacro(Model, PaintbrushModel *)

  void paintGL();

protected:


  PaintbrushRenderer();

  PaintbrushModel *m_Model;

  void BuildBrush();

  // Representation of the brush
  std::list<Vector2d> m_Walk;
};

#endif // PAINTBRUSHRENDERER_H
