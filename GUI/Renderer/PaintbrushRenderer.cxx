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
PaintbrushRenderer::BuildBrush(AbstractRenderContext *context)
{
  // The outline is computed by the model; rebuild the path when it changes
  const auto &outline = m_Model->GetBrushOutline();
  if (m_BrushOutlinePath && outline == m_PathOutline)
    return;
  m_PathOutline = outline;

  // Create a renderer path
  m_BrushOutlinePath = context->CreatePath();
  context->AddPolygonSegmentToPath(m_BrushOutlinePath, m_PathOutline, false);
  context->BuildPath(m_BrushOutlinePath);
}
