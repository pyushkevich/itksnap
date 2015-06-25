#include "QtRendererPlatformSupport.h"
#include <QGLWidget>
#include <QPainter>
#include <QPixmap>
#include <QFontMetrics>
#include "SNAPQtCommon.h"
#include <QDebug>
#include <iostream>

QRect
QtRendererPlatformSupport::WorldRectangleToPixelRectangle(double wx, double wy, double ww, double wh)
{
  // Viewport and such
  vnl_vector<int> viewport(4);
  vnl_matrix<double> model_view(4,4), projection(4,4);

  // Map the rectangle to viewport coordinates
  glGetIntegerv(GL_VIEWPORT, viewport.data_block());
  glGetDoublev(GL_PROJECTION_MATRIX, projection.data_block());
  glGetDoublev(GL_MODELVIEW_MATRIX, model_view.data_block());

  vnl_vector<double> xw1(4), xs1, xw2(4), xs2;
  xw1[0] = wx; xw1[1] = wy;
  xw1[2] = 0.0; xw1[3] = 1.0;
  xs1 = projection.transpose() * (model_view.transpose() * xw1);

  xw2[0] = wx + ww; xw2[1] = wy + wh;
  xw2[2] = 0.0; xw2[3] = 1.0;
  xs2 = projection.transpose() * (model_view.transpose() * xw2);

  double x = std::min(xs1[0] / xs1[3], xs2[0] / xs2[3]);
  double y = std::min(xs1[1] / xs1[3], xs2[1] / xs2[3]);
  double w = fabs(xs1[0] / xs1[3] - xs2[0] / xs2[3]);
  double h = fabs(xs1[1] / xs1[3] - xs2[1] / xs2[3]);

  x = (x + 1) / 2;  y = (y + 1) / 2;
  w = w / 2;  h = h / 2;


  return QRect(
      QPoint((int)(x * viewport[2] + viewport[0]), (int)(y * viewport[3] + viewport[1])),
      QSize((int)(w * viewport[2]),(int)(h * viewport[3])));
}


void QtRendererPlatformSupport
::RenderTextInOpenGL(const char *text,
                     double x, double y, double w, double h,
                     FontInfo font,
                     int align_horiz, int align_vert,
                     const Vector3d &rgbf)
{
  // Map the coordinates to screen coordinates
  QRect rScreen = this->WorldRectangleToPixelRectangle(x, y, w, h);

  // Create a pixmap to render the text
  QImage canvas(rScreen.width(), rScreen.height(), QImage::Format_ARGB32);
  canvas.fill(QColor(0,0,0,0));

  // Paint the text onto the canvas
  QPainter painter(&canvas);

  QColor pen_color;
  pen_color.setRgbF(rgbf[0], rgbf[1], rgbf[2], 1.0);
  painter.setPen(pen_color);

  int ah = Qt::AlignHCenter, av = Qt::AlignVCenter;
  switch(align_horiz)
    {
    case AbstractRendererPlatformSupport::LEFT    : ah = Qt::AlignLeft;    break;
    case AbstractRendererPlatformSupport::HCENTER : ah = Qt::AlignHCenter; break;
    case AbstractRendererPlatformSupport::RIGHT   : ah = Qt::AlignRight;   break;
    }
  switch(align_vert)
    {
    case AbstractRendererPlatformSupport::BOTTOM  : av = Qt::AlignBottom;  break;
    case AbstractRendererPlatformSupport::VCENTER : av = Qt::AlignVCenter; break;
    case AbstractRendererPlatformSupport::TOP     : av = Qt::AlignTop;     break;
    }

  QFont qfont;
  switch(font.type)
    {
    case AbstractRendererPlatformSupport::SERIF:
      qfont.setFamily("Times");     break;
    case AbstractRendererPlatformSupport::SANS:
      qfont.setFamily("Helvetica"); break;
    case AbstractRendererPlatformSupport::TYPEWRITER:
      qfont.setFamily("Courier"); break;
    }

  qfont.setPixelSize(font.pixel_size);
  qfont.setBold(font.bold);
  painter.setFont(qfont);

  // painter.fillRect(QRectF(0,0,rScreen.width(),rScreen.height()), QColor(64,64,64,64));
  painter.drawText(QRectF(0,0,rScreen.width(),rScreen.height()), ah | av, QString::fromUtf8(text));

  QImage gl = QGLWidget::convertToGLFormat(canvas);

  glPushAttrib(GL_COLOR_BUFFER_BIT);
  glEnable(GL_BLEND);
  glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
  glRasterPos2i(x,y);
  glDrawPixels(rScreen.width(),rScreen.height(), GL_RGBA, GL_UNSIGNED_BYTE, gl.bits());
  glPopAttrib();
}

int
QtRendererPlatformSupport
::MeasureTextWidth(const char *text, AbstractRendererPlatformSupport::FontInfo font)
{
  QFont qfont;
  switch(font.type)
    {
    case AbstractRendererPlatformSupport::SERIF:
      qfont.setFamily("Times");     break;
    case AbstractRendererPlatformSupport::SANS:
      qfont.setFamily("Helvetica"); break;
    case AbstractRendererPlatformSupport::TYPEWRITER:
      qfont.setFamily("Courier"); break;
    }
  qfont.setPixelSize(font.pixel_size);
  qfont.setBold(font.bold);

  QFontMetrics fm(qfont);
  return fm.width(QString::fromUtf8(text));
}

#include <QOpenGLTexture>

void QtRendererPlatformSupport::LoadTexture(const char *url, GLuint &texture_id, Vector2ui &tex_size)
{
  // Get the icon corresponding to the URL
  QImage myimage(QString(":/root/%1.png").arg(url));

  // Create an opengl texture object
  QOpenGLTexture *texture = new QOpenGLTexture(myimage);
  texture_id = texture->textureId();

  // Set the texture size
  tex_size[0] = texture->width();
  tex_size[1] = texture->height();
}
