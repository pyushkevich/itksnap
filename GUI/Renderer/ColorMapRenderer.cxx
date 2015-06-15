#include "ColorMapRenderer.h"
#include "ColorMapModel.h"
#include "SNAPOpenGL.h"

using namespace std;

ColorMapRenderer::ColorMapRenderer()
{
  m_TextureId = 0xffffffff;
  m_Width = 0;
  m_Height = 0;
}

ColorMapRenderer::~ColorMapRenderer()
{

}

void
ColorMapRenderer
::SetModel(ColorMapModel *model)
{
  this->m_Model = model;
}

void ColorMapRenderer::paintGL()
{
  // Clear the viewport
  glClearColor(1.0, 1.0, 1.0, 1.0);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  // Push the related attributes
  glPushAttrib(GL_LIGHTING_BIT | GL_COLOR_BUFFER_BIT | GL_LINE_BIT | GL_TEXTURE_BIT);

  // Disable lighting
  glEnable(GL_TEXTURE_2D);

  // Generate the texture for the background
  const GLsizei boxw = 16;
  if(m_TextureId == 0xffffffff)
    {
    // Create a checkerboard
    unsigned char *boxarr = new unsigned char[boxw * boxw], *p = boxarr;
    for(GLsizei i = 0; i < boxw; i++)
      for(GLsizei j = 0; j < boxw; j++)
        if((i < boxw / 2 && j < boxw / 2) || (i > boxw / 2 && j > boxw / 2))
          *p++ = 0xef;
        else
          *p++ = 0xff;

    glGenTextures(1, &m_TextureId);
    glBindTexture(GL_TEXTURE_2D, m_TextureId);

    // Properties for the texture
    glTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );

    // Turn off modulo-4 rounding in GL
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, boxw, boxw, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, boxarr);
    delete boxarr;
    }

  // Draw the background pattern
  glBindTexture(GL_TEXTURE_2D, m_TextureId);

  glColor3ub(0xff,0xff,0xff);

  glBegin(GL_QUADS);
  double tx = m_Width * 1.0 / boxw;
  double ty = m_Height * 1.0 / boxw;
  glTexCoord2d(0.0, 0.0); glVertex2d(-0.1, -0.1);
  glTexCoord2d(tx, 0.0); glVertex2d(1.1, -0.1);
  glTexCoord2d(tx, ty); glVertex2d(1.1, 1.1);
  glTexCoord2d(0.0, ty); glVertex2d(-0.1, 1.1);
  glEnd();

  glDisable(GL_LIGHTING);
  glDisable(GL_TEXTURE_2D);


  if(m_Model->GetLayer() != NULL && m_Model->GetColorMap() != NULL)
    {
    // Turn on alpha blending
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);

    // Get the starting and ending color values
    ColorMap *cmap = m_Model->GetColorMap();
    ColorMap::RGBAType v0 = cmap->MapIndexToRGBA(-0.1);
    ColorMap::RGBAType v1 = cmap->MapIndexToRGBA(1.1);

    // Draw the actual texture
    glBegin(GL_QUADS);

    // Draw the color before 0.0
    glColor4ub(v0[0], v0[1], v0[2], 0x00); glVertex2d(-0.1,0.0);
    glColor4ub(v0[0], v0[1], v0[2], 0xff); glVertex2d(-0.1,1.0);

    // Render the colormap
    for(size_t i = 0; i < 512; i++)
      {
      double t = i / 511.0;
      ColorMap::RGBAType rgba = cmap->MapIndexToRGBA(t);

      glColor4ub(rgba[0], rgba[1], rgba[2], 0xff);
      glVertex2d(t, 1.0);
      glColor4ub(rgba[0], rgba[1], rgba[2], 0x00);
      glVertex2d(t, 0.0);
      glColor4ub(rgba[0], rgba[1], rgba[2], 0x00);
      glVertex2d(t, 0.0);
      glColor4ub(rgba[0], rgba[1], rgba[2], 0xff);
      glVertex2d(t, 1.0);
      }

    // Draw the color after 1.0
    glColor4ub(v1[0], v1[1], v1[2], 0xff); glVertex2d(1.1,1.0);
    glColor4ub(v1[0], v1[1], v1[2], 0x00); glVertex2d(1.1,0.0);

    // Done drawing
    glEnd();

    // Draw the gridlines
    glBegin(GL_LINES);
    glColor4ub(0x80, 0x80, 0x80, 0xff);

    // Horizontal gridlines
    glVertex2d(-0.1, 0.0); glVertex2d( 1.1, 0.0);
    glVertex2d(-0.1, 0.5); glVertex2d( 1.1, 0.5);
    glVertex2d(-0.1, 1.0); glVertex2d( 1.1, 1.0);

    // Vertical gridlines
    glVertex2d(0.0, -0.1); glVertex2d(0.0, 1.1);
    glVertex2d(0.25, -0.1); glVertex2d(0.25, 1.1);
    glVertex2d(0.5, -0.1); glVertex2d(0.5, 1.1);
    glVertex2d(0.75, -0.1); glVertex2d(0.75, 1.1);
    glVertex2d(1.0, -0.1); glVertex2d(1.0, 1.1);

    glEnd();

    // Draw the Alpha curve
    glEnable(GL_LINE_SMOOTH);
    glLineWidth(3.0);

    glBegin(GL_LINE_STRIP);
    glColor4ub(0x00,0x00,0x00,0xff);
    glVertex2d(-0.1, v0[3] / 255.0);

    for(size_t i = 0; i < cmap->GetNumberOfCMPoints(); i++)
      {
      ColorMap::CMPoint p = cmap->GetCMPoint(i);
      glVertex2d(p.m_Index, p.m_RGBA[0][3] / 255.0);
      glVertex2d(p.m_Index, p.m_RGBA[1][3] / 255.0);
      }

    glVertex2d(1.1, v1[3] / 255.0);
    glEnd();

    // Define select and non-select colors
    Vector3ui clrBorderSelect(0xff, 0x00, 0xff);
    Vector3ui clrBorderPlain(0x00, 0x00, 0x00);

    // Draw circles around the points
    glDisable(GL_LIGHTING);

    glLineWidth(1.0);
    for(size_t i = 0; i < cmap->GetNumberOfCMPoints(); i++)
      {
      ColorMap::CMPoint p = cmap->GetCMPoint(i);


      glColor3ub(p.m_RGBA[0][0],p.m_RGBA[0][1],p.m_RGBA[0][2]);
      bool select = m_Model->IsControlSelected(i, ColorMapLayerProperties::LEFT);
      gl_draw_circle_with_border(p.m_Index, p.m_RGBA[0][3] / 255.0, 5.0,
                                 m_Width, m_Height,
                                 select ? clrBorderSelect : clrBorderPlain);

      if(p.m_RGBA[0][3] != p.m_RGBA[1][3])
        {
        glColor3ub(p.m_RGBA[1][0],p.m_RGBA[1][1],p.m_RGBA[1][2]);
        bool select = m_Model->IsControlSelected(i, ColorMapLayerProperties::RIGHT);
        gl_draw_circle_with_border(p.m_Index, p.m_RGBA[1][3] / 255.0, 5.0,
                                   m_Width, m_Height,
                                   select ? clrBorderSelect : clrBorderPlain);
        }
      }
    }

  // Pop the attributes
  glPopAttrib();

  // Done
  glFlush();

}

void ColorMapRenderer::resizeGL(int w, int h, int device_pixel_ratio)
{
  // Instead of using the w/h passed in here, which are in physical pixel units,
  // we will use 'logical' pixel units on Retina-type displays. This makes the
  // sizes of all circles and such consistent between Retina and normal displays

  if(!m_Model) return;

  Vector2ui vpl = m_Model->GetViewportReporter()->GetLogicalViewportSize();

  // Set up the basic projection with a small margin
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  irisOrtho2D(-0.1,1.1,-0.1,1.1);
  glViewport(0,0,vpl[0],vpl[1]);

  // Establish the model view matrix
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  // Store the dimensions
  m_Width = vpl[0];
  m_Height = vpl[1];
}
