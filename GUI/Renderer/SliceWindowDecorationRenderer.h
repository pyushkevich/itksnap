#ifndef SLICEWINDOWDECORATIONRENDERER_H
#define SLICEWINDOWDECORATIONRENDERER_H

#include "SNAPCommon.h"
#include "GenericSliceRenderer.h"

class GenericSliceRenderer;
class GenericSliceModel;

class SliceWindowDecorationRenderer : public SliceRendererDelegate
{
public:

  irisITKObjectMacro(SliceWindowDecorationRenderer, SliceRendererDelegate)

  virtual void paintGL();

protected:

  void DrawOrientationLabels();

  void DrawRulers();

  void DrawNicknames();

  SliceWindowDecorationRenderer();
  virtual ~SliceWindowDecorationRenderer();
};

#endif // SLICEWINDOWDECORATIONRENDERER_H
