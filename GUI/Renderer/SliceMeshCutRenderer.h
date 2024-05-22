#ifndef SLICEMESHCUTRENDERER_H
#define SLICEMESHCUTRENDERER_H

#include "GenericSliceRenderer.h"
#include "Generic3DModel.h"

class SliceMeshCutRenderer : public SliceRendererDelegate
{
public:
  irisITKObjectMacro(SliceMeshCutRenderer, SliceRendererDelegate)

  irisGetSetMacro(Model, Generic3DModel *)

  virtual void AddContextItemsToTiledOverlay(
      vtkAbstractContextItem *parent, ImageWrapperBase *) override;

protected:
  SliceMeshCutRenderer();
  virtual ~SliceMeshCutRenderer() {}

  Generic3DModel *m_Model;

};

#endif // SLICEMESHCUTRENDERER_H
