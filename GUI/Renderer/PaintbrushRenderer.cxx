#include "PaintbrushRenderer.h"
#include "PaintbrushModel.h"
#include "IRISApplication.h"
#include "GlobalState.h"
#include "SNAPAppearanceSettings.h"
#include "GlobalUIModel.h"
#include <vtkPen.h>

void
PaintbrushRenderer::RenderOverTiledLayer(AbstractRenderContext *context,
                                            ImageWrapperBase         *base_layer,
                                            const SubViewport        &vp)
{
  // Check if the mouse is inside
  if(!m_Model->IsMouseInside() || vp.isThumbnail)
    return;

  // Paint all the edges in the paintbrush definition
  auto *as = m_Model->GetParent()->GetParentUI()->GetAppearanceSettings();
  const auto *elt = as->GetUIElement(SNAPAppearanceSettings::PAINTBRUSH_OUTLINE);

  // Build the mask edges
  BuildBrush(context);

  // Apply the line properties
  context->SetPenAppearance(*elt);

  // Get the brush position
  Vector3d xPos = m_Model->GetCenterOfPaintbrushInSliceSpace();

  // Shift by the position
  context->PushMatrix();
  context->Translate(xPos[0], xPos[1]);
  context->DrawPath(m_BrushOutlinePath);
  context->PopMatrix();
}


void
PaintbrushRenderer::InsertWithCollinearCheck(const Vector2d &head)
{
  Vector2d      a, b;
  int           n = m_BrushOutline.size();

  // Do not insert the same point twice
  if (n > 0)
  {
    b = m_BrushOutline[n - 1];
    if (fabs(b[0] - head[0]) < 1e-4 && fabs(b[1] - head[1]) < 1e-4)
      return;

    // Do not insert collinear points
    if (n > 1)
    {
      a = m_BrushOutline[n - 2];
      double v = (b[0] - a[0]) * (head[1] - a[1]) - (b[1] - a[1]) * (head[0] - a[0]);
      if (fabs(v) < 1.e-4)
      {
        m_BrushOutline[n - 1] = head;
        return;
      }
    }
  }

  // Safe to append point
  m_BrushOutline.push_back(head);
}

void
PaintbrushRenderer::BuildBrush(AbstractRenderContext *context)
{
  // Get the current properties
  PaintbrushSettings ps = m_Model->GetEffectivePaintbrushSettings();

  // Check if the cached settings can be used
  if (m_BrushOutline.size() && ps.radius == m_CachedBrushSettings.radius &&
      ps.isotropic == m_CachedBrushSettings.isotropic && ps.shape == m_CachedBrushSettings.shape &&
      ps.volumetric == m_CachedBrushSettings.volumetric)
    return;

  // Cache the current settings
  m_CachedBrushSettings = ps;

  // Create the new walk
  m_BrushOutline.clear();

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
  if (fmod(ps.radius, 1.0) == 0)
  {
    xTail = Vector2d(0.0, 0.0);
    xHead = Vector2d(0.0, 1.0);
  }
  else
  {
    xTail = Vector2d(-0.5, -0.5);
    xHead = Vector2d(-0.5, 0.5);
  }

  // Shift the arrow to the left until it is in position
  while (m_Model->TestInside(Vector2d(xTail(0) - 0.5, xTail(1) + 0.5), ps))
  {
    xTail(0) -= 1.0;
    xHead(0) -= 1.0;
  }

  // Record the starting point, which is the current tail. Once the head
  // returns to the starting point, the loop is done
  Vector2d xStart = xTail;

  // Add the first vertex
  InsertWithCollinearCheck(xStart);

  // Do the loop
  size_t n = 0;
  while ((xHead - xStart).squared_magnitude() > 0.01 && (++n) < 10000)
  {
    // Add the current head to the loop
    InsertWithCollinearCheck(xHead);

    // Check the voxels ahead to the right and left
    Vector2d xStep = xHead - xTail;
    Vector2d xLeft(-xStep(1), xStep(0));
    Vector2d xRight(xStep(1), -xStep(0));
    bool     il = m_Model->TestInside(xHead + 0.5 * (xStep + xLeft), ps);
    bool     ir = m_Model->TestInside(xHead + 0.5 * (xStep + xRight), ps);

    // Update the tail
    xTail = xHead;

    // Decide which way to go
    if (il && ir)
      xHead += xLeft;
    else if (!il && ir)
      xHead += xStep;
    else if (!il && !ir)
      xHead += xRight;
    else
      assert(0);
  }

  // Add the last vertex
  InsertWithCollinearCheck(xStart);

  // Send to the model
  m_Model->SetBrushPoints(m_BrushOutline);

  // Create a renderer path
  m_BrushOutlinePath = context->CreatePath();
  context->AddPolygonSegmentToPath(m_BrushOutlinePath, m_BrushOutline, false);
  context->BuildPath(m_BrushOutlinePath);
}
