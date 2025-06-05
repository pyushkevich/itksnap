#include "SnakeROIRenderer.h"
#include "IRISApplication.h"
#include "GlobalState.h"
#include "SnakeROIModel.h"
#include "SNAPAppearanceSettings.h"
#include "GlobalUIModel.h"

void
SnakeROIRenderer::RenderOverTiledLayer(AbstractRenderContext *context,
                                          ImageWrapperBase         *base_layer,
                                          const SubViewport        &vp)
{
  // Get some global information
  GenericSliceModel *slice_model = m_Model->GetParent();
  IRISApplication *app = slice_model->GetDriver();
  GlobalState *gs = app->GetGlobalState();
  SNAPAppearanceSettings *as = slice_model->GetParentUI()->GetAppearanceSettings();

  if(vp.isThumbnail || !gs->isSegmentationROIValid())
    return;

  // Compute the corners in slice coordinates
  Vector3d corner[2];
  m_Model->GetSystemROICorners(corner);

  // Check that the current slice is actually within the bounding box
  // int slice = m_Parent->m_SliceIndex;
  int dim = slice_model->GetSliceDirectionInImageSpace();
  int slice = slice_model->GetSliceIndex();
  int bbMin = gs->GetSegmentationROI().GetIndex(dim);
  int bbMax = bbMin + gs->GetSegmentationROI().GetSize(dim);

  // And if so, return without painting anything
  if(bbMin > slice || bbMax <= slice)
    return;

  // Get the line color, thickness and dash spacing
  const auto *elt = as->GetUIElement(SNAPAppearanceSettings::ROI_BOX);

  // Get the line color, thickness and dash spacing
  const auto *eltActive = as->GetUIElement(SNAPAppearanceSettings::ROI_BOX_ACTIVE);

  // Draw each of the edges
  for(unsigned int dir=0;dir<2;dir++)
  {
    for(unsigned int i=0;i<2;i++)
    {
      // Select color according to edge state
      if(m_Model->GetHighlight().Highlighted[dir][i])
        context->SetPenAppearance(*eltActive);
      else
        context->SetPenAppearance(*elt);

      // Compute the vertices of the edge
      Vector2d x0, x1;
      m_Model->GetEdgeVertices(dir, i, x0, x1, corner);

      // Start drawing the lines
      context->DrawLine(x0[0], x0[1], x1[0], x1[1]);
    }
  }
}
