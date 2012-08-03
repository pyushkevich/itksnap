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

  if(app->IsSnakeModeActive())
    {
    // Get the image data
    SNAPImageData *sid = app->GetSNAPImageData();

    // Depending on the state, draw the speed image
    if(gs->GetShowSpeed())
      {
      GenericSliceRenderer *parent = this->GetParentRenderer();
      std::cout << "Drawing SPEED!!!" << std::endl;
      parent->DrawTextureForLayer(sid->GetSpeed(), false);
      SpeedImageWrapper::DisplayPixelType *p =
          sid->GetSpeed()->GetDisplaySlice(parent->GetModel()->GetSliceDirectionInImageSpace())->GetBufferPointer();
      std::cout << "  p[300] = "
                << p[300].GetRed() << ", "
                << p[300].GetGreen() << ", "
                << p[300].GetBlue() << std::endl;
      }
    }
}
