#include "SliceWindowDecorationRenderer.h"
#include "GenericSliceModel.h"
#include "SNAPAppearanceSettings.h"
#include "GlobalUIModel.h"
#include "GenericSliceRenderer.h"
#include "IRISException.h"
#include "IRISApplication.h"

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
  const SNAPAppearanceSettings::Element &elt =
      as->GetUIElement(SNAPAppearanceSettings::MARKERS);

  // Leave if the labels are disabled
  if(!elt.Visible) return;

  // Repeat for X and Y directions
  for(unsigned int i=0;i<2;i++)
    {
    // Which axis are we on in anatomy space?
    unsigned int anatomyAxis =
        parentModel->GetDisplayToAnatomyTransform().GetCoordinateIndexZeroBased(i);

    // Which direction is the axis facing (returns -1 or 1)
    unsigned int anatomyAxisDirection =
        parentModel->GetDisplayToAnatomyTransform().GetCoordinateOrientation(i);

    // Map the direction onto 0 or 1
    unsigned int letterIndex = (1 + anatomyAxisDirection) >> 1;

    // Compute the two labels for this axis
    labels[i][0] = letters[anatomyAxis][1-letterIndex];
    labels[i][1] = letters[anatomyAxis][letterIndex];
    }

  glPushAttrib(GL_COLOR_BUFFER_BIT | GL_CURRENT_BIT | GL_DEPTH_BUFFER_BIT);
  glPushMatrix();
  glLoadIdentity();

  glEnable(GL_BLEND);
  glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

  // glColor4d( elt.NormalColor[0], elt.NormalColor[1], elt.NormalColor[2], 1.0 );

  // Set the text drawing color
  unsigned char rgba[] = {
    (unsigned char)(255 * elt.NormalColor[0]),
    (unsigned char)(255 * elt.NormalColor[1]),
    (unsigned char)(255 * elt.NormalColor[2]),
    255
  };

  // Get the various sizes and offsets
  int offset = 4 + elt.FontSize;
  int margin = elt.FontSize / 3;
  Vector2ui vp = parentModel->GetSizeReporter()->GetViewportSize();
  int w = vp[0], h = vp[1];

  // Create the font info
  AbstractRendererPlatformSupport::FontInfo font_info =
        { AbstractRendererPlatformSupport::TYPEWRITER, elt.FontSize, true };

  // Use the delegate to draw text
  this->m_PlatformSupport->RenderTextInOpenGL(
        labels[0][0], margin, (h-offset)/2, offset, offset, font_info, -1, 0, elt.NormalColor);

  this->m_PlatformSupport->RenderTextInOpenGL(
        labels[0][1], w - (offset+margin), (h-offset)/2, offset, offset, font_info, 1, 0, elt.NormalColor);

  this->m_PlatformSupport->RenderTextInOpenGL(
        labels[1][0], (w-offset)/2, 0, offset, offset, font_info, 0, -1, elt.NormalColor);

  this->m_PlatformSupport->RenderTextInOpenGL(
        labels[1][1], (w-offset)/2, h - (offset+1), offset, offset, font_info, 0, 1, elt.NormalColor);

  glPopMatrix();
  glPopAttrib();
}

void SliceWindowDecorationRenderer::DrawRulers()
{
  GenericSliceModel *parentModel = this->GetParentRenderer()->GetModel();
  SNAPAppearanceSettings *as =
      parentModel->GetParentUI()->GetAppearanceSettings();

  // Get the properties for the labels
  const SNAPAppearanceSettings::Element &elt =
      as->GetUIElement(SNAPAppearanceSettings::RULER);

  // Leave if the labels are disabled
  if(!elt.Visible) return;

  glPushAttrib(GL_COLOR_BUFFER_BIT | GL_CURRENT_BIT | GL_DEPTH_BUFFER_BIT);
  glPushMatrix();
  glLoadIdentity();

  SNAPAppearanceSettings::ApplyUIElementLineSettings(elt);
  glColor4d( elt.NormalColor[0], elt.NormalColor[1], elt.NormalColor[2], 1.0 );

  // gl_font(FL_HELVETICA, elt.FontSize);

  // Pick the scale of the ruler
  Vector2ui vp = parentModel->GetSizeReporter()->GetViewportSize();

  // The ruler bar should be as large as possible but less than one half
  // of the screen width (not to go over the markers)
  double maxw = 0.5 * vp[0] - 20.0;
  maxw = maxw < 5 ? 5 : maxw;

  double zoom = parentModel->GetViewZoom();
  double scale = 1.0;
  while(zoom * scale > maxw) scale /= 10.0;
  while(zoom * scale < 0.1 * maxw) scale *= 10.0;

  // Draw a zoom bar
  double bw = scale * zoom;
  glBegin(GL_LINES);
  glVertex2d(5,vp[1] - 5);
  glVertex2d(5,vp[1] - 20);
  glVertex2d(5,vp[1] - 10);
  glVertex2d(5 + bw,vp[1] - 10);
  glVertex2d(5 + bw,vp[1] - 5);
  glVertex2d(5 + bw,vp[1] - 20);
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

  unsigned char rgba[] = {
    (unsigned char)(255 * elt.NormalColor[0]),
    (unsigned char)(255 * elt.NormalColor[1]),
    (unsigned char)(255 * elt.NormalColor[2]),
    255
  };

  // Create the font info
  AbstractRendererPlatformSupport::FontInfo font_info =
        { AbstractRendererPlatformSupport::SANS, elt.FontSize, false };

  // See if we can squeeze the label under the ruler
  if(bw > elt.FontSize * 4)
    {
    this->m_PlatformSupport->RenderTextInOpenGL(
        oss.str().c_str(),
        10, vp[1]-32, (int) bw, 20,
        font_info, 0, 1, elt.NormalColor);
    //gl_draw(oss.str().c_str(), 10, h - 30, (int) bw, 20, FL_ALIGN_TOP);
    }
  else
    {
    this->m_PlatformSupport->RenderTextInOpenGL(
          oss.str().c_str(),
          (int) (bw+10), vp[1] - 20, (int) (bw + elt.FontSize * 4+10), 15,
          font_info, -1, 0, elt.NormalColor);
    // gl_draw(oss.str().c_str(), (int) bw+10, h - 20, (int) bw + elt.FontSize * 4+10, 15, FL_ALIGN_LEFT);
    }

  glPopMatrix();
  glPopAttrib();

}
