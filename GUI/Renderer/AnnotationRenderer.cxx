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
    auto *rps = AbstractRenderer::GetPlatformSupport();
    auto font_info = rps->MakeFont(12 * GetVPPR(),
                                   AbstractRendererPlatformSupport::TYPEWRITER);

    Vector3d curr_center = (xSlice1 + xSlice2) * 0.5;

    // Draw the length text
    this->DrawStringRect(painter,
          oss_length.str(),
          curr_center[0] + text_offset_slice[0], curr_center[1] + text_offset_slice[1],
          text_width_slice[0], text_width_slice[1],
          font_info, -1, 1, color, alpha);
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
    auto *gs = m_Model->GetDriver()->GetGlobalState();

    // Get the opacity of the annotations, and stop if it is zero
    double alpha = gs->GetAnnotationAlpha();
    if(alpha == 0)
      return false;

    // Set up the rendering properties
    auto *rps = AbstractRenderer::GetPlatformSupport();
    auto font_info = rps->MakeFont(12 * GetVPPR(),
                                   AbstractRendererPlatformSupport::TYPEWRITER);

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

            // Draw the angle text
            this->DrawStringRect(painter, oss_angle.str(),
                                 line_center[0] + text_offset_slice[0],
                                 line_center[1] + text_offset_slice[1],
                                 text_width_slice[0], text_width_slice[1],
                                 font_info, -1, 1, lsa->GetColor(), alpha);
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

          // Text box size in slice coordinate units
          Vector2d xTextSizeSlice(
                AbstractRenderer::GetPlatformSupport()->MeasureTextWidth(text.c_str(), font_info),
                font_info.pixel_size * GetVPPR());

          // How to position the text
          double xbox, ybox;
          int align_horiz, align_vert;
          if(fabs(lma->GetLandmark().Offset[0]) >= fabs(lma->GetLandmark().Offset[1]))
            {
            align_vert = 0;
            ybox = xTailSlice[1] - xTextSizeSlice[1] / 2;
            if(lma->GetLandmark().Offset[0] >= 0)
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
            if(lma->GetLandmark().Offset[1] >= 0)
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
          font_info = rps->MakeFont(12 * GetVPPR(),
                                    AbstractRendererPlatformSupport::SANS);
          this->DrawStringRect(painter, text,
                               xbox, ybox,
                               xTextSizeSlice[0], xTextSizeSlice[1], font_info,
                               align_horiz, align_vert, lma->GetColor(), alpha);
          }

        }
      }

    return true;
  }

protected:

  AnnotationModel *m_AnnotationModel;

};


vtkStandardNewMacro(AnnotationContextItem);


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

AnnotationRenderer::AnnotationRenderer()
{
  m_Model = nullptr;
}


