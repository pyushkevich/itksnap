#ifndef SNAKEROIRENDERER_H
#define SNAKEROIRENDERER_H

#include "SNAPCommon.h"
#include "GenericSliceRenderer.h"
#include "GenericSliceNewRenderer.h"


class SnakeROIModel;

class SnakeROIRenderer : public SliceRendererDelegate
{
public:

  irisITKObjectMacro(SnakeROIRenderer, SliceRendererDelegate)

  irisGetSetMacro(Model, SnakeROIModel *)

  virtual void AddContextItemsToTiledOverlay(
      vtkAbstractContextItem *parent, ImageWrapperBase *) override;

protected:

  SnakeROIRenderer();
  virtual ~SnakeROIRenderer() {}

  SnakeROIModel *m_Model;
};


class SnakeROINewRenderer : public SliceNewRendererDelegate
{
public:
  irisITKObjectMacro(SnakeROINewRenderer, SliceNewRendererDelegate)

  void RenderOverTiledLayer(AbstractNewRenderContext *context,
                            ImageWrapperBase         *base_layer,
                            const SubViewport        &vp) override;

  irisGetSetMacro(Model, SnakeROIModel *)

protected:
  SnakeROINewRenderer() {}
  virtual ~SnakeROINewRenderer() {}


  SnakeROIModel *m_Model;
};


#endif // SNAKEROIRENDERER_H
