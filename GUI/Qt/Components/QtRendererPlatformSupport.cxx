#include "QtRendererPlatformSupport.h"
#include <vtkImageData.h>
#include <QPainter>
#include <QPixmap>
#include <QFontMetrics>
#include "SNAPQtCommon.h"
#include <QDebug>
#include <iostream>

void QtRendererPlatformSupport
::RenderTextIntoVTKImage(const char *text,
                         vtkImageData *target,
                         FontInfo font,
                         int align_horiz, int align_vert,
                         const Vector3d &rgbf, double alpha)
{
  int *dim = target->GetDimensions();
  QImage canvas((unsigned char *) target->GetScalarPointer(),
                dim[0], dim[1], dim[0] * 4, QImage::Format_RGBA8888);
  canvas.fill(QColor(0,0,0,0));

  // Paint the text onto the canvas
  QPainter painter(&canvas);

  QColor pen_color;
  pen_color.setRgbF(rgbf[0], rgbf[1], rgbf[2], 1.0);
  pen_color.setAlphaF(alpha);
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

  // For some reason text is getting flipped
  QTransform tran(1, 0, 0, -1, 0, dim[1]);
  painter.setTransform(tran);
  painter.drawText(QRectF(0,0,dim[0],dim[1]), ah | av, QString::fromUtf8(text));
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
  return fm.horizontalAdvance(QString::fromUtf8(text));
}
