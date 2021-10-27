#ifndef SNAKEROIRENDERER_H
#define SNAKEROIRENDERER_H

#include "SNAPCommon.h"
#include "GenericSliceRenderer.h"

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

#endif // SNAKEROIRENDERER_H
