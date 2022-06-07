#ifndef PAINTBRUSHRENDERER_H
#define PAINTBRUSHRENDERER_H

#include "GenericSliceRenderer.h"

class PaintbrushModel;

class PaintbrushRenderer : public SliceRendererDelegate
{
public:

  irisITKObjectMacro(PaintbrushRenderer, SliceRendererDelegate)

  irisGetSetMacro(Model, PaintbrushModel *)

  virtual void AddContextItemsToTiledOverlay(
      vtkAbstractContextItem *parent, ImageWrapperBase *) override;

protected:

  PaintbrushRenderer() {};
  virtual ~PaintbrushRenderer() {}

  PaintbrushModel *m_Model;

};

#endif // PAINTBRUSHRENDERER_H
