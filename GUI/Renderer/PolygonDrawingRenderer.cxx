#include "PolygonDrawingRenderer.h"
#include "PolygonDrawingModel.h"
#include "SNAPAppearanceSettings.h"
#include "GlobalUIModel.h"
#include "GenericSliceContextItem.h"
#include <vtkObjectFactory.h>
#include <vtkContext2D.h>
#include <vtkPen.h>

class PolygonContextItem : public GenericSliceContextItem
{
public:
  vtkTypeMacro(PolygonContextItem, GenericSliceContextItem)
  static PolygonContextItem *New();

  irisSetMacro(PolygonModel, PolygonDrawingModel *);
  irisGetMacro(PolygonModel, PolygonDrawingModel *);

  void DrawVertices(
      vtkContext2D *painter, const PolygonDrawingModel::VertexList &vx,
      bool closed = false)
  {
    // TODO: it would be more efficient to use polyline code rather
    // than drawing multiple line segments
    for(auto it = vx.begin(); it!=vx.end(); ++it)
      {
      auto itNext = it; ++itNext;
      if(itNext == vx.end())
        {
        if(closed)
          itNext = vx.begin();
        else
          break;
        }

      painter->DrawLine(it->x, it->y, itNext->x, itNext->y);
      }
  }

  void DrawSelectionBox(
      vtkContext2D *painter,
      const PolygonDrawingModel::BoxType &box,
      double border_x, double border_y)
  {
    this->DrawRectNoFill(
          painter,
          box[0] - border_x, box[2] - border_y,
          box[1] + border_x, box[3] + border_y);
  }

  virtual bool Paint(vtkContext2D *painter) override
  {
    // Pointers to the polygon model and the parent model
    auto *parentModel = this->GetModel();
    auto *polyModel = this->GetPolygonModel();

    // Polygon drawing state
    PolygonDrawingModel::PolygonState state = polyModel->GetState();

    // Must be in active state
    if (state == PolygonDrawingModel::INACTIVE_STATE)
      return false;

    // Get appearance settings, etc
    auto *as = parentModel->GetParentUI()->GetAppearanceSettings();

    // Get appearance settings for the drawing
    auto *aeDraw = as->GetUIElement(SNAPAppearanceSettings::POLY_DRAW_MAIN);
    auto *aeClose = as->GetUIElement(SNAPAppearanceSettings::POLY_DRAW_CLOSE);
    auto *aeEdit = as->GetUIElement(SNAPAppearanceSettings::POLY_EDIT);
    auto *aeEditSelect = as->GetUIElement(SNAPAppearanceSettings::POLY_EDIT_SELECT);

    // Set line and point drawing parameters
    double vppr = parentModel->GetSizeReporter()->GetViewportPixelRatio();

    // Draw the line segments
    const PolygonDrawingModel::VertexList &vx = polyModel->GetVertices();
    const PolygonDrawingModel::VertexList &dvx = polyModel->GetDragVertices();

    // Useful iterators
    PolygonDrawingModel::VertexList::const_iterator it, itNext;

    if (state == PolygonDrawingModel::EDITING_STATE)
      {
      for(it = vx.begin(); it!=vx.end(); ++it)
        {
        // Point to the next vertex, circular
        itNext = it; ++itNext;
        if(itNext == vx.end())
          itNext = vx.begin();

        // Set the color based on the mode
        if (it->selected && itNext->selected)
          this->ApplyAppearanceSettingsToPen(painter, aeEditSelect);
        else
          this->ApplyAppearanceSettingsToPen(painter, aeEdit);

        // Draw the line segment
        painter->DrawLine(it->x, it->y, itNext->x, itNext->y);
        }
      }
    else
      {
      // Not editing state
      this->ApplyAppearanceSettingsToPen(painter, aeDraw);

      // Draw polyline
      // TODO: it would be more efficient to use polyline code rather
      // than drawing multiple line segments
      this->DrawVertices(painter, vx, false);

      // Draw the drag vertices
      this->DrawVertices(painter, dvx, false);

      // If hovering over the last point, draw the closing line using
      // current appearance settings
      if(polyModel->IsHoverOverFirstVertex())
        {
        auto &last_vtx = dvx.size() ? dvx.back() : vx.back();
        painter->DrawLine(last_vtx.x, last_vtx.y, vx.front().x, vx.front().y);
        }

      else if(dvx.size() + vx.size() > 2 && aeClose->GetVisible())
        {
        // Draw the stripped line.
        this->ApplyAppearanceSettingsToPen(painter, aeClose);
        auto &last_vtx = dvx.size() ? dvx.back() : vx.back();
        painter->DrawLine(last_vtx.x, last_vtx.y, vx.front().x, vx.front().y);
        }
      }

    // draw the vertices as points
    for(it = vx.begin(); it!=vx.end();++it)
      {
      if(it->control)
        {
        auto *elt = it->selected
                    ? aeEditSelect
                    : (state == PolygonDrawingModel::DRAWING_STATE
                       ? aeDraw
                       : aeEdit);

        painter->GetPen()->SetColorF(elt->GetColor().data_block());
        painter->GetPen()->SetWidth(elt->GetLineThickness() * 3 * vppr);
        painter->DrawPoint(it->x, it->y);
        }
      }

    // Draw the last dragging vertex point
    if(dvx.size())
      {
      painter->GetPen()->SetColorF(aeEdit->GetColor().data_block());
      glVertex3d(dvx.back().x, dvx.back().y, 0.0f);
      }

    // draw edit or pick box
    if(polyModel->IsDraggingPickBox())
      {
      auto &box = polyModel->GetSelectionBox();
      painter->GetPen()->SetWidth(1);
      painter->GetPen()->SetColorF(aeEditSelect->GetColor().data_block());
      DrawSelectionBox(painter, box, 0, 0);
      }
    else if (polyModel->GetSelectedVertices())
      {
      auto &box = polyModel->GetEditBox();
      Vector2d border = polyModel->GetPixelSize() * 4.0;

      painter->GetPen()->SetWidth(1);
      painter->GetPen()->SetColorF(aeEditSelect->GetColor().data_block());
      DrawSelectionBox(painter, box, border[0], border[1]);
      }

    return true;
  }

protected:

  PolygonDrawingModel *m_PolygonModel;

};

vtkStandardNewMacro(PolygonContextItem);


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

void PolygonDrawingRenderer::SetModel(PolygonDrawingModel *model)
{
  m_ContextItem = vtkNew<PolygonContextItem>();
  m_ContextItem->SetModel(model->GetParent());
  m_ContextItem->SetPolygonModel(model);
  m_Model = model;
}

void PolygonDrawingRenderer::AddContextItemsToTiledOverlay(vtkAbstractContextItem *parent)
{
  if(m_ContextItem)
    parent->AddItem(m_ContextItem);
}

