#include "PolygonDrawingRenderer.h"
#include "PolygonDrawingModel.h"
#include "SNAPAppearanceSettings.h"
#include "GlobalUIModel.h"


// Default colors
const double PolygonDrawingRenderer::m_DrawingModeColor[] = { 1.0f, 0.0f, 0.5f };
const double PolygonDrawingRenderer::m_EditModeNormalColor[] = { 1.0f, 0.0f, 0.0f };
const double PolygonDrawingRenderer::m_EditModeSelectedColor[] = { 0.0f, 1.0f, 0.0f };

PolygonDrawingRenderer::PolygonDrawingRenderer()
{
  m_Model = NULL;
}

void
PolygonDrawingRenderer
::DrawBox(const vnl_vector_fixed<double, 4> &box,
          double border_x, double border_y)
{
  glBegin(GL_LINE_LOOP);
  glVertex3d(box[0] - border_x, box[2] - border_y, 0.0);
  glVertex3d(box[1] + border_x, box[2] - border_y, 0.0);
  glVertex3d(box[1] + border_x, box[3] + border_y, 0.0);
  glVertex3d(box[0] - border_x, box[3] + border_y, 0.0);
  glEnd();
}

void
PolygonDrawingRenderer
::paintGL()
{
  assert(m_Model);
  if(m_ParentRenderer->IsDrawingZoomThumbnail() || m_ParentRenderer->IsDrawingLayerThumbnail())
    return;

  PolygonDrawingModel::PolygonState state = m_Model->GetState();

  // Get appearance settings, etc
  GenericSliceModel *parentModel = m_ParentRenderer->GetModel();
  SNAPAppearanceSettings *as =
      parentModel->GetParentUI()->GetAppearanceSettings();

  // Must be in active state
  if (state == PolygonDrawingModel::INACTIVE_STATE)
    return;

  // Get appearance settings for the drawing
  OpenGLAppearanceElement *aeDraw = as->GetUIElement(
        SNAPAppearanceSettings::POLY_DRAW_MAIN);

  OpenGLAppearanceElement *aeClose = as->GetUIElement(
        SNAPAppearanceSettings::POLY_DRAW_CLOSE);

  OpenGLAppearanceElement *aeEdit = as->GetUIElement(
        SNAPAppearanceSettings::POLY_EDIT);

  OpenGLAppearanceElement *aeEditSelect = as->GetUIElement(
        SNAPAppearanceSettings::POLY_EDIT_SELECT);

  // Set line and point drawing parameters
  double vppr = m_ParentRenderer->GetModel()->GetSizeReporter()->GetViewportPixelRatio();
  glPointSize(3 * vppr);

  // Draw the line segments
  const PolygonDrawingModel::VertexList &vx = m_Model->GetVertices();
  const PolygonDrawingModel::VertexList &dvx = m_Model->GetDragVertices();

  // Useful iterators
  PolygonDrawingModel::VertexList::const_iterator it, itNext;

  if (state == PolygonDrawingModel::EDITING_STATE)
    {
    for(it = vx.begin(); it!=vx.end(); ++it)
      {
      glPushAttrib(GL_LINE_BIT | GL_COLOR_BUFFER_BIT);

      // Point to the next vertex, circular
      itNext = it; ++itNext;
      if(itNext == vx.end())
        itNext = vx.begin();

      // Set the color based on the mode
      if (it->selected && itNext->selected)
        {
        aeEditSelect->ApplyLineSettings();
        aeEditSelect->ApplyColor();
        }
      else
        {
        aeEdit->ApplyLineSettings();
        aeEdit->ApplyColor();
        }

      // Draw the line
      glBegin(GL_LINES);
      glVertex3d(it->x, it->y, 0);
      glVertex3d(itNext->x, itNext->y, 0);
      glEnd();

      glPopAttrib();
      }
    }
  else
    {
    // Not editing state
    glPushAttrib(GL_LINE_BIT | GL_COLOR_BUFFER_BIT);
    aeDraw->ApplyLineSettings();
    aeDraw->ApplyColor();

    // Draw the vertices
    glBegin(GL_LINE_STRIP);
    for(it = vx.begin(); it!=vx.end(); ++it)
      glVertex3d(it->x, it->y, 0);
    glEnd();

    // Draw the drag vertices
    if(dvx.size())
      {
      glBegin(GL_LINE_STRIP);
      for(it = dvx.begin(); it != dvx.end(); ++it)
        glVertex3d(it->x, it->y, 0);
      glEnd();
      }

    // If hovering over the last point, draw the closing line using
    // current appearance settings
    if(m_Model->IsHoverOverFirstVertex())
      {
      glBegin(GL_LINES);
      if(dvx.size())
        glVertex3d(dvx.back().x, dvx.back().y, 0);
      else
        glVertex3d(vx.back().x, vx.back().y, 0);
      glVertex3d(vx.front().x, vx.front().y, 0);
      glEnd();
      }

    else if(dvx.size() + vx.size() > 2 && aeClose->GetVisible())
      {
      // Draw the stripped line.
      glPushAttrib(GL_LINE_BIT | GL_COLOR_BUFFER_BIT);
      aeClose->ApplyLineSettings();
      aeClose->ApplyColor();

      glBegin(GL_LINES);
      if(dvx.size())
        glVertex3d(dvx.back().x, dvx.back().y, 0);
      else
        glVertex3d(vx.back().x, vx.back().y, 0);
      glVertex3d(vx.front().x, vx.front().y, 0);
      glEnd();
      glPopAttrib();
      }

    glPopAttrib();
    }

  // draw the vertices
  glBegin(GL_POINTS);
  glPushAttrib(GL_COLOR_BUFFER_BIT);
  for(it = vx.begin(); it!=vx.end();++it)
    {
    if(it->control)
      {
      if (it->selected)
        aeEditSelect->ApplyColor();
      else if (state == PolygonDrawingModel::DRAWING_STATE)
        aeDraw->ApplyColor();
      else
        aeEdit->ApplyColor();

      glVertex3d(it->x, it->y, 0.0f);
      }
    }

  // Draw the last dragging vertex point
  if(dvx.size())
    {
    PolygonVertex last = dvx.back();
    aeEdit->ApplyColor();
    glVertex3d(last.x, last.y, 0.0f);
    }

  glEnd();
  glPopAttrib();

  // draw edit or pick box
  if(m_Model->IsDraggingPickBox())
    {
    glPushAttrib(GL_LINE_BIT | GL_COLOR_BUFFER_BIT);

    glLineWidth(1);
    aeEdit->ApplyColor();
    DrawBox(m_Model->GetSelectionBox());

    glPopAttrib();
    }
  else if (m_Model->GetSelectedVertices())
    {
    glPushAttrib(GL_LINE_BIT | GL_COLOR_BUFFER_BIT);

    glLineWidth(1);
    aeEdit->ApplyColor();
    Vector2d border = m_Model->GetPixelSize() * 4.0;
    glLineWidth(1);
    glColor3dv(m_EditModeSelectedColor);
    DrawBox(m_Model->GetEditBox(), border[0], border[1]);
    glPopAttrib();
    }

  glPopAttrib();
}

