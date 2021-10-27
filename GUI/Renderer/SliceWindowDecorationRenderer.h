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

  virtual void AddContextItemsToGlobalOverlayScene(
      vtkContextScene *) override;

  irisGetSetMacro(Model, GenericSliceModel *)

protected:

  SliceWindowDecorationRenderer();
  virtual ~SliceWindowDecorationRenderer();

  GenericSliceModel *m_Model;
};

#endif // SLICEWINDOWDECORATIONRENDERER_H
