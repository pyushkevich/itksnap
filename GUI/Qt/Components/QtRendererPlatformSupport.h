#ifndef QTRENDERERPLATFORMSUPPORT_H
#define QTRENDERERPLATFORMSUPPORT_H

#include "AbstractRenderer.h"
#include <QRect>

class QtRendererPlatformSupport : public AbstractRendererPlatformSupport
{
public:

  virtual void RenderTextIntoVTKImage(
      const char *text, vtkImageData *target,
      FontInfo font, int align_horiz, int align_vert,
      const Vector3d &rgbf, double alpha = 1.0);

  virtual int MeasureTextWidth(const char *text, FontInfo font);

protected:
  QRect WorldRectangleToPixelRectangle(double wx, double wy, double ww, double wh);
};

#endif // QTRENDERERPLATFORMSUPPORT_H
