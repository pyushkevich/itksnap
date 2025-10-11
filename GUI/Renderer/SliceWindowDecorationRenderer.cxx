#include "SliceWindowDecorationRenderer.h"
#include "GenericSliceModel.h"
#include "SNAPAppearanceSettings.h"
#include "GlobalUIModel.h"
#include "IRISException.h"
#include "IRISApplication.h"
#include "GenericImageData.h"
#include "DisplayLayoutModel.h"
#include "GenericImageData.h"
#include "TimePointProperties.h"


void
SliceWindowDecorationRenderer::RenderOverMainViewport(AbstractRenderContext *context)
{   
  if (!m_Model || !m_Model->IsSliceInitialized()) 
    return;
   
  // Push an identity transform in logical pixel units
  double vppr = m_Model->GetSizeReporter()->GetViewportPixelRatio();
  context->PushMatrix();
  context->Scale(vppr, vppr);
  DrawRuler(context);
  DrawNicknames(context);
  DrawOrientationLabels(context);
  context->PopMatrix();
}

void
SliceWindowDecorationRenderer::RenderOverTiledLayer(AbstractRenderContext *context,
                                                       ImageWrapperBase         *base_layer,
                                                       const SubViewport        &vp)
{}

void
SliceWindowDecorationRenderer::DrawRuler(AbstractRenderContext *context)
{
  auto *as = m_Model->GetParentUI()->GetAppearanceSettings();
  auto *eltRuler = as->GetUIElement(SNAPAppearanceSettings::RULER);
  if (eltRuler->GetVisible())
  {
    // Get the dimensions of the non-thumbnail area where the decorations go
    Vector2ui vp_pos, vp_size;
    double    vppr = m_Model->GetSizeReporter()->GetViewportPixelRatio();
    m_Model->GetNonThumbnailViewport(vp_pos, vp_size);

    // Convert into logical pixel units
    Vector2d vp = to_double(vp_size) / vppr;

    // The ruler bar should be as large as possible but less than one half
    // of the screen width (not to go over the markers)
    double maxw = 0.5 * vp[0] - 20.0;
    maxw = maxw < 5 ? 5 : maxw;

    double zoom = m_Model->GetViewZoom() / vppr;
    double scale = 1.0;
    while (zoom * scale > maxw)
      scale /= 10.0;
    while (zoom * scale < 0.1 * maxw)
      scale *= 10.0;

    // Draw a zoom bar
    double bw = scale * zoom;

    // Based on the log of the scale, determine the unit
    string unit = "mm";
    if (scale >= 10 && scale < 1000)
    {
      unit = "cm";
      scale /= 10;
    }
    else if (scale >= 1000)
    {
      unit = "m";
      scale /= 1000;
    }
    else if (scale >= 1000000)
    {
      unit = "km";
      scale /= 1000000;
    }
    else if (scale < 1 && scale > 0.001)
    {
      unit = "\xb5m";
      scale *= 1000;
    }
    else if (scale < 0.001)
    {
      unit = "nm";
      scale *= 1000000;
    }

    std::ostringstream oss;
    oss << scale << " " << unit;

    // Draw the ruler itself
    context->SetPenAppearance(*eltRuler);
    context->DrawLine(vp[0] - 5, 5, vp[0] - 5, 15);
    context->DrawLine(vp[0] - 5, 10, vp[0] - (5 + bw), 10);
    context->DrawLine(vp[0] - (5 + bw), 5, vp[0] - (5 + bw), 15);

    // Apply font properties
    int font_size = eltRuler->GetFontSize();

    // Create the font info
    AbstractRendererPlatformSupport::FontInfo font_info = { AbstractRendererPlatformSupport::SANS,
                                                            font_size,
                                                            false };
    context->SetFont(font_info);

    // See if we can squeeze the label under the ruler
    if (bw > font_size * 4)
    {
      context->DrawText(oss.str().c_str(), vp[0] - (bw + 10), 12, (int)bw, font_size + 8, 0, -1);
    }
    else
    {
      context->DrawText(oss.str().c_str(),
                        vp[0] - (int)(2 * bw + font_size * 4 + 20),
                        5,
                        (int)(bw + font_size * 4 + 10),
                        font_size,
                        1,
                        0);
    }
  }
}

void
SliceWindowDecorationRenderer::DrawNicknames(AbstractRenderContext *context)
{
  // Draw the nicknames
  DisplayLayoutModel *dlm = m_Model->GetParentUI()->GetDisplayLayoutModel();
  Vector2ui           layout = dlm->GetSliceViewLayerTilingModel()->GetValue();
  int                 nrows = (int)layout[0];
  int                 ncols = (int)layout[1];

  // Get the properties for the labels
  auto       *as = m_Model->GetParentUI()->GetAppearanceSettings();
  const auto *elt = as->GetUIElement(SNAPAppearanceSettings::RULER);

  // Leave if the labels are disabled
  if (!elt->GetVisible())
    return;

  // Viewport properties (retina-related)
  Vector2ui vp_pos, vp_size;
  m_Model->GetNonThumbnailViewport(vp_pos, vp_size);

  // Apply line settings
  context->SetPenAppearance(*elt);

  // Get the viewport size
  double vppr = m_Model->GetSizeReporter()->GetViewportPixelRatio();
  int    w = vp_size[0] / (vppr * ncols), h = vp_size[1] / (vppr * nrows);

  // For non-tiled mode, set the offset to equal that of the anatomical markers
  int left_margin = elt->GetFontSize() / 3;
  int right_margin = left_margin + 2 + elt->GetFontSize() / 2;

  // Set the maximum allowed width. For tiled layout, this is the size of the tile,
  // for stacked, it is the half-width of the tile, to leave space for the anatomic
  // marker
  int max_allowed_width =
    (ncols == 1) ? (int)(0.5 * w - left_margin - right_margin) : (int)(0.92 * w);

  // Get the renderer platform support
  auto *rps = AbstractRenderer::GetPlatformSupport();
  auto  font_info = rps->MakeFont(elt->GetFontSize(), AbstractRendererPlatformSupport::SANS);
  context->SetFont(font_info);

  // Place to keep the strings
  std::list<std::list<std::string>> disp_names;

  // Find the longest nickname
  int maxwidth = 0;
  for (int i = 0; i < nrows; i++)
  {
    for (int j = 0; j < ncols; j++)
    {
      // Define the ROI for this label
      ImageWrapperBase *layer = m_Model->GetLayerForNthTile(i, j);
      if (layer)
      {
        disp_names.push_back(this->GetDisplayText(layer));
        for (auto it : disp_names.back())
        {
          int fw = context->TextWidth(it);
          // int fw = rps->MeasureTextWidth(it.c_str(), font_info) / GetVPPR();
          if (fw > maxwidth)
            maxwidth = fw;
        }
      }
    }
  }

  // Adjust the font size
  if (maxwidth > max_allowed_width)
    font_info.pixel_size = (int)(font_info.pixel_size * max_allowed_width / maxwidth);

  // Draw each nickname
  auto it_disp_names = disp_names.begin();
  for (int i = 0; i < nrows; i++)
  {
    for (int j = 0; j < ncols; j++)
    {
      // Define the ROI for this label
      ImageWrapperBase *layer = m_Model->GetLayerForNthTile(i, j);
      if (layer)
      {
        int v_offset = 0;
        for (auto it : *it_disp_names)
        {
          // If there is only one column, we render the text on the left
          if (ncols == 1)
          {
            context->DrawText(it,
                              left_margin,
                              h * (nrows - i) - 18 - v_offset,
                              w,
                              15,
                              AbstractRendererPlatformSupport::LEFT,
                              AbstractRendererPlatformSupport::TOP);
            /*
            this->DrawStringRect(painter,
                                 it,
                                 left_margin,
                                 h * (nrows - i) - 18 - v_offset,
                                 w,
                                 15,
                                 font_info,
                                 AbstractRendererPlatformSupport::LEFT,
                                 AbstractRendererPlatformSupport::TOP,
                                 elt->GetColor(),
                                 elt->GetAlpha());*/
          }
          else
          {
            context->DrawText(it,
                              w * j,
                              h * (nrows - i) - 20 - v_offset,
                              w,
                              15,
                              AbstractRendererPlatformSupport::HCENTER,
                              AbstractRendererPlatformSupport::TOP);
            /*
            this->DrawStringRect(painter,
                                 it,
                                 w * j,
                                 h * (nrows - i) - 20 - v_offset,
                                 w,
                                 15,
                                 font_info,
                                 AbstractRendererPlatformSupport::HCENTER,
                                 AbstractRendererPlatformSupport::TOP,
                                 elt->GetColor(),
                                 elt->GetAlpha());*/
          }

          v_offset += (int)(font_info.pixel_size * 1.34);
        }
        ++it_disp_names;
      }
    }
  }
}

std::list<std::string>
SliceWindowDecorationRenderer::GetDisplayText(ImageWrapperBase *layer)
{
  // Lines to be returned
  std::list<std::string> lines;
  int                    nc = layer->GetNumberOfComponents();

  // Get the nickname of the layer itself - this is the first line
  std::string nickname = CapStringLength(layer->GetNickname(), nc > 1 ? 20 : 28);
  if (nc > 1)
  {
    AbstractMultiChannelDisplayMappingPolicy *policy =
      static_cast<AbstractMultiChannelDisplayMappingPolicy *>(layer->GetDisplayMapping());
    MultiChannelDisplayMode mode = policy->GetDisplayMode();
    if (mode.UseRGB)
      nickname += " [RGB]";
    else if (mode.RenderAsGrid)
      nickname += " [Grid]";
    else if (mode.SelectedScalarRep == SCALAR_REP_MAGNITUDE)
      nickname += " [Mag]";
    else if (mode.SelectedScalarRep == SCALAR_REP_MAX)
      nickname += " [Max]";
    else if (mode.SelectedScalarRep == SCALAR_REP_AVERAGE)
      nickname += " [Avg]";
    else
    {
      std::ostringstream oss;
      oss << " [" << mode.SelectedComponent + 1 << "/" << nc << "]";
      nickname += oss.str();
    }
  }

  // Get the time point for the image
  if (layer->GetNumberOfTimePoints() > 1)
  {
    std::ostringstream oss_tpn; // timepoint nickname stream
    uint32_t           crnt_tp_ind =
      m_Model->GetDriver()->GetCursorTimePoint() + 1; // cursor is 0-based, tp is 1-based
    auto        tpp = m_Model->GetDriver()->GetCurrentImageData()->GetTimePointProperties();
    std::string tpn = tpp->GetProperty(crnt_tp_ind)->GetNickname();
    if (tpn.size() > 0) // only append a space when nickname exists
      tpn = tpn + ' ';

    std::ostringstream oss;
    oss << " [" << tpn << layer->GetTimePointIndex() + 1 << "/" << layer->GetNumberOfTimePoints()
        << "]";
    nickname += oss.str();
  }

  lines.push_back(nickname);

  // Get the nickname of the segmentation if there are multiple segmentations
  IRISApplication *app = m_Model->GetDriver();
  if (app->GetCurrentImageData()->GetNumberOfLayers(LABEL_ROLE) > 1)
    lines.push_back(CapStringLength(app->GetSelectedSegmentationLayer()->GetNickname(), 28));

  return lines;
}

std::string
SliceWindowDecorationRenderer::CapStringLength(const std::string &str, size_t max_size)
{
  if (str.size() <= max_size)
    return str;

  std::string strout = str.substr(0, max_size - 1);
  strout += "\u2026";
  return strout;
}

void
SliceWindowDecorationRenderer::DrawOrientationLabels(AbstractRenderContext *context)
{

  // The letter labels
  static const char *letters[3][2] = { { "R", "L" }, { "A", "P" }, { "I", "S" } };
  const char        *labels[2][2];

  // Get the properties for the labels
  auto *as = m_Model->GetParentUI()->GetAppearanceSettings();
  auto *elt = as->GetUIElement(SNAPAppearanceSettings::MARKERS);

  // Leave if the labels are disabled
  if (!elt->GetVisible())
    return;

  // Repeat for X and Y directions
  for (unsigned int i = 0; i < 2; i++)
  {
    // Which axis are we on in anatomy space?
    unsigned int anatomyAxis =
      m_Model->GetDisplayToAnatomyTransform()->GetCoordinateIndexZeroBased(i);

    // Which direction is the axis facing (returns -1 or 1)
    unsigned int anatomyAxisDirection =
      m_Model->GetDisplayToAnatomyTransform()->GetCoordinateOrientation(i);

    // Map the direction onto 0 or 1
    unsigned int letterIndex = (1 + anatomyAxisDirection) >> 1;

    // Compute the two labels for this axis
    labels[i][0] = letters[anatomyAxis][1 - letterIndex];
    labels[i][1] = letters[anatomyAxis][letterIndex];
  }

  double vppr = m_Model->GetSizeReporter()->GetViewportPixelRatio();

  Vector2ui vp_pos, vp_size;
  m_Model->GetNonThumbnailViewport(vp_pos, vp_size);

  // Get the various sizes and offsets
  int offset = 4 + elt->GetFontSize();
  int margin = elt->GetFontSize() / 3;
  int w = vp_size[0] / vppr, h = vp_size[1] / vppr;

  // Create the font info
  auto *rps = AbstractRenderer::GetPlatformSupport();
  context->SetPenColor(elt->GetColor(), elt->GetAlpha());
  context->SetFont(rps->MakeFont(
    (int)(elt->GetFontSize()), AbstractRendererPlatformSupport::TYPEWRITER, true));

  // Use the delegate to draw text
  context->DrawText(labels[0][0], margin, (h - offset) / 2, offset, offset, -1, 0);
  context->DrawText(labels[0][1], w - (offset + margin), (h - offset) / 2, offset, offset, 1, 0);
  context->DrawText(labels[1][0], (w - offset) / 2, 0, offset, offset, 0, -1);
  context->DrawText(labels[1][1], (w - offset) / 2, h - (offset + 1), offset, offset, 0, 1);
}
