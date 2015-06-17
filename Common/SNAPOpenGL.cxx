#include "SNAPOpenGL.h"
#include <vnl/vnl_math.h>
#include <vector>

void
gl_draw_circle_with_border(double x, double y, double r,
                           int vp_width, int vp_height,
                           Vector3ui color)
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
  glScaled(1.2 / vp_width, 1.2 / vp_height, 1.0);

  glBegin(GL_TRIANGLE_FAN);
  glVertex2d(0, 0);
  for(size_t i = 0; i < cx.size(); i++)
    glVertex2d(r * cx[i], r * cy[i]);
  glVertex2d(r, 0);
  glEnd();

  glColor3ub(color(0), color(1), color(2));

  glBegin(GL_LINE_LOOP);
  for(size_t i = 0; i < cx.size(); i++)
    glVertex2d(r * cx[i], r * cy[i]);
  glEnd();

  glPopMatrix();
}


#ifdef __APPLE__

#include <Availability.h>

#if __MAC_10_8

#include <GLKit/GLKMatrix4.h>

void irisOrtho2D(double x, double w, double y, double h)
{
  GLKMatrix4 matrix = GLKMatrix4MakeOrtho(x, w, y, h, -1, 1);
  glLoadMatrixf(matrix.m);
}

#else

void irisOrtho2D(double x, double w, double y, double h)
{
  gluOrtho2D(x,w,y,h);
}

#endif // APPLE

#else

void irisOrtho2D(double x, double w, double y, double h)
{
  gluOrtho2D(x,w,y,h);
}

#endif

