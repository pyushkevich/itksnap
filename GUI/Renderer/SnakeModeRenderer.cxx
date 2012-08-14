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

    // Depending on the state, draw the speed image
    if(gs->GetShowSpeed())
      {
      GenericSliceRenderer *parent = this->GetParentRenderer();
      parent->DrawTextureForLayer(sid->GetSpeed(), true);
      }
    }
}
