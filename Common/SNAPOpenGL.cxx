#include "SNAPOpenGL.h"
#include <vnl/vnl_math.h>
#include <vector>

void
gl_draw_circle_with_border(double x, double y, double r,
                           double scale_x, double scale_y,
                           Vector3ui colorFill, unsigned int alphaFill,
                           Vector3ui colorBorder,unsigned int alphaBorder,
                           int n_segments)
{
  static std::vector<double> cx, cy;
  if(cx.size() == 0)
    {
    for(double a = 0; a < 2 * vnl_math::pi - 1.0e-6; a += vnl_math::pi / n_segments)
      {
      cx.push_back(cos(a));
      cy.push_back(sin(a));
      }
    }

  glPushMatrix();
  glTranslated(x, y, 0.0);
  glScaled(scale_x, scale_y, 1.0);

  glColor4ub(colorFill(0), colorFill(1), colorFill(2), alphaFill);

  glBegin(GL_TRIANGLE_FAN);
  glVertex2d(0, 0);
  for(size_t i = 0; i < cx.size(); i++)
    glVertex2d(r * cx[i], r * cy[i]);
  glVertex2d(r, 0);
  glEnd();

  glColor4ub(colorBorder(0), colorBorder(1), colorBorder(2), alphaBorder);

  glBegin(GL_LINE_LOOP);
  for(size_t i = 0; i < cx.size(); i++)
    glVertex2d(r * cx[i], r * cy[i]);
  glEnd();

  glPopMatrix();
}




#ifdef __APPLE__
#include <Availability.h>
#endif

#ifdef __MAC_10_8

#include <GLKit/GLKMatrix4.h>
#include <GLKit/GLKMathUtils.h>

void irisOrtho2D(double x, double w, double y, double h)
{
  GLKMatrix4 matrix = GLKMatrix4MakeOrtho(x, w, y, h, -1, 1);
  glLoadMatrixf(matrix.m);
}

void irisUnProject(GLdouble winX, GLdouble winY, GLdouble winZ,
                   const GLdouble * model, const GLdouble * proj, GLint * view,
                   GLdouble* objX, GLdouble* objY, GLdouble* objZ)
{
  GLKMatrix4 m_model, m_proj;
  for(int k = 0; k < 16; k++)
    {
    m_model.m[k] = (float) model[k];
    m_proj.m[k] = (float) proj[k];
    }

  GLKVector3 window, obj;
  window.v[0] = winX;
  window.v[1] = winY;
  window.v[2] = winZ;

  bool success;
  obj = GLKMathUnproject(window, m_model, m_proj, view, &success);

  *objX = obj.v[0];
  *objY = obj.v[1];
  *objZ = obj.v[2];
}

#else

void irisOrtho2D(double x, double w, double y, double h)
{
  gluOrtho2D(x,w,y,h);
}


void irisUnProject(GLdouble winX, GLdouble winY, GLdouble winZ,
                   const GLdouble * model, const GLdouble * proj, GLint * view,
                   GLdouble* objX, GLdouble* objY, GLdouble* objZ)
{
  gluUnProject(winX, winY, winZ, model, proj, view, objX, objY, objZ);
}

#endif

