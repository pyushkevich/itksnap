#include "AnnotationRenderer.h"
#include "SNAPAppearanceSettings.h"
#include "GlobalUIModel.h"
#include "AnnotationModel.h"
#include "GlobalState.h"
#include "IRISApplication.h"
#include "SNAPAppearanceSettings.h"
#include "ImageAnnotationData.h"
#include <iomanip>

void AnnotationRenderer::paintGL()
{
  assert(m_Model);

  // Not in thumbnail mode
  if(m_ParentRenderer->IsThumbnailDrawing())
    return;

  // Get appearance settings
  SNAPAppearanceSettings *as =
      m_Model->GetParent()->GetParentUI()->GetAppearanceSettings();

  // Get the current annotation settings
  AnnotationSettings annset = m_Model->GetParent()->GetDriver()->GetGlobalState()
                          ->GetAnnotationSettings();
  bool shownOnAllSlices = annset.shownOnAllSlices;

  // Get the retina pixel ratio
  int vppr = m_ParentRenderer->GetModel()->GetSizeReporter()->GetViewportPixelRatio();

  // Shared settings for text drawing
  Vector3f text_offset_slice =
      m_Model->GetParent()->MapWindowOffsetToSliceOffset(
        Vector2f(5.f, 5.f) * (float) vppr);

  Vector3f text_width_slice =
      m_Model->GetParent()->MapWindowOffsetToSliceOffset(
        Vector2f(96.f, 12.f) * (float) vppr);

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
    glColor3d(1.,1.,0.);
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
      // Compute the length of the drawing line
      double length = m_Model->GetCurrentLineLength();
      std::ostringstream oss_length;
      oss_length << std::setprecision(4) << length << " " << "mm";

      // Set up the rendering properties
      AbstractRendererPlatformSupport::FontInfo font_info =
            { AbstractRendererPlatformSupport::TYPEWRITER,
              12 * vppr,
              false };

      Vector3f curr_center = (curr_line.first + curr_line.second) * 0.5f;

      // Draw the length text
      m_PlatformSupport->RenderTextInOpenGL(
            oss_length.str().c_str(),
            curr_center[0] + text_offset_slice[0], curr_center[1] + text_offset_slice[1],
            text_width_slice[0], text_width_slice[1],
            font_info,
            AbstractRendererPlatformSupport::LEFT, AbstractRendererPlatformSupport::TOP,
            elt->GetNormalColor());

      // Compute and show the intersection angles of the drawing line with the other (visible) lines
      for(ImageAnnotationData::AnnotationConstIterator it = adata->GetAnnotations().begin();
          it != adata->GetAnnotations().end(); ++it)
        {
        const annot::LineSegmentAnnotation *lsa =
            dynamic_cast<const annot::LineSegmentAnnotation *>(it->GetPointer());

        if(lsa && m_Model->IsAnnotationVisible(lsa))
          {
          // Compute the dot product and no need for the third components that are zeros
          double angle = m_Model->GetAngleWithCurrentLine(lsa);
          std::ostringstream oss_angle;
          oss_angle << std::setprecision(3) << angle << " " << "deg";

          Vector3f line_center = m_Model->GetAnnotationCenter(lsa);

          // Draw the angle text
          m_PlatformSupport->RenderTextInOpenGL(
                oss_angle.str().c_str(),
                line_center[0] + text_offset_slice[0], line_center[1] + text_offset_slice[1],
                text_width_slice[0], text_width_slice[1],
                font_info,
                AbstractRendererPlatformSupport::LEFT, AbstractRendererPlatformSupport::TOP,
                elt->GetNormalColor());

          }
        }
      } // Ruler mode
    } // Current line valid

  // Draw each annotation
  for(ImageAnnotationData::AnnotationConstIterator it = adata->GetAnnotations().begin();
      it != adata->GetAnnotations().end(); ++it)
    {
    if(m_Model->IsAnnotationVisible(*it))
      {
      annot::LineSegmentAnnotation *lsa =
          dynamic_cast<annot::LineSegmentAnnotation *>(it->GetPointer());
      if(lsa)
        {
        Vector3f p1 = m_Model->GetParent()->MapImageToSlice(lsa->GetSegment().first);
        Vector3f p2 = m_Model->GetParent()->MapImageToSlice(lsa->GetSegment().second);

        glColor3dv(lsa->GetColor().data_block());



        glBegin(GL_POINTS);
        glVertex2d((p1[0] + p2[0]) * 0.5, (p1[1] + p2[1]) * 0.5);
        glEnd();

        glBegin(GL_LINES);
        glVertex2d(p1[0], p1[1]);
        glVertex2d(p2[0], p2[1]);
        glEnd();

        if(lsa->GetSelected())
          {
          this->DrawSelectionHandle(p1);
          this->DrawSelectionHandle(p2);
          }

        }

      annot::LandmarkAnnotation *lma =
          dynamic_cast<annot::LandmarkAnnotation *>(it->GetPointer());
      if(lma)
        {
        // Get the head and tail coordinate in slice units
        Vector3f xHeadSlice, xTailSlice;
        m_Model->GetLandmarkArrowPoints(lma->GetLandmark(), xHeadSlice, xTailSlice);

        std::string text = lma->GetLandmark().Text;

        glColor3dv(lma->GetColor().data_block());

        glBegin(GL_LINES);
        glVertex2d(xHeadSlice[0], xHeadSlice[1]);
        glVertex2d(xTailSlice[0], xTailSlice[1]);
        glEnd();

        if(lma->GetSelected())
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
        Vector2f xTextSizeWin;
        xTextSizeWin[0] = this->m_PlatformSupport->MeasureTextWidth(text.c_str(), fi);
        xTextSizeWin[1] = fi.pixel_size * vppr;

        // Text box size in slice coordinate units
        Vector3f xTextSizeSlice = m_Model->GetParent()->MapWindowOffsetToSliceOffset(xTextSizeWin);

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
                                                    lma->GetColor());
        }

      }
    }

  glPopAttrib();
}

void AnnotationRenderer::DrawSelectionHandle(const Vector3f &xSlice)
{
  // Determine the width of the line
  float radius = 4 * m_Model->GetParent()->GetSizeReporter()->GetViewportPixelRatio();
  Vector3f offset = m_Model->GetParent()->MapWindowOffsetToSliceOffset(Vector2f(radius, radius));

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


