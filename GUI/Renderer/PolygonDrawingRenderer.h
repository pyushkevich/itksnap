#ifndef POLYGONDRAWINGRENDERER_H
#define POLYGONDRAWINGRENDERER_H

#include "SNAPCommon.h"
#include "GenericSliceRenderer.h"

class PolygonDrawingModel;

class PolygonDrawingRenderer : public SliceRendererDelegate
{
public:

  irisITKObjectMacro(PolygonDrawingRenderer, SliceRendererDelegate)

  irisGetSetMacro(Model, PolygonDrawingModel *)

  virtual void AddContextItemsToTiledOverlay(
      vtkAbstractContextItem *parent, ImageWrapperBase *) override;

protected:

  PolygonDrawingRenderer();
  virtual ~PolygonDrawingRenderer() {}

  PolygonDrawingModel *m_Model;
};

#endif // POLYGONDRAWINGRENDERER_H
