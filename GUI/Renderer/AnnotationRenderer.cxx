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

  // Draw current line
  if(m_Model->IsDrawingRuler())
    {
    const AnnotationModel::LineSegment &curr_line = m_Model->GetCurrentLine();

    // Use the polygon drawing settings
    const OpenGLAppearanceElement *elt =
        as->GetUIElement(SNAPAppearanceSettings::POLY_DRAW_MAIN);

    // Draw the current line
    glColor3d(1.,1.,0.);
    glBegin(GL_POINTS);
    glVertex2d(curr_line.first[0], curr_line.first[1]);
    glVertex2d(0.5 * (curr_line.first[0] + curr_line.second[0]),
               0.5 * (curr_line.first[1] + curr_line.second[1]));
    glVertex2d(curr_line.second[0], curr_line.second[1]);
    glEnd();
    glPushAttrib(GL_LINE_BIT | GL_COLOR_BUFFER_BIT);
    glEnable(GL_LINE_STIPPLE);
    glLineStipple(1, 0x9999);
    glBegin(GL_LINES);
    glVertex2d(curr_line.first[0], curr_line.first[1]);
    glVertex2d(curr_line.second[0], curr_line.second[1]);
    glEnd();
    glPopAttrib();

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

        if(lsa->GetSelected())
          glColor3d(1.,1.,0.);
        else
          glColor3d(1.,0.,0.);

        glBegin(GL_POINTS);
        glVertex2d((p1[0] + p2[0]) * 0.5, (p1[1] + p2[1]) * 0.5);
        glEnd();

        glBegin(GL_LINES);
        glVertex2d(p1[0], p1[1]);
        glVertex2d(p2[0], p2[1]);
        glEnd();
        }
      }
    }

  glPopAttrib();
}

AnnotationRenderer::AnnotationRenderer()
{
  m_Model = NULL;
}


