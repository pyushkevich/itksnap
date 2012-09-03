#include "SnakeModeRenderer.h"
#include "SnakeWizardModel.h"
#include "GenericSliceModel.h"
#include "GlobalUIModel.h"
#include "IRISApplication.h"
#include "SNAPImageData.h"
#include "GlobalState.h"

SnakeModeRenderer::SnakeModeRenderer()
{
  m_Model = NULL;
}

#include "itkImage.h"

void SnakeModeRenderer::paintGL()
{
  IRISApplication *app = m_Model->GetParent()->GetDriver();
  GlobalState *gs = app->GetGlobalState();

  if(app->IsSnakeModeActive() && !this->GetParentRenderer()->IsThumbnailDrawing())
    {
    // Get the image data
    SNAPImageData *sid = app->GetSNAPImageData();

    // The speed image is rendered when the speed volume is marked as valid
    // in the global state (indicating that the speed volume has been computed)
    // or if the speed image is hooked up to the preview pipeline
    if(gs->GetSpeedValid() || sid->GetSpeed()->IsPreviewPipelineAttached())
      {
      GenericSliceRenderer *parent = this->GetParentRenderer();
      parent->DrawTextureForLayer(sid->GetSpeed(), true);
      }
    }
}
