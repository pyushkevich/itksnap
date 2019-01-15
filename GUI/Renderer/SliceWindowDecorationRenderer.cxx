#include "SliceWindowDecorationRenderer.h"
#include "GenericSliceModel.h"
#include "SNAPAppearanceSettings.h"
#include "GlobalUIModel.h"
#include "GenericSliceRenderer.h"
#include "IRISException.h"
#include "IRISApplication.h"
#include "DisplayLayoutModel.h"
#include "GenericImageData.h"

SliceWindowDecorationRenderer::SliceWindowDecorationRenderer()
{
}

SliceWindowDecorationRenderer::~SliceWindowDecorationRenderer()
{

}

void SliceWindowDecorationRenderer::paintGL()
{
  DrawOrientationLabels();
  DrawRulers();
  DrawNicknames();

}

void SliceWindowDecorationRenderer::DrawOrientationLabels()
{
  GenericSliceModel *parentModel = this->GetParentRenderer()->GetModel();
  SNAPAppearanceSettings *as =
      parentModel->GetParentUI()->GetAppearanceSettings();

  // The letter labels
  static const char *letters[3][2] = {{"R","L"},{"A","P"},{"I","S"}};
  const char *labels[2][2];

  // Get the properties for the labels
  const OpenGLAppearanceElement *elt =
      as->GetUIElement(SNAPAppearanceSettings::MARKERS);

  // Leave if the labels are disabled
  if(!elt->GetVisible()) return;

  // Repeat for X and Y directions
  for(unsigned int i=0;i<2;i++)
    {
    // Which axis are we on in anatomy space?
    unsigned int anatomyAxis =
        parentModel->GetDisplayToAnatomyTransform()->GetCoordinateIndexZeroBased(i);

    // Which direction is the axis facing (returns -1 or 1)
    unsigned int anatomyAxisDirection =
        parentModel->GetDisplayToAnatomyTransform()->GetCoordinateOrientation(i);

    // Map the direction onto 0 or 1
    unsigned int letterIndex = (1 + anatomyAxisDirection) >> 1;

    // Compute the two labels for this axis
    labels[i][0] = letters[anatomyAxis][1-letterIndex];
    labels[i][1] = letters[anatomyAxis][letterIndex];
    }

  double vppr = parentModel->GetSizeReporter()->GetViewportPixelRatio();

  Vector2ui vp_pos, vp_size;
  parentModel->GetNonThumbnailViewport(vp_pos, vp_size);

  glPushAttrib(GL_COLOR_BUFFER_BIT | GL_CURRENT_BIT | GL_DEPTH_BUFFER_BIT);
  glPushMatrix();
  glLoadIdentity();
  glScaled(vppr, vppr, 1.0);

  glEnable(GL_BLEND);
  glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

  // Get the various sizes and offsets
  int offset = 4 + elt->GetFontSize();
  int margin = elt->GetFontSize() / 3;
  int w = vp_size[0] / vppr, h = vp_size[1] / vppr;

  // Create the font info
  AbstractRendererPlatformSupport::FontInfo font_info =
        { AbstractRendererPlatformSupport::TYPEWRITER, (int) (elt->GetFontSize() * vppr), true };

  // Use the delegate to draw text
  this->m_PlatformSupport->RenderTextInOpenGL(
        labels[0][0], margin, (h-offset)/2, offset, offset, font_info, -1, 0,
        elt->GetColor(), elt->GetAlpha());

  this->m_PlatformSupport->RenderTextInOpenGL(
        labels[0][1], w - (offset+margin), (h-offset)/2, offset, offset, font_info, 1, 0,
        elt->GetColor(), elt->GetAlpha());

  this->m_PlatformSupport->RenderTextInOpenGL(
        labels[1][0], (w-offset)/2, 0, offset, offset, font_info, 0, -1,
        elt->GetColor(), elt->GetAlpha());

  this->m_PlatformSupport->RenderTextInOpenGL(
        labels[1][1], (w-offset)/2, h - (offset+1), offset, offset, font_info, 0, 1,
        elt->GetColor(), elt->GetAlpha());

  glPopMatrix();
  glPopAttrib();
}

std::string
SliceWindowDecorationRenderer::CapStringLength(const std::string &str, int max_size)
{
  if(str.size() <= max_size)
    return str;

  std::string strout = str.substr(0, max_size - 1);
  strout += "\u2026";
  return strout;
}

SliceWindowDecorationRenderer::StringList
SliceWindowDecorationRenderer::GetDisplayText(ImageWrapperBase *layer)
{
  // Lines to be returned
  StringList lines;
  int nc = layer->GetNumberOfComponents();

  // Get the nickname of the layer itself - this is the first line
  std::string nickname = CapStringLength(layer->GetNickname(), nc > 1 ? 20 : 28);
  if(nc > 1)
    {
    AbstractMultiChannelDisplayMappingPolicy *policy =
        static_cast<AbstractMultiChannelDisplayMappingPolicy *>(layer->GetDisplayMapping());
    MultiChannelDisplayMode mode = policy->GetDisplayMode();
    if(mode.UseRGB)
      nickname += " [RGB]";
    else if(mode.RenderAsGrid)
      nickname += " [Grid]";
    else if(mode.SelectedScalarRep == SCALAR_REP_MAGNITUDE)
      nickname += " [Mag]";
    else if(mode.SelectedScalarRep == SCALAR_REP_MAX)
      nickname += " [Max]";
    else if(mode.SelectedScalarRep == SCALAR_REP_AVERAGE)
      nickname += " [Avg]";
    else
      {
      std::ostringstream oss;
      oss << " [" << mode.SelectedComponent + 1 << "/" << nc <<  "]";
      nickname += oss.str();
      }
    }
  lines.push_back(nickname);

  // Get the nickname of the segmentation if there are multiple segmentations
  IRISApplication *app = this->GetParentRenderer()->GetModel()->GetDriver();
  if(app->GetCurrentImageData()->GetNumberOfLayers(LABEL_ROLE) > 1)
    lines.push_back(CapStringLength(app->GetSelectedSegmentationLayer()->GetNickname(), 28));

  return lines;
}

void SliceWindowDecorationRenderer::DrawNicknames()
{
  // Draw the nicknames
  GenericSliceModel *parentModel = this->GetParentRenderer()->GetModel();
  DisplayLayoutModel *dlm = parentModel->GetParentUI()->GetDisplayLayoutModel();
  Vector2ui layout = dlm->GetSliceViewLayerTilingModel()->GetValue();
  int nrows = (int) layout[0];
  int ncols = (int) layout[1];

  // Get the properties for the labels
  SNAPAppearanceSettings *as =
      parentModel->GetParentUI()->GetAppearanceSettings();

  const OpenGLAppearanceElement *elt =
      as->GetUIElement(SNAPAppearanceSettings::RULER);

  // Leave if the labels are disabled
  if(!elt->GetVisible())
    return;

  // Leave if there is trivial information to show
  // ### if(dlm->GetNumberOfGroundLevelLayers() < 2) return;

  // Viewport properties (retina-related)
  double vppr = parentModel->GetSizeReporter()->GetViewportPixelRatio();

  Vector2ui vp_pos, vp_size;
  parentModel->GetNonThumbnailViewport(vp_pos, vp_size);

  // Apply the label properties
  glPushAttrib(GL_COLOR_BUFFER_BIT | GL_CURRENT_BIT | GL_DEPTH_BUFFER_BIT);
  glPushMatrix();
  glLoadIdentity();
  glScaled(vppr, vppr, 1.0);

  elt->ApplyLineSettings();
  elt->ApplyColor();

  // Get the viewport size
  int w = vp_size[0] / (vppr * ncols), h = vp_size[1] / (vppr * nrows);

  // For non-tiled mode, set the offset to equal that of the anatomical markers
  const OpenGLAppearanceElement *elt_marker = as->GetUIElement(SNAPAppearanceSettings::RULER);
  int left_margin = elt_marker->GetFontSize() / 3;
  int right_margin = left_margin + 2 + elt_marker->GetFontSize() / 2;

  // Set the maximum allowed width. For tiled layout, this is the size of the tile,
  // for stacked, it is the half-width of the tile, to leave space for the anatomic
  // marker
  int max_allowed_width = (ncols == 1)
                          ? (int) (0.5 * w - left_margin - right_margin)
                          : (int) (0.92 * w);


  AbstractRendererPlatformSupport::FontInfo font_info =
        { AbstractRendererPlatformSupport::SANS, (int)(elt->GetFontSize() * vppr), false };

  // Find the longest nickname
  int maxwidth = 0;
  for(int i = 0; i < nrows; i++)
    {
    for(int j = 0; j < ncols; j++)
      {
      // Define the ROI for this label
      ImageWrapperBase *layer =
          parentModel->GetLayerForNthTile(i, j);

      if(layer)
        {
        StringList nick_lines = this->GetDisplayText(layer);
        for(StringListCIter it = nick_lines.begin(); it != nick_lines.end(); ++it)
          {
          int fw = this->m_PlatformSupport->MeasureTextWidth(it->c_str(), font_info) / vppr;
          if(fw > maxwidth)
            maxwidth = fw;
          }
        }
      }
    }

  // Adjust the font size
  if(maxwidth > max_allowed_width)
    {
    font_info.pixel_size = (int) (font_info.pixel_size * max_allowed_width / maxwidth);
    }

  // Draw each nickname
  for(int i = 0; i < nrows; i++)
    {
    for(int j = 0; j < ncols; j++)
      {
      // Define the ROI for this label
      ImageWrapperBase *layer =
          parentModel->GetLayerForNthTile(i, j);
      if(layer)
        {
        StringList nick_lines = this->GetDisplayText(layer);
        int v_offset = 0;
        for(StringListCIter it = nick_lines.begin(); it != nick_lines.end(); ++it)
          {
          // If there is only one column, we render the text on the left
          if(ncols == 1)
            {
            this->m_PlatformSupport->RenderTextInOpenGL(
                  it->c_str(),
                  left_margin, h * (nrows - i) - 18 - v_offset, w, 15, font_info,
                  AbstractRendererPlatformSupport::LEFT,
                  AbstractRendererPlatformSupport::TOP,
                  elt->GetColor(), elt->GetAlpha());
            }
          else
            {
            this->m_PlatformSupport->RenderTextInOpenGL(
                  it->c_str(),
                  w * j, h * (nrows - i) - 20 - v_offset, w, 15, font_info,
                  AbstractRendererPlatformSupport::HCENTER,
                  AbstractRendererPlatformSupport::TOP,
                  elt->GetColor(), elt->GetAlpha());
            }
          v_offset += (int) (font_info.pixel_size * 1.34 / vppr);
          }
        }
      }
    }


  glPopMatrix();
  glPopAttrib();
}

void SliceWindowDecorationRenderer::DrawRulers()
{
  GenericSliceModel *parentModel = this->GetParentRenderer()->GetModel();
  SNAPAppearanceSettings *as =
      parentModel->GetParentUI()->GetAppearanceSettings();

  // Get the properties for the labels
  const OpenGLAppearanceElement *elt =
      as->GetUIElement(SNAPAppearanceSettings::RULER);

  // Leave if the labels are disabled
  if(!elt->GetVisible()) return;

  // Get the viewport properties (retina-capable)
  float vppr = parentModel->GetSizeReporter()->GetViewportPixelRatio();

  // Get the dimensions of the non-thumbnail area where the decorations go
  Vector2ui vp_pos, vp_size;
  parentModel->GetNonThumbnailViewport(vp_pos, vp_size);

  // Convert into logical pixel units
  Vector2f vp = to_float(vp_size) / vppr;

  glPushAttrib(GL_COLOR_BUFFER_BIT | GL_CURRENT_BIT | GL_DEPTH_BUFFER_BIT);
  glPushMatrix();
  glLoadIdentity();
  glScaled(vppr, vppr, 1.0);

  elt->ApplyLineSettings();
  elt->ApplyColor();

  // The ruler bar should be as large as possible but less than one half
  // of the screen width (not to go over the markers)
  double maxw = 0.5 * vp[0] - 20.0;
  maxw = maxw < 5 ? 5 : maxw;

  double zoom = parentModel->GetViewZoom() / vppr;
  double scale = 1.0;
  while(zoom * scale > maxw) scale /= 10.0;
  while(zoom * scale < 0.1 * maxw) scale *= 10.0;

  // Draw a zoom bar
  double bw = scale * zoom;
  glBegin(GL_LINES);
  glVertex2d(vp[0] - 5,5);
  glVertex2d(vp[0] - 5,15);
  glVertex2d(vp[0] - 5,10);
  glVertex2d(vp[0] - (5 + bw),10);
  glVertex2d(vp[0] - (5 + bw),5);
  glVertex2d(vp[0] - (5 + bw),15);
  glEnd();

  // Based on the log of the scale, determine the unit
  string unit = "mm";
  if(scale >= 10 && scale < 1000)
    { unit = "cm"; scale /= 10; }
  else if(scale >= 1000)
    { unit = "m"; scale /= 1000; }
  else if(scale >= 1000000)
    { unit = "km"; scale /= 1000000; }
  else if(scale < 1 && scale > 0.001)
    { unit = "\xb5m"; scale *= 1000; }
  else if(scale < 0.001)
    { unit = "nm"; scale *= 1000000; }

  std::ostringstream oss;
  oss << scale << " " << unit;

  // Get the fontsize in GL pixels
  int font_size = elt->GetFontSize();

  // Create the font info
  AbstractRendererPlatformSupport::FontInfo font_info =
        { AbstractRendererPlatformSupport::SANS, (int)(font_size * vppr), false };

  // See if we can squeeze the label under the ruler
  if(bw > font_size * 4)
    {
    this->m_PlatformSupport->RenderTextInOpenGL(
        oss.str().c_str(),
        vp[0]-(bw+10), 12, (int) bw, font_size+8,
        font_info, 0, -1, elt->GetColor(), elt->GetAlpha());
    }
  else
    {
    this->m_PlatformSupport->RenderTextInOpenGL(
          oss.str().c_str(),
          vp[0] - (int) (2 * bw + font_size * 4 + 20), 5,
          (int) (bw + font_size * 4+10), font_size,
          font_info, 1, 0, elt->GetColor(), elt->GetAlpha());
    }

  glPopMatrix();
  glPopAttrib();
}
