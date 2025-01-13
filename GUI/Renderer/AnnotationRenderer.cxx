#include "AnnotationRenderer.h"
#include "GlobalUIModel.h"
#include "AnnotationModel.h"
#include "GlobalState.h"
#include "IRISApplication.h"
#include "ImageAnnotationData.h"
#include <iomanip>
#include <vtkObjectFactory.h>
#include <vtkContext2D.h>
#include <vtkPen.h>
#include <vtkBrush.h>
#include <vtkTextProperty.h>
#include <vtkPoints2D.h>

/* Courtesy of ChatGPT */
std::string
formatDistance(double distance_mm, int significant_digits)
{
  // Define scale factors and abbreviations
  const std::vector<std::tuple<double, std::string>> scales = {
    { 1e-6, "nm" }, // nanometers
    { 1e-3, "µm" }, // micrometers
    { 1.0, "mm" },  // millimeters
    { 1e3, "m" },   // meters
    { 1e6, "km" }   // kilometers
  };

  // Normalize distance to meters
  double distance_meters = distance_mm / 1000.0;

  // Find the most appropriate scale
  std::string abbreviation;
  double      scaled_distance = distance_mm;
  for (const auto &[scale_factor, unit] : scales)
  {
    if (std::abs(distance_meters) < scale_factor || unit == "km")
    {
      scaled_distance = 1000 * distance_meters / scale_factor;
      abbreviation = unit;
      break;
    }
  }

  // Format the output with significant digits
  std::ostringstream oss;
  oss << std::setprecision(significant_digits - 1) << scaled_distance;
  return oss.str() + " " + abbreviation;
}

void
AnnotationRenderer::DrawLineLength(AbstractRenderContext *context,
                                      const Vector3d           &xSlice1,
                                      const Vector3d           &xSlice2,
                                      const Vector3d           &color,
                                      double                    alpha)
{
  // Compute the length of the drawing line
  double length = m_Model->GetLineLength(xSlice1, xSlice2);

  // Get the retina pixel ratio
  auto *parentModel = m_Model->GetParent();

  // Shared settings for text drawing
  auto text_offset_slice = context->MapScreenOffsetToWorldOffset(Vector2d(5., 5.));
  auto text_width_slice = context->MapScreenOffsetToWorldOffset(Vector2d(96., 12.));

  // TODO: support other units
  // std::ostringstream oss_length;
  // oss_length << std::setprecision(4) << length << " " << "mm";
  auto len_str = formatDistance(length, 4);

  // Set up the rendering properties
  auto *rps = AbstractRenderer::GetPlatformSupport();
  auto  font_info = rps->MakeFont(12, AbstractRendererPlatformSupport::TYPEWRITER);

  Vector3d curr_center = (xSlice1 + xSlice2) * 0.5;

  // Draw the length text
  context->SetFont(font_info);
  context->SetPenColor(color, alpha);
  context->DrawText(len_str,
                    curr_center[0] + text_offset_slice[0],
                    curr_center[1] + text_offset_slice[1],
                    text_width_slice[0],
                    text_width_slice[1],
                    -1,
                    1);
}

void
AnnotationRenderer::DrawSelectionHandle(AbstractRenderContext *context, const Vector3d &xSlice)
{
  // Determine the width of the line
  double vppr = m_Model->GetParent()->GetSizeReporter()->GetViewportPixelRatio();
  auto offset = context->MapScreenOffsetToWorldOffset(Vector2d(4., 4.));

  context->SetBrush(Vector3d(1,1,1), 1.0);
  context->SetPenColor(Vector3d(0,0,0), 1.0);
  context->SetPenWidth(1);
  context->DrawRect(xSlice[0] - offset[0], xSlice[1] - offset[1], 2 * offset[0], 2 * offset[1]);
}

void
AnnotationRenderer::RenderOverTiledLayer(AbstractRenderContext *context,
                                            ImageWrapperBase         *base_layer,
                                            const SubViewport        &vp)
{
  // Get appearance settings
  auto *parentModel = m_Model->GetParent();
  auto *gs = m_Model->GetParent()->GetDriver()->GetGlobalState();

  // Get the opacity of the annotations, and stop if it is zero
  double alpha = gs->GetAnnotationAlpha();
  if (alpha == 0 || vp.isThumbnail)
    return;

  // Set up the rendering properties
  auto  *rps = AbstractRenderer::GetPlatformSupport();
  auto font_info = rps->MakeFont(12, AbstractRendererPlatformSupport::TYPEWRITER);

  // Get the color for the annotations
  Vector3d ann_color = gs->GetAnnotationColor();

  // Shared settings for text drawing
  auto text_offset_slice = context->MapScreenOffsetToWorldOffset(Vector2d(5., 5.));
  auto text_width_slice = context->MapScreenOffsetToWorldOffset(Vector2d(96., 12.));

  // Get the list of annotations
  const ImageAnnotationData *adata = m_Model->GetAnnotations();

  // Draw current line - the current line can either be a ruler or an annotation arrow
  if (m_Model->GetFlagDrawingLine())
  {
    const auto &curr_line = m_Model->GetCurrentLine();

    // Draw the line and the endpoints

    // Draw the current line's points
    context->SetPenColor(ann_color, alpha);
    context->SetPenWidth(3);

    // First point and last point
    context->DrawPoint(curr_line.first[0], curr_line.first[1]);
    context->DrawPoint(curr_line.second[0], curr_line.second[1]);

    // Midpoint only drawn in ruler mode
    if (m_Model->GetAnnotationMode() == ANNOTATION_RULER)
      context->DrawPoint(0.5 * (curr_line.first[0] + curr_line.second[0]),
                         0.5 * (curr_line.first[1] + curr_line.second[1]));

    // Draw the line itself
    context->SetPenWidth(1);
    context->SetPenLineType(vtkPen::DOT_LINE);
    context->DrawLine(
      curr_line.first[0], curr_line.first[1], curr_line.second[0], curr_line.second[1]);

    // Decoration drawn only in ruler mode
    if (m_Model->GetAnnotationMode() == ANNOTATION_RULER)
    {
      // Draw the current line length
      DrawLineLength(context, curr_line.first, curr_line.second, ann_color, alpha);
    }
  } // Current line valid

  // Draw each annotation
  for (auto it = adata->GetAnnotations().begin(); it != adata->GetAnnotations().end(); ++it)
  {
    if (m_Model->IsAnnotationVisible(*it))
    {
      // Draw all the line segments
      auto *lsa = dynamic_cast<annot::LineSegmentAnnotation *>(it->GetPointer());
      if (lsa)
      {
        // Draw the line
        Vector3d p1 = parentModel->MapImageToSlice(lsa->GetSegment().first);
        Vector3d p2 = parentModel->MapImageToSlice(lsa->GetSegment().second);

        Vector3d color = lsa->GetColor();
        context->SetPenColor(color, alpha);
        context->SetPenWidth(3);
        context->DrawPoint((p1[0] + p2[0]) * 0.5, (p1[1] + p2[1]) * 0.5);

        context->SetPenWidth(1);
        context->SetPenLineType(vtkPen::SOLID_LINE);
        context->DrawLine(p1[0], p1[1], p2[0], p2[1]);

        if (lsa->GetSelected() && m_Model->IsAnnotationModeActive() &&
            m_Model->GetAnnotationMode() == ANNOTATION_SELECT)
        {
          this->DrawSelectionHandle(context, p1);
          this->DrawSelectionHandle(context, p2);
        }

        // Draw length or angle
        if (m_Model->IsDrawingRuler())
        {
          // Draw angle:
          // Compute the dot product and no need for the third components that are zeros
          double             angle = m_Model->GetAngleWithCurrentLine(lsa);
          std::ostringstream oss_angle;
          oss_angle << std::setprecision(3) << angle << "°";

          Vector3d line_center = m_Model->GetAnnotationCenter(lsa);

          // Draw the angle text
          context->SetFont(font_info);
          context->SetPenColor(lsa->GetColor(), alpha);
          context->DrawText(oss_angle.str(),
                            line_center[0] + text_offset_slice[0],
                            line_center[1] + text_offset_slice[1],
                            text_width_slice[0],
                            text_width_slice[1],
                            -1,
                            1);
        }
        else
        {
          this->DrawLineLength(context, p1, p2, lsa->GetColor(), alpha);
        }
      }

      auto *lma = dynamic_cast<annot::LandmarkAnnotation *>(it->GetPointer());
      if (lma)
      {
        // Get the head and tail coordinate in slice units
        Vector3d xHeadSlice, xTailSlice;
        m_Model->GetLandmarkArrowPoints(lma->GetLandmark(), xHeadSlice, xTailSlice);

        std::string text = lma->GetLandmark().Text;
        Vector3d    color = lma->GetColor();

        // Draw the annotation line segment
        context->SetPenColor(color, alpha);
        context->SetPenLineType(vtkPen::SOLID_LINE);
        context->DrawLine(xHeadSlice[0], xHeadSlice[1], xTailSlice[0], xTailSlice[1]);

        if (lma->GetSelected() && m_Model->IsAnnotationModeActive() &&
            m_Model->GetAnnotationMode() == ANNOTATION_SELECT)
        {
          this->DrawSelectionHandle(context, xHeadSlice);
          this->DrawSelectionHandle(context, xTailSlice);
        }

        // Text box size in slice coordinate units
        font_info = rps->MakeFont(12, AbstractRendererPlatformSupport::SANS);
        context->SetFont(font_info);
        auto xTextSizeSlice = context->MapScreenOffsetToWorldOffset(
          Vector2d(context->TextWidth(text), font_info.pixel_size));

        // How to position the text
        double xbox, ybox;
        int    align_horiz, align_vert;
        if (fabs(lma->GetLandmark().Offset[0]) >= fabs(lma->GetLandmark().Offset[1]))
        {
          align_vert = 0;
          ybox = xTailSlice[1] - xTextSizeSlice[1] / 2;
          if (lma->GetLandmark().Offset[0] >= 0)
          {
            align_horiz = -1;
            xbox = xTailSlice[0];
          }
          else
          {
            align_horiz = 1;
            xbox = xTailSlice[0] - xTextSizeSlice[0];
          }
        }
        else
        {
          align_horiz = 0;
          xbox = xTailSlice[0] - xTextSizeSlice[0] / 2;
          if (lma->GetLandmark().Offset[1] >= 0)
          {
            align_vert = -1;
            ybox = xTailSlice[1];
          }
          else
          {
            align_vert = 1;
            ybox = xTailSlice[1] - xTextSizeSlice[1];
          }
        }

        // Draw the text at the right location
        context->SetPenColor(lma->GetColor(), alpha);
        context->DrawText(
          text, xbox, ybox, xTextSizeSlice[0], xTextSizeSlice[1], align_horiz, align_vert);
      }
    }
  }
}


