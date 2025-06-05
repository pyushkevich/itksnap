#include "SnakeModeRenderer.h"
#include "SnakeWizardModel.h"
#include "GenericSliceModel.h"
#include "GlobalUIModel.h"
#include "IRISApplication.h"
#include "GlobalState.h"


void
SnakeModeRenderer::RenderOverTiledLayer(AbstractRenderContext *context,
                                           ImageWrapperBase         *base_layer,
                                           const SubViewport        &vp)
{
  IRISApplication *app = m_Model->GetDriver();
  auto            *snake_wiz_model = m_Model->GetParentUI()->GetSnakeWizardModel();

  if (!vp.isThumbnail && app->IsSnakeModeActive())
  {
    // Bubbles are drawn only when on the bubbles page
    if (snake_wiz_model->CheckState(SnakeWizardModel::UIF_BUBBLE_MODE))
    {
      // Draw the bubbles before segmentation starts
      this->DrawBubbles(context);
    }
  }
}

void
SnakeModeRenderer::DrawBubbles(AbstractRenderContext *context)
{
  IRISApplication *app = m_Model->GetDriver();
  GlobalState     *gs = app->GetGlobalState();
  auto            *snake_wiz_model = m_Model->GetParentUI()->GetSnakeWizardModel();

  // Get the list of bubbles
  IRISApplication::BubbleArray &bubbles = app->GetBubbleArray();

  // draw bubbles
  int numBubbles = bubbles.size();
  int activeBubble = gs->GetActiveBubble();

  if (numBubbles > 0)
  {
    // Get the active color label
    int        currentcolor = gs->GetDrawingColorLabel();
    ColorLabel cl = app->GetColorLabelTable()->GetColorLabel(currentcolor);

    // Get the current alpha blending factor for displaying overlays
    double alpha = gs->GetSegmentationAlpha();

    // Get the color of the active color label
    Vector3d clrFill = cl.GetRGBAsDoubleVector();
    Vector3d clrWhite(1.0, 1.0, 1.0);
    Vector3d clrLine = clrWhite - (clrWhite - clrFill) / 2.0;

    // Get the current crosshairs position
    Vector3d cursorImage = to_double(app->GetCursorPosition()) + Vector3d(0.5);

    // Get the image space dimension that corresponds to this window
    int iid = m_Model->GetSliceDirectionInImageSpace();

    // Get the other essentials from the parent
    Vector3d scaling = m_Model->GetSliceSpacing();

    // Draw each bubble
    for (int i = 0; i < numBubbles; i++)
    {
      // Get the center and radius of the i-th bubble
      Vector3d ctrImage = to_double(bubbles[i].center) + Vector3d(0.5);
      double   radius = bubbles[i].radius;

      // Remap the center into slice coordinates
      Vector3d ctrSlice = m_Model->MapImageToSlice(ctrImage);

      // Compute the offset from the center along the slice z-direction
      // in physical coordinates
      double dcenter = scaling(2) * (cursorImage(iid) - ctrImage(iid));

      // Check if the bubble is intersected by the current slice plane
      if (dcenter >= radius || -dcenter >= radius)
        continue;

      // Compute the radius of the bubble in the cut plane
      double diskradius = sqrt(fabs(radius * radius - dcenter * dcenter));

      // Set brush settings
      context->SetBrush(clrFill, 0.5 * alpha);
      if (i == activeBubble)
      {
        context->SetPenColor(clrLine, 1.0);
        context->SetPenWidth(1.5);
        context->SetPenLineType(vtkPen::SOLID_LINE);
      }
      else
      {
        context->SetPenLineType(vtkPen::NO_PEN);
      }

      // Draw the bubble
      context->DrawEllipse(ctrSlice[0], ctrSlice[1], diskradius / scaling[0], diskradius / scaling[1]);
    }
  }
}
