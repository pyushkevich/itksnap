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

  void paintGL();

protected:

  SnakeModeRenderer();
  virtual ~SnakeModeRenderer() {}

  SnakeWizardModel *m_Model;

  void DrawBubbles();
};

#endif // SNAKEMODERENDERER_H
