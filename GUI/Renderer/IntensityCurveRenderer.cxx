#include "IntensityCurveRenderer.h"
#include "IntensityCurveInterface.h"
#include "IntensityCurveModel.h"
#include "ScalarImageHistogram.h"
#include "ImageWrapperBase.h"

#include "SNAPOpenGL.h"

using namespace std;

const unsigned int IntensityCurveRenderer::CURVE_RESOLUTION = 64;

IntensityCurveRenderer::IntensityCurveRenderer()
{
}

IntensityCurveRenderer::~IntensityCurveRenderer()
{
}


void IntensityCurveRenderer::paintGL(int *bkgColor)
{
  // The curve should have been initialized
  IntensityCurveInterface *curve = m_Model->GetCurve();

  // Get the histogram too
  const ScalarImageHistogram *histogram = m_Model->GetHistogram();

  // Get the viewport dimensions
  GLint viewport[4];
  glGetIntegerv(GL_VIEWPORT, viewport);
  int w = viewport[2]; int h = viewport[3];

  // Clear the viewport
  glClearColor(bkgColor[0] / 255., bkgColor[1] / 255., bkgColor[2] / 255., 1.0);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  // Push the related attributes
  glPushAttrib(GL_LIGHTING_BIT | GL_COLOR_BUFFER_BIT | GL_LINE_BIT);

  // Disable lighting
  glDisable(GL_LIGHTING);

  // Draw the plot area. The intensities outside of the window and level
  // are going to be drawn in darker shade of gray
  float t0, t1, xDummy;
  curve->GetControlPoint(0, t0, xDummy);
  curve->GetControlPoint(curve->GetControlPointCount() - 1, t1, xDummy);

  // Scale the display so that leftmost point to plot maps to 0, rightmost to 1
  float z0 = std::min(t0, 0.0f);
  float z1 = std::max(t1, 1.0f);
  glPushMatrix();
  glTranslated(-z0 / (z1-z0), 0, 0);
  glScaled(1.0 / (z1-z0), 1, 1);

  // Draw the quads
  glBegin(GL_QUADS);

  // Outer quad
  glColor3d(0.9, 0.9, 0.9);
  glVertex2d(z0,0);
  glVertex2d(z0,1);
  glVertex2d(z1,1);
  glVertex2d(z1,0);

  float q0 = std::max(t0, 0.0f);
  float q1 = std::min(t1, 1.0f);
  if(q1 > q0)
    {
    // Inner quad
    glColor3d(1.0, 1.0, 1.0);
    glVertex2d(q0, 0.0);
    glVertex2d(q0, 1.0);
    glVertex2d(q1, 1.0);
    glVertex2d(q1, 0.0);
    }

  glEnd();

  // Draw a histogram if it exists
  unsigned int nBins = histogram->GetSize();

  if(nBins)
    {
    glPushMatrix();

    // Set up a transform, such that the histogram plot area fits into
    // the unit square
    double xspan = histogram->GetBinMax(nBins - 1) - histogram->GetBinMin(0);
    double yspan = histogram->GetMaxFrequency() *
        m_Model->GetProperties().GetHistogramCutoff();

    if(m_Model->GetProperties().IsHistogramLog() && yspan > 0)
      yspan = log10(yspan);

    glScaled(1.0 / xspan, 1.0 / yspan, 1.0);
    glTranslated(-histogram->GetBinMin(0), 0, 0);

    // Figure out how many pixels are actually inside each histogram bin


    // Paint the filled-in histogram bars. We fill in with black color if the
    // histogram width is less than 4 pixels
    if(m_Model->GetProperties().GetHistogramBinSize() < 4)
      glColor3f(0.0f, 0.0f, 0.0f);
    else
      glColor3f(0.8f, 0.8f, 1.0f);

    // Paint the bars as quads
    glBegin(GL_QUADS);
    for(unsigned int i = 0; i < nBins; i++)
      {
      // Compute the physical height of the bin
      double x0 = histogram->GetBinMin(i);
      double x1 = histogram->GetBinMax(i);
      double y = histogram->GetFrequency(i);
      if(m_Model->GetProperties().IsHistogramLog() && y > 0)
        y = log10(y);

      // Paint the bar
      glVertex2f(x0,0);
      glVertex2f(x0,y);
      glVertex2f(x1,y);
      glVertex2f(x1,0);
      }
    glEnd();

    // Draw lines around the quads, but only if the bins are thick
    if(m_Model->GetProperties().GetHistogramBinSize() >= 4)
      {
      // Draw the vertical lines between the histogram bars
      glBegin(GL_LINE_STRIP);
      glColor3d(0.0, 0.0, 0.0);
      for(unsigned int i = 0; i < nBins; i++)
        {
        // Compute the physical height of the bin
        double x0 = histogram->GetBinMin(i);
        double x1 = histogram->GetBinMax(i);
        double y = histogram->GetFrequency(i);
        if(m_Model->GetProperties().IsHistogramLog() && y > 0)
          y = log10(y);

        // Paint the bar
        glVertex2f(x0,0);
        glVertex2f(x0,y);
        glVertex2f(x1,y);
        glVertex2f(x1,0);
        }
      glEnd();
      }

    glPopMatrix();
    }

  // Draw the box around the plot area
  glColor3d(0,0,0);
  glBegin(GL_LINE_LOOP);
  glVertex2d(z0,0);
  glVertex2d(z0,1);
  glVertex2d(z1,1);
  glVertex2d(z1,0);
  glEnd();

  // Set up the smooth line drawing style
  glEnable(GL_LINE_SMOOTH);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glLineWidth(2.0);

  // Draw the curve using linear segments
  glColor3d(1.0,0.0,0.0);
  glBegin(GL_LINE_STRIP);

  float t = z0;
  float tStep = (z1-z0) / (CURVE_RESOLUTION);
  for (unsigned int i=0;i<=CURVE_RESOLUTION;i++)
    {
    glVertex2f(t,curve->Evaluate(t));
    t+=tStep;
    }

  glEnd();

  // Draw the handles
  glLineWidth(1.0);
  for (unsigned int c=0;c<curve->GetControlPointCount();c++)
    {
    // Get the next control point
    float t,x;
    curve->GetControlPoint(c,t,x);

    // Draw a quad around the control point
    if(c == (unsigned int) m_Model->GetProperties().GetMovingControlPoint())
      {
      glColor3d(1,1,0);
      DrawCircleWithBorder(t, x, 5.0, z1-z0, false, w, h);
      }
    else
      {
      glColor3d(1,0,0);
      DrawCircleWithBorder(t, x, 4.0, z1-z0, false, w, h);
      }
    }

  glPopMatrix();

  // Pop the attributes
  glPopAttrib();

  // Done
  glFlush();
}

void
IntensityCurveRenderer::
DrawCircleWithBorder(double x, double y, double r, double bw,
                     bool select, int w, int h)
{
  static std::vector<double> cx, cy;
  if(cx.size() == 0)
    {
    for(double a = 0; a < 2 * vnl_math::pi - 1.0e-6; a += vnl_math::pi / 20)
      {
      cx.push_back(cos(a));
      cy.push_back(sin(a));
      }
    }

  glPushMatrix();
  glTranslated(x, y, 0.0);
  glScaled(1.2 * bw / w, 1.2 / h, 1.0);

  glBegin(GL_TRIANGLE_FAN);
  glVertex2d(0, 0);
  for(size_t i = 0; i < cx.size(); i++)
    glVertex2d(r * cx[i], r * cy[i]);
  glVertex2d(r, 0);
  glEnd();

  if(select)
    glColor3ub(0xff,0x00,0xff);
  else
    glColor3ub(0x00,0x00,0x00);

  glBegin(GL_LINE_LOOP);
  for(size_t i = 0; i < cx.size(); i++)
    glVertex2d(r * cx[i], r * cy[i]);
  glEnd();

  glPopMatrix();
}



void IntensityCurveRenderer::resizeGL(int w, int h)
{
  // Set up the basic projection
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluOrtho2D(-0.025,1.025,-0.05,1.05);
  glViewport(0,0,w,h);

  // Establish the model view matrix
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
}

void IntensityCurveRenderer::initializeGL()
{
}

void IntensityCurveRenderer::SetModel(IntensityCurveModel *model)
{
  m_Model = model;
}
