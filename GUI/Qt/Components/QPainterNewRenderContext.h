#ifndef QPAINTERNEWRENDERCONTEXT_H
#define QPAINTERNEWRENDERCONTEXT_H

#include "AbstractNewRenderer.h"
#include "SNAPCommon.h"
#include <QImage>
#include <QPainterPath>
#include <QPainter>
#include <QDebug>

class QPainterNewRenderContextTexture : public AbstractNewRenderContext::Texture
{
public:
  irisITKObjectMacro(QPainterNewRenderContextTexture, AbstractNewRenderContext::Texture)

protected:
  QPixmap qpixmap;
  QBrush qbrush;

  QPainterNewRenderContextTexture() {}
  virtual ~QPainterNewRenderContextTexture() override {}
  friend class QPainterNewRenderContext;
};

class QPainterNewRenderContextPath2D : public AbstractNewRenderContext::Path2D
{
public:
  irisITKObjectMacro(QPainterNewRenderContextPath2D, AbstractNewRenderContext::Texture)

protected:
  QPainterPath *path = nullptr;

  QPainterNewRenderContextPath2D() {}
  virtual ~QPainterNewRenderContextPath2D() override { if(path) delete(path); }
  friend class QPainterNewRenderContext;
};

// Template class that takes a member function pointer as a template parameter
template <typename ValueType, ValueType (QPainter::*Getter)() const, void (QPainter::*Setter)(ValueType)>
class QPainterSettingStateRestorer {
public:
  QPainterSettingStateRestorer(QPainter &painter, const ValueType &value) : m_Painter(painter)
  {
    m_StoredState = (m_Painter.*Getter)();
    (m_Painter.*Setter)(value);
  }
  ~QPainterSettingStateRestorer()
  {
    (m_Painter.*Setter)(m_StoredState);
  }protected:
  QPainter &m_Painter;
  ValueType m_StoredState;
};

class QPainterRenderHintsRestorer
{
public:
  QPainterRenderHintsRestorer(QPainter &painter)
    : m_Painter(painter)
  {
    m_StoredState = m_Painter.renderHints();
  }

  QPainterRenderHintsRestorer(QPainter            &painter,
                              QPainter::RenderHint hint,
                              bool                 on = true)
    : m_Painter(painter)
  {
    m_StoredState = m_Painter.renderHints();
    setRenderHint(hint, on);
  }

  void setRenderHint(QPainter::RenderHint hint, bool on = true)
  {
    m_Painter.setRenderHint(hint, on);
  }

  ~QPainterRenderHintsRestorer() { m_Painter.setRenderHints(m_StoredState); }

protected:
  QPainter             &m_Painter;
  QPainter::RenderHints m_StoredState;
};

/** Sets opacity value in QPainter and restores when out of scope */
using QPainterOpacityRestorer =
  QPainterSettingStateRestorer<double, &QPainter::opacity, &QPainter::setOpacity>;

/** Sets composition mode in QPainter and restores when out of scope */
using QPainterCompositionModeRestorer =
  QPainterSettingStateRestorer<QPainter::CompositionMode,
                               &QPainter::compositionMode,
                               &QPainter::setCompositionMode>;


class QPainterNewRenderContext : public AbstractNewRenderContext
{
public:
  using RGBAPixel = AbstractNewRenderContext::RGBAPixel;
  using RGBAImage = AbstractNewRenderContext::RGBAImage;
  using Texture = AbstractNewRenderContext::Texture;
  using TexturePtr = AbstractNewRenderContext::TexturePtr;
  using Path2D = AbstractNewRenderContext::Path2D;
  using Path2DPtr = AbstractNewRenderContext::Path2DPtr;

  QPainterNewRenderContext(QPainter &painter) : painter(painter) {}

  virtual Path2DPtr CreateClosedPolygonPath(std::vector<Vector2d> &path) override
  {
    // Create the object that will store the path data
    auto wrapper = QPainterNewRenderContextPath2D::New();

    // Create the path from the points supplied
    wrapper->path = new QPainterPath();
    if (path.size())
      wrapper->path->moveTo(path.front()[0], path.front()[1]);
    for (auto &v : path)
    {
      if (wrapper->path->isEmpty())
        wrapper->path->moveTo(v[0], v[1]);
      else
        wrapper->path->lineTo(v[0], v[1]);
    }
    wrapper->Modified();

    // Downcast to generic path2d pointer
    return Path2DPtr(wrapper.GetPointer());
  }

  virtual TexturePtr CreateTexture(RGBAImage *image) override
  {
    // Create the object that will store the texture data
    auto texture = QPainterNewRenderContextTexture::New();

    // Load the texture from ITK
    auto   size = image->GetBufferedRegion().GetSize();
    QImage q_image((unsigned char *)image->GetBufferPointer(),
                   size[0],
                   size[1],
                   QImage::Format_RGBA8888);

    // Create a pixmap of the texture
    texture->qpixmap = QPixmap::fromImage(
      q_image.convertToFormat(QImage::Format_ARGB32_Premultiplied));

    // Mark it as modified
    texture->Modified();

    // Downcast to generic texture pointer
    return TexturePtr(texture.GetPointer());
  }

  virtual TexturePtr ColorizeTexture(Texture *texture, const Vector3d &color) override
  {
    // Get the original texture
    auto *q_texture = static_cast<QPainterNewRenderContextTexture *>(texture);

    QPixmap result(q_texture->qpixmap.size());
    result.fill(Qt::transparent); // Fill with transparency initially
    QPainter rpainter(&result);
    rpainter.setCompositionMode(QPainter::CompositionMode_Source);
    rpainter.drawPixmap(0, 0, q_texture->qpixmap); // Draw the original pixmap

    rpainter.setCompositionMode(QPainter::CompositionMode_Multiply);
    rpainter.fillRect(result.rect(), QColor::fromRgbF(color[0], color[1], color[2]));
    rpainter.end(); // End painting

    auto new_texture = QPainterNewRenderContextTexture::New();
    new_texture->qpixmap = result;
    new_texture->Modified();
    return TexturePtr(new_texture.GetPointer());
  }

  virtual void DrawPath(void *path) override
  {
    QPainterPath *p = static_cast<QPainterPath *>(path);
    painter.drawPath(*p);
  }

  virtual void DrawLine(double x0, double y0, double x1, double y1) override
  {
    painter.drawLine(QLineF(x0, y0, x1, y1));
  }

  virtual void DrawRect(double x, double y, double w, double h) override
  {
    painter.drawRect(QRectF(x, y, w, h));
  }

  virtual void FillRect(double x, double y, double w, double h) override
  {
    painter.fillRect(QRectF(x, y, w, h), painter.brush());
  }

  virtual void SetPen(const Vector3d &rgb) override
  {
    painter.setPen(QColor::fromRgbF(rgb[0], rgb[1], rgb[2]));
  }

  virtual void SetPen(const OpenGLAppearanceElement &as) override
  {
    // TODO: check deviceratio for width
    QPen pen;
    auto c = as.GetColor();
    pen.setColor(QColor::fromRgbF(c[0], c[1], c[2], as.GetAlpha()));
    pen.setCosmetic(true);
    pen.setWidthF(as.GetLineThickness());
    switch(as.GetLineType())
    {
      case vtkPen::NO_PEN:
        pen.setStyle(Qt::NoPen);
        break;
      case vtkPen::SOLID_LINE:
        pen.setStyle(Qt::SolidLine);
        break;
      case vtkPen::DASH_LINE:
        pen.setStyle(Qt::DashLine);
        break;
      case vtkPen::DOT_LINE:
        pen.setStyle(Qt::DotLine);
        break;
      case vtkPen::DASH_DOT_LINE:
        pen.setStyle(Qt::DashDotLine);
        break;
      case vtkPen::DASH_DOT_DOT_LINE:
        pen.setStyle(Qt::DashDotDotLine);
        break;
      default:
        pen.setStyle(Qt::SolidLine);
        break;
    }

    painter.setPen(pen);
  }

  virtual void SetBrush(const Vector3d &rgb, double alpha = 1.0) override
  {
    QBrush brush(Qt::SolidPattern);
    brush.setColor(QColor::fromRgbF(rgb[0], rgb[1], rgb[2], alpha));
    painter.setBrush(brush);
  }

  using CompositionMode = AbstractNewRenderContext::CompositionMode;

  virtual void SetCompositionMode(CompositionMode mode) override
  {
    switch(mode)
    {
      case AbstractNewRenderContext::SOURCE_OVER:
        painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
        break;
      case AbstractNewRenderContext::DESTINATION_OVER:
        painter.setCompositionMode(QPainter::CompositionMode_DestinationOver);
        break;
      case AbstractNewRenderContext::MULTIPLY:
        painter.setCompositionMode(QPainter::CompositionMode_Xor);
        break;
      case AbstractNewRenderContext::UNKNOWN:
        break;
    }
  }

  virtual CompositionMode GetCompositionMode() override
  {
    switch(painter.compositionMode())
    {
      case QPainter::CompositionMode_SourceOver:
        return AbstractNewRenderContext::SOURCE_OVER;
      case QPainter::CompositionMode_DestinationOver:
        return AbstractNewRenderContext::DESTINATION_OVER;
      case QPainter::CompositionMode_Multiply:
        return AbstractNewRenderContext::MULTIPLY;
      default:
        return AbstractNewRenderContext::UNKNOWN;
    }
  }

  virtual void FillViewport(const Vector3d &rgb) override
  {
    painter.save();
    painter.setWorldTransform(QTransform());
    painter.fillRect(painter.window(), QColor::fromRgbF(rgb[0], rgb[1], rgb[2]));
    painter.restore();
  }

  virtual void DrawText(const std::string &str) override
  {
    painter.setFont(QFont("Arial", 30));
    // QRect rect(0, 0, painter.device()->width(), painter.device()->height());
    // painter.drawText(rect, Qt::AlignCenter, str.c_str());
    painter.drawText(QPointF(0, 0), str.c_str());
  }

  virtual QRect Mirror(const QRect &r)
  {
    int h = painter.device()->height();
    return QRect(r.x(), h-(r.y() + r.height()), r.width(), r.height());
  }

  virtual void DrawImage(double   x,
                         double   y,
                         double   width,
                         double   height,
                         Texture *texture,
                         bool     bilinear,
                         double   opacity) override
  {
    auto *q_texture = static_cast<QPainterNewRenderContextTexture *>(texture);
    QPainterOpacityRestorer op(painter, opacity);
    QPainterCompositionModeRestorer comp(painter, QPainter::CompositionMode_SourceOver);
    QPainterRenderHintsRestorer hints(painter, QPainter::SmoothPixmapTransform, bilinear);

    QRectF source(0., 0., q_texture->qpixmap.width(), q_texture->qpixmap.height());
    QRectF target(x, y, width, height);

    painter.drawPixmap(target, q_texture->qpixmap, source);
  }

  virtual void DrawImageColorized(double   x,
                                  double   y,
                                  double   width,
                                  double   height,
                                  Texture *texture,
                                  bool     bilinear,
                                  double   opacity,
                                  Vector3d &color) override
  {
    auto *q_texture = static_cast<QPainterNewRenderContextTexture *>(texture);
    this->DrawImage(x,y,width,height,texture,bilinear,opacity);
    QRectF target(x, y, width, height);
    QPainterCompositionModeRestorer comp(painter, QPainter::CompositionMode_Multiply);
    painter.fillRect(target, QColor::fromRgbF(color[0], color[1], color[2]));
  }

  virtual void DrawImageRegion(double   x,
                               double   y,
                               double   width,
                               double   height,
                               double   src_x,
                               double   src_y,
                               double   src_width,
                               double   src_height,
                               Texture *texture,
                               bool     bilinear,
                               double   opacity) override
  {
    auto *q_texture = static_cast<QPainterNewRenderContextTexture *>(texture);
    QPainterOpacityRestorer op(painter, opacity);
    QPainterCompositionModeRestorer comp(painter, QPainter::CompositionMode_SourceOver);
    QPainterRenderHintsRestorer hints(painter, QPainter::SmoothPixmapTransform, bilinear);

    QRectF source(src_x, src_y, src_width, src_height);
    QRectF target(x, y, width, height);
    painter.drawPixmap(target, q_texture->qpixmap, source);
  }

  virtual void SetLogicalWindow(int x, int y, int width, int height) override
  {
    painter.setWindow(x, y, width, height);
    painter.setClipRect(QRect(x, y, width, height));
    painter.setWorldTransform(QTransform(1, 0, 0, -1, 0, 2 * y + height), false);
  }

  virtual void SetViewport(int x, int y, int width, int height) override
  {
    // painter.setViewport(x, y, width, height);
    auto dev_h = painter.device()->height();

    painter.setViewport(x, dev_h - y - height, width, height);
  }

  virtual void LoadIdentity() override
  {
    auto win = painter.window();
    painter.setWorldTransform(QTransform(1, 0, 0, -1, 0, 2 * win.y() + win.height()), false);
  }

  virtual void PushMatrix() override
  {
    painter.save();
  }

  virtual void PopMatrix() override
  {
    painter.restore();
  }

  virtual void Scale(double sx, double sy) override
  {
    painter.scale(sx, sy);
  }

  virtual void Translate(double tx, double ty) override
  {
    painter.translate(tx, ty);
  }

protected:
  QPainter &painter;
};



#endif // QPAINTERNEWRENDERCONTEXT_H
