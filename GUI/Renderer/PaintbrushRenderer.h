#ifndef PAINTBRUSHRENDERER_H
#define PAINTBRUSHRENDERER_H

#include "GenericSliceRenderer.h"

class PaintbrushContextItem;
class PaintbrushModel;

class PaintbrushRenderer : public SliceRendererDelegate
{
public:

  irisITKObjectMacro(PaintbrushRenderer, SliceRendererDelegate)

  irisGetMacro(Model, PaintbrushModel *)
  virtual void SetModel(PaintbrushModel *model);

  void AddContextItemsToTiledOverlay(vtkAbstractContextItem *parent) override;

  void paintGL() ITK_OVERRIDE {};

protected:

  PaintbrushRenderer() {};
  virtual ~PaintbrushRenderer() {}

  PaintbrushModel *m_Model;

  // Context item used to draw crosshairs
  vtkSmartPointer<PaintbrushContextItem> m_ContextItem;

};

#endif // PAINTBRUSHRENDERER_H
