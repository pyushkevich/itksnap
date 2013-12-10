#include "QtRendererPlatformSupport.h"
#include <QGLWidget>
#include <QPainter>
#include <QPixmap>
#include "SNAPQtCommon.h"

void QtRendererPlatformSupport
::RenderTextInOpenGL(const char *text,
                     int x, int y, int w, int h,
                     FontInfo font,
                     int align_horiz, int align_vert,
                     const Vector3d &rgbf)
{
  // Create a pixmap to render the text
  QImage canvas(w, h, QImage::Format_ARGB32);
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

  painter.drawText(QRectF(0,0,w,h), ah | av, QString::fromUtf8(text));

  QImage gl = QGLWidget::convertToGLFormat(canvas);

  glPushAttrib(GL_COLOR_BUFFER_BIT);
  glEnable(GL_BLEND);
  glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
  glRasterPos2i(x, y);
  glDrawPixels(w, h, GL_RGBA, GL_UNSIGNED_BYTE, gl.bits());
  glPopAttrib();
}
