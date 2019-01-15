#include "SnakeROIRenderer.h"
#include "IRISException.h"
#include "IRISApplication.h"
#include "GlobalState.h"
#include "SnakeROIModel.h"
#include "SNAPAppearanceSettings.h"
#include "GlobalUIModel.h"

SnakeROIRenderer::SnakeROIRenderer()
{
}

void SnakeROIRenderer::paintGL()
{
  // Get some global information
  GenericSliceModel *parentModel = m_ParentRenderer->GetModel();
  IRISApplication *app = parentModel->GetDriver();
  GlobalState *gs = app->GetGlobalState();
  SNAPAppearanceSettings *as =
      parentModel->GetParentUI()->GetAppearanceSettings();

  // Check the current state
  assert(m_Model);
  if(m_ParentRenderer->IsDrawingZoomThumbnail())
    return;

  // The region of interest should be in effect
  assert(gs->isSegmentationROIValid());

  // Compute the corners in slice coordinates
  Vector3d corner[2];
  m_Model->GetSystemROICorners(corner);

  // Check that the current slice is actually within the bounding box
  // int slice = m_Parent->m_SliceIndex;
  int dim = parentModel->GetSliceDirectionInImageSpace();
  int slice = parentModel->GetSliceIndex();
  int bbMin = gs->GetSegmentationROI().GetIndex(dim);
  int bbMax = bbMin + gs->GetSegmentationROI().GetSize(dim);

  // And if so, return without painting anything
  if(bbMin > slice || bbMax <= slice)
    return;

  // Get the line color, thickness and dash spacing
  const OpenGLAppearanceElement *elt =
      as->GetUIElement(SNAPAppearanceSettings::ROI_BOX);

  // Get the line color, thickness and dash spacing
  const OpenGLAppearanceElement *eltActive =
      as->GetUIElement(SNAPAppearanceSettings::ROI_BOX_ACTIVE);

  // Draw each of the edges
  for(unsigned int dir=0;dir<2;dir++)
    {
    for(unsigned int i=0;i<2;i++)
      {
      // Set line properties
      glPushAttrib(GL_LINE_BIT | GL_COLOR_BUFFER_BIT);

      // Select color according to edge state
      if(m_Model->m_Highlight.Highlighted[dir][i])
        {
        eltActive->ApplyLineSettings();
        eltActive->ApplyColor();
        }
      else
        {
        elt->ApplyLineSettings();
        elt->ApplyColor();
        }

      // Compute the vertices of the edge
      Vector2d x0,x1;
      m_Model->GetEdgeVertices(dir,i,x0,x1,corner);

      // Start drawing the lines
      glBegin(GL_LINES);

      // Draw the line
      glVertex2d(x0[0],x0[1]);
      glVertex2d(x1[0],x1[1]);

      glEnd();
      glPopAttrib();
      }
    }
}
