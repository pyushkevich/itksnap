#include "PaintbrushRenderer.h"
#include "PaintbrushModel.h"
#include "IRISApplication.h"
#include "GlobalState.h"
#include "SNAPAppearanceSettings.h"
#include "GlobalUIModel.h"
#include "SNAPOpenGL.h"

PaintbrushRenderer::PaintbrushRenderer()
{
}

void PaintbrushRenderer::BuildBrush()
{
  // Get the current properties
  GlobalState *gs = m_Model->GetParent()->GetDriver()->GetGlobalState();
  PaintbrushSettings ps = gs->GetPaintbrushSettings();

  // This is a simple 2D marching algorithm. At any given state of the
  // marching, there is a 'tail' and a 'head' of an arrow. To the right
  // of the arrow is a voxel that's inside the brush and to the left a
  // voxel that's outside. Depending on the two voxels that are
  // ahead of the arrow to the left and right (in in, in out, out out)
  // at the next step the arrow turns right, continues straight or turns
  // left. This goes on until convergence

  // Initialize the marching. This requires constructing the first arrow
  // and marching it to the left until it is between out and in voxels.
  // If the brush has even diameter, the arrow is from (0,0) to (1,0). If
  // the brush has odd diameter (center at voxel center) then the arrow
  // is from (-0.5, -0.5) to (-0.5, 0.5)
  Vector2d xTail, xHead;
  if(fmod(ps.radius,1.0) == 0)
    { xTail = Vector2d(0.0, 0.0); xHead = Vector2d(0.0, 1.0); }
  else
    { xTail = Vector2d(-0.5, -0.5); xHead = Vector2d(-0.5, 0.5); }

  // Shift the arrow to the left until it is in position

  while(m_Model->TestInside(Vector2d(xTail(0) - 0.5, xTail(1) + 0.5), ps))
    { xTail(0) -= 1.0; xHead(0) -= 1.0; }

  // Record the starting point, which is the current tail. Once the head
  // returns to the starting point, the loop is done
  Vector2d xStart = xTail;

  // Do the loop
  m_Walk.clear();
  size_t n = 0;
  while((xHead - xStart).squared_magnitude() > 0.01 && (++n) < 10000)
    {
    // Add the current head to the loop
    m_Walk.push_back(xHead);

    // Check the voxels ahead to the right and left
    Vector2d xStep = xHead - xTail;
    Vector2d xLeft(-xStep(1), xStep(0));
    Vector2d xRight(xStep(1), -xStep(0));
    bool il = m_Model->TestInside(xHead + 0.5 * (xStep + xLeft),ps);
    bool ir = m_Model->TestInside(xHead + 0.5 * (xStep + xRight),ps);

    // Update the tail
    xTail = xHead;

    // Decide which way to go
    if(il && ir)
      xHead += xLeft;
    else if(!il && ir)
      xHead += xStep;
    else if(!il && !ir)
      xHead += xRight;
    else
      assert(0);
    }

  // Add the last vertex
  m_Walk.push_back(xStart);
}



void PaintbrushRenderer::paintGL()
{
  // Check if the mouse is inside
  if(!m_Model->IsMouseInside())
    return;

  // Paint all the edges in the paintbrush definition
  SNAPAppearanceSettings *as =
      m_Model->GetParent()->GetParentUI()->GetAppearanceSettings();
  const OpenGLAppearanceElement *elt =
    as->GetUIElement(SNAPAppearanceSettings::PAINTBRUSH_OUTLINE);

  // Build the mask edges
  BuildBrush();

  // Set line properties
  glPushAttrib(GL_LINE_BIT | GL_COLOR_BUFFER_BIT);

  // Apply the line properties
  glColor3dv(elt->GetNormalColor().data_block());
  elt->ApplyLineSettings();

  // Get the brush position
  Vector3f xPos = m_Model->GetCenterOfPaintbrushInSliceSpace();

  // Refit matrix so that the lines are centered on the current pixel
  glPushMatrix();
  glTranslated( xPos(0), xPos(1), 0.0 );

  // Draw the lines around the point
  glBegin(GL_LINE_LOOP);
  for(std::list<Vector2d>::iterator it = m_Walk.begin(); it != m_Walk.end(); ++it)
    glVertex2d((*it)(0), (*it)(1));
  glEnd();

  // Pop the matrix
  glPopMatrix();

  // Pop the attributes
  glPopAttrib();


}
