#ifndef SNAKEMODERENDERER_H
#define SNAKEMODERENDERER_H

#include "SNAPCommon.h"
#include "GenericSliceRenderer.h"

class SnakeWizardModel;

/**
  This renderer draws SNAP-specific overlays
  */
class SnakeModeRenderer : public SliceRendererDelegate
{
public:

  irisITKObjectMacro(SnakeModeRenderer, SliceRendererDelegate)
  irisGetSetMacro(Model, SnakeWizardModel *)

  virtual void AddContextItemsToTiledOverlay(
      vtkAbstractContextItem *parent, ImageWrapperBase *) override;

protected:

  SnakeModeRenderer();
  virtual ~SnakeModeRenderer() {}

  SnakeWizardModel *m_Model;

};

#endif // SNAKEMODERENDERER_H
