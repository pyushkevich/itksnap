#include "SnakeModeRenderer.h"
#include "SnakeWizardModel.h"
#include "GenericSliceModel.h"
#include "GlobalUIModel.h"
#include "IRISException.h"
#include "IRISApplication.h"
#include "SNAPImageData.h"
#include "GlobalState.h"
#include "itkImage.h"

SnakeModeRenderer::SnakeModeRenderer()
{
  m_Model = NULL;
}

void SnakeModeRenderer::paintGL()
{
  IRISApplication *app = m_Model->GetParent()->GetDriver();

  if(app->IsSnakeModeActive()
     && !this->GetParentRenderer()->IsDrawingZoomThumbnail()
     && !this->GetParentRenderer()->IsDrawingLayerThumbnail())
    {
    // Bubbles are drawn only when on the bubbles page
    if(m_Model->CheckState(SnakeWizardModel::UIF_BUBBLE_MODE))
      {
      // Draw the bubbles before segmentation starts
      this->DrawBubbles();
      }
    }
}

void SnakeModeRenderer::DrawBubbles()
{
  const GLubyte stipple[] = {
    0xaa, 0xaa, 0xaa, 0xaa, 0x55, 0x55, 0x55, 0x55,
    0xaa, 0xaa, 0xaa, 0xaa, 0x55, 0x55, 0x55, 0x55,
    0xaa, 0xaa, 0xaa, 0xaa, 0x55, 0x55, 0x55, 0x55,
    0xaa, 0xaa, 0xaa, 0xaa, 0x55, 0x55, 0x55, 0x55,
    0xaa, 0xaa, 0xaa, 0xaa, 0x55, 0x55, 0x55, 0x55,
    0xaa, 0xaa, 0xaa, 0xaa, 0x55, 0x55, 0x55, 0x55,
    0xaa, 0xaa, 0xaa, 0xaa, 0x55, 0x55, 0x55, 0x55,
    0xaa, 0xaa, 0xaa, 0xaa, 0x55, 0x55, 0x55, 0x55,
    0xaa, 0xaa, 0xaa, 0xaa, 0x55, 0x55, 0x55, 0x55,
    0xaa, 0xaa, 0xaa, 0xaa, 0x55, 0x55, 0x55, 0x55,
    0xaa, 0xaa, 0xaa, 0xaa, 0x55, 0x55, 0x55, 0x55,
    0xaa, 0xaa, 0xaa, 0xaa, 0x55, 0x55, 0x55, 0x55,
    0xaa, 0xaa, 0xaa, 0xaa, 0x55, 0x55, 0x55, 0x55,
    0xaa, 0xaa, 0xaa, 0xaa, 0x55, 0x55, 0x55, 0x55,
    0xaa, 0xaa, 0xaa, 0xaa, 0x55, 0x55, 0x55, 0x55,
    0xaa, 0xaa, 0xaa, 0xaa, 0x55, 0x55, 0x55, 0x55 };

  IRISApplication *app = m_Model->GetParent()->GetDriver();
  GlobalState *gs = app->GetGlobalState();
  GenericSliceModel *sliceModel = GetParentRenderer()->GetModel();

  // Get the list of bubbles
  IRISApplication::BubbleArray &bubbles = app->GetBubbleArray();

  // draw bubbles
  int numBubbles = bubbles.size();
  int activeBubble = gs->GetActiveBubble();

  if (numBubbles > 0)
    {
    // Get the active color label
    int currentcolor =  gs->GetDrawingColorLabel();
    ColorLabel cl = app->GetColorLabelTable()->GetColorLabel(currentcolor);

    // Get the current alpha blending factor for displaying overlays
    unsigned char alpha = (unsigned char)(255 * gs->GetSegmentationAlpha());

    // Get the color of the active color label
    Vector3ui clrFill(cl.GetRGB(0),cl.GetRGB(1),cl.GetRGB(2));
    Vector3ui clrWhite(255, 255, 255);
    Vector3ui clrLine = clrWhite - (clrWhite - clrFill) / 2u;

    // Get the current crosshairs position
    Vector3d cursorImage = to_double(app->GetCursorPosition()) + Vector3d(0.5);

    // Get the image space dimension that corresponds to this window
    int iid = sliceModel->GetSliceDirectionInImageSpace();

    // Get the other essentials from the parent
    Vector3d scaling = sliceModel->GetSliceSpacing();

    // Turn on alpha blending
    glPushAttrib(GL_POLYGON_BIT | GL_COLOR_BUFFER_BIT);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_POLYGON_STIPPLE);
    glEnable(GL_LINE_SMOOTH);
    glLineWidth(1.5);
    glPolygonStipple(stipple);

    // Draw each bubble
    for (int i = 0; i < numBubbles; i++)
      {
      // Get the center and radius of the i-th bubble
      Vector3d ctrImage = to_double(bubbles[i].center) + Vector3d(0.5);
      double radius = bubbles[i].radius;

      // Remap the center into slice coordinates
      Vector3d ctrSlice = sliceModel->MapImageToSlice(ctrImage);

      // Compute the offset from the center along the slice z-direction
      // in physical coordinates
      double dcenter = scaling(2) * (cursorImage(iid) - ctrImage(iid));

      // Check if the bubble is intersected by the current slice plane
      if (dcenter >= radius || -dcenter >= radius) continue;

      // Compute the radius of the bubble in the cut plane
      double diskradius = sqrt(fabs(radius*radius - dcenter*dcenter));

      // Inner and outer colors of the bubble
      gl_draw_circle_with_border(ctrSlice[0], ctrSlice[1], diskradius,
          1.0 / scaling(0), 1.0 / scaling(1),
          clrFill, alpha, clrLine, (activeBubble == i) ? 255 : 0,
          100);
      }

    glPopAttrib();
    }
}
