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
     && !this->GetParentRenderer()->IsThumbnailDrawing())
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
    unsigned char rgb[3];
    cl.GetRGBVector(rgb);

    // Get the current crosshairs position
    Vector3f cursorImage = to_float(app->GetCursorPosition()) + Vector3f(0.5f);

    // Get the image space dimension that corresponds to this window
    int iid = sliceModel->GetSliceDirectionInImageSpace();

    // Get the other essentials from the parent
    Vector3f scaling = sliceModel->GetSliceSpacing();

    // Turn on alpha blending
    glPushAttrib(GL_POLYGON_BIT | GL_COLOR_BUFFER_BIT);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Create a filled circle object
    GLUquadricObj *object = gluNewQuadric();
    gluQuadricDrawStyle(object,GLU_FILL);

    // Draw each bubble
    for (int i = 0; i < numBubbles; i++)
      {

      // Get the center and radius of the i-th bubble
      Vector3f ctrImage = to_float(bubbles[i].center) + Vector3f(0.5f);
      double radius = bubbles[i].radius;

      // Remap the center into slice coordinates
      Vector3f ctrSlice = sliceModel->MapImageToSlice(to_float(ctrImage));

      // Compute the offset from the center along the slice z-direction
      // in physical coordinates
      double dcenter = scaling(2) * (cursorImage(iid) - ctrImage(iid));

      // Check if the bubble is intersected by the current slice plane
      if (dcenter >= radius || -dcenter >= radius) continue;

      // Compute the radius of the bubble in the cut plane
      double diskradius = sqrt(fabs(radius*radius - dcenter*dcenter));

      // Draw the bubble
      glColor4ub(rgb[0],rgb[1],rgb[2],alpha);
      glPushMatrix();

      if(activeBubble == i)
        {
        glEnable(GL_POLYGON_STIPPLE);
        glPolygonStipple(stipple);
        }

      glTranslatef(ctrSlice[0], ctrSlice[1], 0.0f);
      glScalef(1.0f / scaling(0),1.0f / scaling(1),1.0f);
      gluDisk(object,0,diskradius,100,1);

      // If the bubble is active, draw an outline around the bubble
      if(activeBubble == i)
        {
        glPushAttrib(GL_LINE_BIT | GL_COLOR_BUFFER_BIT);

        glEnable(GL_BLEND);
        glEnable(GL_LINE_SMOOTH);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glLineWidth(1.5);

        glColor4ub(
              255 - (255 - rgb[0]) / 2,
              255 - (255 - rgb[1]) / 2,
              255 - (255 - rgb[2]) / 2, 255);

        glBegin(GL_LINE_LOOP);
        for(unsigned int d = 0; d < 360; d+=2)
          {
          double rad = d * vnl_math::pi / 180.0;
          glVertex2f(diskradius * cos(rad), diskradius * sin(rad));
          }
        glEnd();
        glPopAttrib();
        }

      glPopMatrix();

      }

    gluDeleteQuadric(object);
    glDisable(GL_BLEND);
    glPopAttrib();
    }
}
