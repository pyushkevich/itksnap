#include "AnnotationRenderer.h"
#include "SNAPAppearanceSettings.h"
#include "GlobalUIModel.h"
#include "AnnotationModel.h"
#include "GlobalState.h"
#include "IRISApplication.h"
#include "SNAPAppearanceSettings.h"
#include "ImageAnnotationData.h"
#include <iomanip>
#include "GenericSliceContextItem.h"
#include <vtkObjectFactory.h>
#include <vtkContext2D.h>
#include <vtkPen.h>
#include <vtkBrush.h>
#include <vtkTextProperty.h>
#include <vtkPoints2D.h>


class AnnotationContextItem : public GenericSliceContextItem
{
public:
  vtkTypeMacro(AnnotationContextItem, GenericSliceContextItem)
  static AnnotationContextItem *New();

  irisSetMacro(AnnotationModel, AnnotationModel *);
  irisGetMacro(AnnotationModel, AnnotationModel *);

  void DrawLineLength(vtkContext2D *painter,
                      const Vector3d &xSlice1,
                      const Vector3d &xSlice2,
                      const Vector3d &color,
                      double alpha)
  {
    // Compute the length of the drawing line
    double length = m_AnnotationModel->GetLineLength(xSlice1, xSlice2);

    // Get the retina pixel ratio
    int vppr = this->GetVPPR();

    // Shared settings for text drawing
    Vector3d text_offset_slice =
        m_Model->MapWindowOffsetToSliceOffset(
          Vector2d(5.0, 5.0) * (double) vppr);

    Vector3d text_width_slice =
        m_Model->MapWindowOffsetToSliceOffset(
          Vector2d(96., 12.) * (double) vppr);

    // TODO: support other units
    std::ostringstream oss_length;
    oss_length << std::setprecision(4) << length << " " << "mm";

    // Set up the rendering properties
    AbstractRendererPlatformSupport::FontInfo font_info =
          { AbstractRendererPlatformSupport::TYPEWRITER,
            12 * vppr,
            false };

    Vector3d curr_center = (xSlice1 + xSlice2) * 0.5;

    // Draw the length text
    painter->GetTextProp()->SetFontFamilyToCourier();
    painter->GetTextProp()->SetFontSize(12 * vppr);
    painter->GetTextProp()->SetBold(false);
    painter->GetTextProp()->SetJustificationToLeft();
    painter->GetTextProp()->SetVerticalJustificationToBottom();
    painter->GetTextProp()->SetColor(color[0], color[1], color[2]);
    painter->GetTextProp()->SetOpacity(alpha);

    painter->DrawString(
        curr_center[0] + text_offset_slice[0],
        curr_center[1] + text_offset_slice[1],
        oss_length.str().c_str());
  }

  void DrawSelectionHandle(vtkContext2D *painter, const Vector3d &xSlice)
  {
    // Determine the width of the line
    double radius = 4 * GetVPPR();
    Vector3d offset = m_Model->MapWindowOffsetToSliceOffset(Vector2d(radius, radius));

    painter->GetBrush()->SetColorF(1.0, 1.0, 1.0);
    painter->GetBrush()->SetOpacityF(0.5);
    painter->GetPen()->SetColorF(0.0, 0.0, 0.0);
    painter->GetPen()->SetOpacityF(1.0);
    painter->GetPen()->SetWidth(1 * GetVPPR());

    painter->DrawRect(
        xSlice[0] - offset[0], xSlice[1] - offset[1],
        2 * offset[0], 2 * offset[1]);
  }

  virtual bool Paint(vtkContext2D *painter) override
  {
    // Get appearance settings
    auto *as = m_Model->GetParentUI()->GetAppearanceSettings();
    auto *gs = m_Model->GetDriver()->GetGlobalState();

    // Get the opacity of the annotations, and stop if it is zero
    double alpha = gs->GetAnnotationAlpha();
    if(alpha == 0)
      return false;

    // Get the color for the annotations
    Vector3d ann_color = gs->GetAnnotationColor();

    // Get the retina pixel ratio
    int vppr = this->GetVPPR();

    // Shared settings for text drawing
    Vector3d text_offset_slice =
        m_Model->MapWindowOffsetToSliceOffset(Vector2d(5 * vppr, 5 * vppr));

    Vector3d text_width_slice =
        m_Model->MapWindowOffsetToSliceOffset(Vector2d(96 * vppr , 12 * vppr));

    // Get the list of annotations
    const ImageAnnotationData *adata = m_AnnotationModel->GetAnnotations();

    // set line and point drawing parameters
    // glPointSize(3 * vppr);
    // glLineWidth(1.0 * vppr);

    // Draw current line - the current line can either be a ruler or an annotation arrow
    if(m_AnnotationModel->GetFlagDrawingLine())
      {
      const auto &curr_line = m_AnnotationModel->GetCurrentLine();

      // Draw the line and the endpoints

      // Draw the current line's points
      painter->GetPen()->SetColorF(ann_color.data_block());
      painter->GetPen()->SetOpacityF(alpha);
      painter->GetPen()->SetWidth(3 * vppr);

      // First point and last point
      painter->DrawPoint(curr_line.first[0], curr_line.first[1]);
      painter->DrawPoint(curr_line.second[0], curr_line.second[1]);

      // Midpoint only drawn in ruler mode
      if(m_AnnotationModel->GetAnnotationMode() == ANNOTATION_RULER)
        painter->DrawPoint(
              0.5 * (curr_line.first[0] + curr_line.second[0]),
              0.5 * (curr_line.first[1] + curr_line.second[1]));

      // Draw the line itself
      painter->GetPen()->SetWidth(1 * vppr);
      painter->GetPen()->SetLineType(vtkPen::DOT_LINE);
      painter->DrawLine(
            curr_line.first[0], curr_line.first[1],
            curr_line.second[0], curr_line.second[1]);

      // Decoration drawn only in ruler mode
      if(m_AnnotationModel->GetAnnotationMode() == ANNOTATION_RULER)
        {
        // Draw the current line length
        DrawLineLength(painter, curr_line.first, curr_line.second, ann_color, alpha);
        }
      } // Current line valid

    // Draw each annotation
    for(auto it = adata->GetAnnotations().begin(); it != adata->GetAnnotations().end(); ++it)
      {
      if(m_AnnotationModel->IsAnnotationVisible(*it))
        {
        // Draw all the line segments
        auto *lsa = dynamic_cast<annot::LineSegmentAnnotation *>(it->GetPointer());
        if(lsa)
          {
          // Draw the line
          Vector3d p1 = m_Model->MapImageToSlice(lsa->GetSegment().first);
          Vector3d p2 = m_Model->MapImageToSlice(lsa->GetSegment().second);

          Vector3d color = lsa->GetColor();

          painter->GetPen()->SetColorF(color.data_block());
          painter->GetPen()->SetOpacityF(alpha);
          painter->GetPen()->SetWidth(3 * vppr);
          painter->DrawPoint((p1[0] + p2[0]) * 0.5, (p1[1] + p2[1]) * 0.5);

          painter->GetPen()->SetWidth(1 * vppr);
          painter->GetPen()->SetLineType(vtkPen::SOLID_LINE);
          painter->DrawLine(p1[0], p1[1], p2[0], p2[1]);

          if(lsa->GetSelected()
             && m_AnnotationModel->IsAnnotationModeActive()
             && m_AnnotationModel->GetAnnotationMode() == ANNOTATION_SELECT)
            {
            this->DrawSelectionHandle(painter, p1);
            this->DrawSelectionHandle(painter, p2);
            }

          // Draw length or angle
          if(m_AnnotationModel->IsDrawingRuler())
            {
            // Draw angle:
            // Compute the dot product and no need for the third components that are zeros
            double angle = m_AnnotationModel->GetAngleWithCurrentLine(lsa);
            std::ostringstream oss_angle;
            oss_angle << std::setprecision(3) << angle << "°";

            Vector3d line_center = m_AnnotationModel->GetAnnotationCenter(lsa);

            // Set up the rendering properties
            AbstractRendererPlatformSupport::FontInfo font_info =
                  { AbstractRendererPlatformSupport::TYPEWRITER,
                    12 * vppr,
                    false };

            // Draw the angle text
            painter->GetTextProp()->SetFontFamilyToCourier();
            painter->GetTextProp()->SetFontSize(12 * vppr);
            painter->GetTextProp()->SetBold(false);
            painter->GetTextProp()->SetJustificationToLeft();
            painter->GetTextProp()->SetVerticalJustificationToTop();
            painter->GetTextProp()->SetColor(color[0], color[1], color[2]);
            painter->GetTextProp()->SetOpacity(alpha);
            painter->DrawString(
                line_center[0] + text_offset_slice[0],
                line_center[1] + text_offset_slice[1],
                oss_angle.str().c_str());
            }
          else
            {
            this->DrawLineLength(painter, p1, p2, lsa->GetColor(),alpha);
            }
          }

        auto *lma = dynamic_cast<annot::LandmarkAnnotation *>(it->GetPointer());
        if(lma)
          {
          // Get the head and tail coordinate in slice units
          Vector3d xHeadSlice, xTailSlice;
          m_AnnotationModel->GetLandmarkArrowPoints(lma->GetLandmark(), xHeadSlice, xTailSlice);

          std::string text = lma->GetLandmark().Text;
          Vector3d color = lma->GetColor();

          // Draw the annotation line segment
          painter->GetPen()->SetColorF(color.data_block());
          painter->GetPen()->SetOpacityF(alpha);
          painter->GetPen()->SetWidth(1 * vppr);
          painter->GetPen()->SetLineType(vtkPen::SOLID_LINE);
          painter->DrawLine(xHeadSlice[0], xHeadSlice[1], xTailSlice[0], xTailSlice[1]);

          if(lma->GetSelected() && m_AnnotationModel->IsAnnotationModeActive() &&
             m_AnnotationModel->GetAnnotationMode() == ANNOTATION_SELECT)
            {
            this->DrawSelectionHandle(painter, xHeadSlice);
            this->DrawSelectionHandle(painter, xTailSlice);
            }

          // Font properties
          painter->GetTextProp()->SetFontFamilyToArial();
          painter->GetTextProp()->SetFontSize(12 * vppr);
          painter->GetTextProp()->SetBold(false);
          painter->GetTextProp()->SetJustificationToLeft();
          painter->GetTextProp()->SetVerticalJustificationToTop();
          painter->GetTextProp()->SetColor(color[0], color[1], color[2]);
          painter->GetTextProp()->SetOpacity(alpha);

          // Text box size in screen pixels
          float bounds[4];
          painter->ComputeStringBounds(text.c_str(), bounds);
          Vector2d xTextSizeWin(bounds[2] - bounds[0], bounds[3] - bounds[1]);
          std::cout << "STRING BOUNDS " << xTextSizeWin << std::endl;

          // Text box size in slice coordinate units
          // Vector3d xTextSizeSlice = m_Model->MapWindowOffsetToSliceOffset(xTextSizeWin);
          // std::cout << "STRING BOUNDS SLICE " << xTextSizeSlice << std::endl;
          Vector2d xTextSizeSlice = xTextSizeWin;

          // How to position the text
          vtkNew<vtkPoints2D> trect;
          trect->SetNumberOfPoints(2);
          double xbox, ybox;

          int align_horiz, align_vert;
          if(fabs(lma->GetLandmark().Offset[0]) >= fabs(lma->GetLandmark().Offset[1]))
            {
            align_vert = VTK_TEXT_CENTERED;
            ybox = xTailSlice[1] - xTextSizeSlice[1] / 2;
            if(lma->GetLandmark().Offset[0] >= 0)
              {
              align_horiz = VTK_TEXT_LEFT;
              xbox = xTailSlice[0];
              }
            else
              {
              align_horiz = VTK_TEXT_RIGHT;
              xbox = xTailSlice[0] - xTextSizeSlice[0];
              }
            }
          else
            {
            align_horiz = VTK_TEXT_CENTERED;
            xbox = xTailSlice[0] - xTextSizeSlice[0] / 2;
            if(lma->GetLandmark().Offset[1] >= 0)
              {
              align_vert = VTK_TEXT_BOTTOM;
              ybox = xTailSlice[1];
              }
            else
              {
              align_vert = VTK_TEXT_TOP;
              ybox = xTailSlice[1] - xTextSizeSlice[1];
              }
            }

          // Draw the text at the right location
          trect->SetPoint(0, xbox, ybox);
          trect->SetPoint(1, xTextSizeSlice[0], xTextSizeSlice[1]);
          painter->GetTextProp()->SetJustification(align_horiz);
          painter->GetTextProp()->SetVerticalJustification(align_vert);

          painter->GetPen()->SetColorF(1.0, 1.0, 0.0);
          painter->GetPen()->SetOpacityF(1.0);
          painter->DrawStringRect(trect, text.c_str());
          }

        }
      }
  }

protected:

  AnnotationModel *m_AnnotationModel;

};


vtkStandardNewMacro(AnnotationContextItem);



void AnnotationRenderer::DrawLineLength(const Vector3d &xSlice1,
                                        const Vector3d &xSlice2,
                                        const Vector3d &color,
                                        double alpha)
{
  // Compute the length of the drawing line
  double length = m_Model->GetLineLength(xSlice1, xSlice2);

  // Get the retina pixel ratio
  int vppr = m_ParentRenderer->GetModel()->GetSizeReporter()->GetViewportPixelRatio();

  // Shared settings for text drawing
  Vector3d text_offset_slice =
      m_Model->GetParent()->MapWindowOffsetToSliceOffset(
        Vector2d(5.0, 5.0) * (double) vppr);

  Vector3d text_width_slice =
      m_Model->GetParent()->MapWindowOffsetToSliceOffset(
        Vector2d(96., 12.) * (double) vppr);

  // TODO: support other units
  std::ostringstream oss_length;
  oss_length << std::setprecision(4) << length << " " << "mm";

  // Set up the rendering properties
  AbstractRendererPlatformSupport::FontInfo font_info =
        { AbstractRendererPlatformSupport::TYPEWRITER,
          12 * vppr,
          false };

  Vector3d curr_center = (xSlice1 + xSlice2) * 0.5;

  // Draw the length text
  m_PlatformSupport->RenderTextInOpenGL(
        oss_length.str().c_str(),
        curr_center[0] + text_offset_slice[0], curr_center[1] + text_offset_slice[1],
        text_width_slice[0], text_width_slice[1],
        font_info,
        AbstractRendererPlatformSupport::LEFT, AbstractRendererPlatformSupport::TOP,
        color, alpha);
}

void AnnotationRenderer::paintGL()
{
  assert(m_Model);

  // Not in thumbnail mode
  if(m_ParentRenderer->IsDrawingZoomThumbnail() || m_ParentRenderer->IsDrawingLayerThumbnail())
    return;

  // Get appearance settings
  SNAPAppearanceSettings *as =
      m_Model->GetParent()->GetParentUI()->GetAppearanceSettings();

  // Get the opacity of the annotations, and stop if it is zero
  double alpha =
      m_Model->GetParent()->GetParentUI()->GetGlobalState()->GetAnnotationAlpha();
  if(alpha == 0)
    return;

  // Get the color for the annotations
  Vector3d ann_color
      = m_Model->GetParent()->GetDriver()->GetGlobalState()->GetAnnotationColor();

  // Get the retina pixel ratio
  int vppr = m_ParentRenderer->GetModel()->GetSizeReporter()->GetViewportPixelRatio();

  // Shared settings for text drawing
  Vector3d text_offset_slice =
      m_Model->GetParent()->MapWindowOffsetToSliceOffset(
        Vector2d(5 * vppr, 5 * vppr));

  Vector3d text_width_slice =
      m_Model->GetParent()->MapWindowOffsetToSliceOffset(
        Vector2d(96 * vppr , 12 * vppr));

  // Get the list of annotations
  const ImageAnnotationData *adata = m_Model->GetAnnotations();

  // Push the line state
  glPushAttrib(GL_LINE_BIT | GL_COLOR_BUFFER_BIT);

  // set line and point drawing parameters
  glPointSize(3 * vppr);
  glLineWidth(1.0 * vppr);
  glEnable(GL_LINE_SMOOTH);
  glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  // Draw current line - the current line can either be a ruler or an annotation arrow
  if(m_Model->GetFlagDrawingLine())
    {
    const AnnotationModel::LineSegment &curr_line = m_Model->GetCurrentLine();

    // Use the polygon drawing settings
    const OpenGLAppearanceElement *elt =
        as->GetUIElement(SNAPAppearanceSettings::POLY_DRAW_MAIN);

    // Draw the line and the endpoints

    // Draw the current line
    glColor4d(ann_color[0], ann_color[1], ann_color[2],alpha);
    glBegin(GL_POINTS);
    glVertex2d(curr_line.first[0], curr_line.first[1]);

    // Midpoint only drawn in ruler mode
    if(m_Model->GetAnnotationMode() == ANNOTATION_RULER)
      {
      glVertex2d(0.5 * (curr_line.first[0] + curr_line.second[0]),
                 0.5 * (curr_line.first[1] + curr_line.second[1]));
      }

    glVertex2d(curr_line.second[0], curr_line.second[1]);
    glEnd();

    // Draw the line itself
    glPushAttrib(GL_LINE_BIT | GL_COLOR_BUFFER_BIT);
    glEnable(GL_LINE_STIPPLE);
    glLineStipple(1, 0x9999);
    glBegin(GL_LINES);
    glVertex2d(curr_line.first[0], curr_line.first[1]);
    glVertex2d(curr_line.second[0], curr_line.second[1]);
    glEnd();
    glPopAttrib();

    // Decoration drawn only in ruler mode
    if(m_Model->GetAnnotationMode() == ANNOTATION_RULER)
      {
      // Draw the current line length
      DrawLineLength(curr_line.first, curr_line.second, ann_color, alpha);
      }
    } // Current line valid

  // Draw each annotation
  for(ImageAnnotationData::AnnotationConstIterator it = adata->GetAnnotations().begin();
      it != adata->GetAnnotations().end(); ++it)
    {
    if(m_Model->IsAnnotationVisible(*it))
      {
      // Draw all the line segments
      annot::LineSegmentAnnotation *lsa =
          dynamic_cast<annot::LineSegmentAnnotation *>(it->GetPointer());
      if(lsa)
        {
        // Draw the line
        Vector3d p1 = m_Model->GetParent()->MapImageToSlice(lsa->GetSegment().first);
        Vector3d p2 = m_Model->GetParent()->MapImageToSlice(lsa->GetSegment().second);

        glColor4d(lsa->GetColor()[0], lsa->GetColor()[1], lsa->GetColor()[2], alpha);

        glBegin(GL_POINTS);
        glVertex2d((p1[0] + p2[0]) * 0.5, (p1[1] + p2[1]) * 0.5);
        glEnd();

        glBegin(GL_LINES);
        glVertex2d(p1[0], p1[1]);
        glVertex2d(p2[0], p2[1]);
        glEnd();

        if(lsa->GetSelected() && m_Model->IsAnnotationModeActive() &&
           m_Model->GetAnnotationMode() == ANNOTATION_SELECT)
          {
          this->DrawSelectionHandle(p1);
          this->DrawSelectionHandle(p2);
          }

        // Draw length or angle
        if(m_Model->IsDrawingRuler())
          {
          // Draw angle:
          // Compute the dot product and no need for the third components that are zeros
          double angle = m_Model->GetAngleWithCurrentLine(lsa);
          std::ostringstream oss_angle;
          oss_angle << std::setprecision(3) << angle << "°";

          Vector3d line_center = m_Model->GetAnnotationCenter(lsa);

          // Set up the rendering properties
          AbstractRendererPlatformSupport::FontInfo font_info =
                { AbstractRendererPlatformSupport::TYPEWRITER,
                  12 * vppr,
                  false };

          // Draw the angle text
          m_PlatformSupport->RenderTextInOpenGL(
                oss_angle.str().c_str(),
                line_center[0] + text_offset_slice[0], line_center[1] + text_offset_slice[1],
              text_width_slice[0], text_width_slice[1],
              font_info,
              AbstractRendererPlatformSupport::LEFT, AbstractRendererPlatformSupport::TOP,
              lsa->GetColor(), alpha);
          }
        else
          {
          this->DrawLineLength(p1, p2, lsa->GetColor(),alpha);
          }
        }

      annot::LandmarkAnnotation *lma =
          dynamic_cast<annot::LandmarkAnnotation *>(it->GetPointer());
      if(lma)
        {
        // Get the head and tail coordinate in slice units
        Vector3d xHeadSlice, xTailSlice;
        m_Model->GetLandmarkArrowPoints(lma->GetLandmark(), xHeadSlice, xTailSlice);

        std::string text = lma->GetLandmark().Text;

        glColor4d(lma->GetColor()[0], lma->GetColor()[1], lma->GetColor()[2], alpha);

        glBegin(GL_LINES);
        glVertex2d(xHeadSlice[0], xHeadSlice[1]);
        glVertex2d(xTailSlice[0], xTailSlice[1]);
        glEnd();

        if(lma->GetSelected() && m_Model->IsAnnotationModeActive() &&
           m_Model->GetAnnotationMode() == ANNOTATION_SELECT)
          {
          this->DrawSelectionHandle(xHeadSlice);
          this->DrawSelectionHandle(xTailSlice);
          }

        // Font properties
        AbstractRendererPlatformSupport::FontInfo fi;
        fi.type = AbstractRendererPlatformSupport::SANS;
        fi.pixel_size = 12 * vppr;
        fi.bold = false;

        // Text box size in screen pixels
        Vector2d xTextSizeWin;
        xTextSizeWin[0] = this->m_PlatformSupport->MeasureTextWidth(text.c_str(), fi);
        xTextSizeWin[1] = fi.pixel_size * vppr;

        // Text box size in slice coordinate units
        Vector3d xTextSizeSlice = m_Model->GetParent()->MapWindowOffsetToSliceOffset(xTextSizeWin);

        // How to position the text
        double xbox, ybox;
        int align_horiz, align_vert;
        if(fabs(lma->GetLandmark().Offset[0]) >= fabs(lma->GetLandmark().Offset[1]))
          {
          align_vert = AbstractRendererPlatformSupport::VCENTER;
          ybox = xTailSlice[1] - xTextSizeSlice[1] / 2;
          if(lma->GetLandmark().Offset[0] >= 0)
            {
            align_horiz = AbstractRendererPlatformSupport::LEFT;
            xbox = xTailSlice[0];
            }
          else
            {
            align_horiz = AbstractRendererPlatformSupport::RIGHT;
            xbox = xTailSlice[0] - xTextSizeSlice[0];
            }
          }
        else
          {
          align_horiz = AbstractRendererPlatformSupport::HCENTER;
          xbox = xTailSlice[0] - xTextSizeSlice[0] / 2;
          if(lma->GetLandmark().Offset[1] >= 0)
            {
            align_vert = AbstractRendererPlatformSupport::BOTTOM;
            ybox = xTailSlice[1];
            }
          else
            {
            align_vert = AbstractRendererPlatformSupport::TOP;
            ybox = xTailSlice[1] - xTextSizeSlice[1];
            }
          }

        // Draw the text at the right location
        this->m_PlatformSupport->RenderTextInOpenGL(text.c_str(),
                                                    xbox, ybox,
                                                    xTextSizeSlice[0], xTextSizeSlice[1], fi,
                                                    align_horiz, align_vert,
                                                    lma->GetColor(), alpha);
        }

      }
    }

  glPopAttrib();
}

void AnnotationRenderer::SetModel(AnnotationModel *model)
{
  m_Model = model;
  GlobalState *gs = m_Model->GetParent()->GetParentUI()->GetGlobalState();

  // Respond to changes in alpha
  m_Model->Rebroadcast(gs->GetAnnotationAlphaModel(), ValueChangedEvent(), ModelUpdateEvent());

  // Respond to current tool mode
  m_Model->Rebroadcast(gs->GetToolbarModeModel(), ValueChangedEvent(), ModelUpdateEvent());
}

void AnnotationRenderer::AddContextItemsToTiledOverlay(
    vtkAbstractContextItem *parent, ImageWrapperBase *)
{
  if(m_Model)
    {
    vtkNew<AnnotationContextItem> ci;
    ci->SetModel(m_Model->GetParent());
    ci->SetAnnotationModel(m_Model);
    parent->AddItem(ci);
    }
}

void AnnotationRenderer::DrawSelectionHandle(const Vector3d &xSlice)
{
  // Determine the width of the line
  double radius = 4 * m_Model->GetParent()->GetSizeReporter()->GetViewportPixelRatio();
  Vector3d offset = m_Model->GetParent()->MapWindowOffsetToSliceOffset(Vector2d(radius, radius));

  glColor4d(1, 1, 1, 0.5);
  glBegin(GL_QUADS);
  glVertex2d(xSlice[0] + offset[0], xSlice[1] + offset[1]);
  glVertex2d(xSlice[0] + offset[0], xSlice[1] - offset[1]);
  glVertex2d(xSlice[0] - offset[0], xSlice[1] - offset[1]);
  glVertex2d(xSlice[0] - offset[0], xSlice[1] + offset[1]);
  glEnd();

  glColor3d(0, 0, 0);
  glBegin(GL_LINE_LOOP);
  glVertex2d(xSlice[0] + offset[0], xSlice[1] + offset[1]);
  glVertex2d(xSlice[0] + offset[0], xSlice[1] - offset[1]);
  glVertex2d(xSlice[0] - offset[0], xSlice[1] - offset[1]);
  glVertex2d(xSlice[0] - offset[0], xSlice[1] + offset[1]);
  glEnd();
}

AnnotationRenderer::AnnotationRenderer()
{
  m_Model = NULL;
}


