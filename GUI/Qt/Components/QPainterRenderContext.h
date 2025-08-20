#ifndef QPAINTERNEWRENDERCONTEXT_H
#define QPAINTERNEWRENDERCONTEXT_H

#include "AbstractContextBasedRenderer.h"
#include "SNAPCommon.h"
#include <QImage>
#include <QPainterPath>
#include <QPainter>
#include <QDebug>
#include <QFontDatabase>
#include <QOpenGLBuffer>
#include <QOpenGLFunctions>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLShaderProgram>
#include <QOpenGLPaintDevice>
#include <vtkCellArrayIterator.h>
#include <vtkPolyData.h>
#include <vtkScalarsToColors.h>

class QPainterRenderContextTexture : public AbstractRenderContext::Texture
{
public:
  irisITKObjectMacro(QPainterRenderContextTexture, AbstractRenderContext::Texture)

protected:
  QPixmap qpixmap;
  QBrush qbrush;

  QPainterRenderContextTexture() {}
  virtual ~QPainterRenderContextTexture() override {}
  friend class QPainterRenderContext;
};

class QPainterRenderContextPath2D : public AbstractRenderContext::Path2D
{
public:
  irisITKObjectMacro(QPainterRenderContextPath2D, AbstractRenderContext::Texture)

protected:
  QPainterPath *path = nullptr;
  QList<QPolygonF> subpath_polygons;

  QPainterRenderContextPath2D() {}
  virtual ~QPainterRenderContextPath2D() override
  {
    if(path)
      delete path;
  }
  friend class QPainterRenderContext;
};

class QPainterRenderContextContourSet2D : public AbstractRenderContext::ContourSet2D
{
public:
  irisITKObjectMacro(QPainterRenderContextContourSet2D, AbstractRenderContext::Texture)

protected:
  QOpenGLBuffer *m_vbo = nullptr;
  QOpenGLBuffer *m_rgba_vbo = nullptr;
  QOpenGLVertexArrayObject *m_vao = nullptr;
  QOpenGLShaderProgram *m_program = nullptr;
  QList<GLfloat> m_vertex_coords;
  QList<GLfloat> m_vertex_colors;
  QList<int> m_strip_sizes;
  bool use_gl;

  QPainterRenderContextContourSet2D() {}
  virtual ~QPainterRenderContextContourSet2D() override
  {
    if(m_vbo)
      delete m_vbo;
    if(m_rgba_vbo)
      delete m_rgba_vbo;
    if(m_vao)
      delete m_vao;
    if(m_program)
      delete m_program;
  }
  friend class QPainterRenderContext;
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




class QPainterRenderContext : public AbstractRenderContext
{
public:
  using RGBAPixel = AbstractRenderContext::RGBAPixel;
  using RGBAImage = AbstractRenderContext::RGBAImage;
  using Texture = AbstractRenderContext::Texture;
  using TexturePtr = AbstractRenderContext::TexturePtr;
  using Path2D = AbstractRenderContext::Path2D;
  using Path2DPtr = AbstractRenderContext::Path2DPtr;
  using ContourSet2D = AbstractRenderContext::ContourSet2D;
  using ContourSet2DPtr = AbstractRenderContext::ContourSet2DPtr;
  using VertexVector = AbstractRenderContext::VertexVector;

  static constexpr char vertexShaderSourceCore[] =
    "#version 150\n"
    "in vec4 vertex;\n"
    "out vec3 vert;\n"
    "in vec4 vertexColor;\n"
    "out vec4 vertColor;\n"
    "uniform mat4 projMatrix;\n"
    "uniform mat4 mvMatrix;\n"
    "void main() {\n"
    "   vert = vertex.xyz;\n"
    "   vertColor = vertexColor;\n"
    "   gl_Position = projMatrix * mvMatrix * vertex;\n"
    "}\n";

  static constexpr char fragmentShaderSourceCore[] =
    "#version 150\n"
    "in vec2 texCoord;\n"
    "in float edgeAlpha;\n"
    "out highp vec4 fragColor;\n"
    "in vec4 gColor;\n"
    "void main() {\n"
    "    float dist = abs(texCoord.x);\n"
    "    float alpha = 1.0 - smoothstep(0.8, 1.0, dist);\n"
    "    fragColor = vec4(gColor.rgb, gColor.a * alpha * edgeAlpha);\n"
    "}\n";

  static constexpr char geometryShaderSourceCore[] =
    "#version 150\n"
    "layout(lines) in;\n"
    "layout(triangle_strip, max_vertices = 4) out;\n"
    "in vec4 vertColor[2]\n;"
    "out vec4 gColor\n;"
    "uniform float lineWidth; // In pixels\n"
    "uniform vec2 viewport_size;\n"
    "uniform float overallAlpha;\n"
    "out vec2 texCoord;\n"
    "out float edgeAlpha;\n"
    "void main() {\n"
    "    float u_width = viewport_size[0];\n"
    "    float u_height = viewport_size[1];\n"
    "    vec2 p0 = gl_in[0].gl_Position.xy;\n"
    "    vec2 p1 = gl_in[1].gl_Position.xy;\n"
    "    vec2 dir = normalize(p1 - p0);\n"
    "    vec2 perp = vec2(-dir.y, dir.x); // Perpendicular vector\n"
    "    float halfWidth = max(lineWidth, 0.5) / max(u_width, u_height);\n"
    "    vec2 offset = perp * halfWidth;\n"
    "    vec4 v0 = vec4(p0 - offset, 0.0, 1.0);\n"
    "    vec4 v1 = vec4(p0 + offset, 0.0, 1.0);\n"
    "    vec4 v2 = vec4(p1 - offset, 0.0, 1.0);\n"
    "    vec4 v3 = vec4(p1 + offset, 0.0, 1.0);\n"
    "    texCoord = vec2(-1.0, 0.0); edgeAlpha = overallAlpha;\n"
    "    gl_Position = v0; gColor = vertColor[0]; EmitVertex();\n"
    "    texCoord = vec2(1.0, 0.0); edgeAlpha = overallAlpha;\n"
    "    gl_Position = v1; gColor = vertColor[0]; EmitVertex();\n"
    "    texCoord = vec2(-1.0, 1.0); edgeAlpha = overallAlpha;\n"
    "    gl_Position = v2; gColor = vertColor[1]; EmitVertex();\n"
    "    texCoord = vec2(1.0, 1.0); edgeAlpha = overallAlpha;\n"
    "    gl_Position = v3; gColor = vertColor[1]; EmitVertex();\n"
    "    EndPrimitive();\n"
    "}\n";

  QPainterRenderContext(QPainter &painter, QOpenGLFunctions &glfunc) : painter(painter), glfunc(glfunc) {}

  virtual Path2DPtr CreatePath() override
  {
    // Create the object that will store the path data
    auto wrapper = QPainterRenderContextPath2D::New();
    wrapper->path = new QPainterPath();
    wrapper->Modified();
    return Path2DPtr(wrapper.GetPointer());
  }

  virtual void AddPolygonSegmentToPath(Path2D *path, const VertexVector &segment, bool closed) override
  {
    // Get the QPainterPath from the passed in pointer
    auto *wrapper = static_cast<QPainterRenderContextPath2D *>(path);
    auto *p = wrapper->path;

    // Add the segment
    QPolygonF polygon;
    for(const auto &pt : segment)
      polygon << QPointF(pt[0], pt[1]);

    if(closed && segment.size() > 2)
      polygon << QPointF(segment.front()[0], segment.front()[1]);

    p->addPolygon(polygon);
    wrapper->Modified();
  }

  virtual void BuildPath(Path2D *path) override
  {
    auto *wrapper = static_cast<QPainterRenderContextPath2D *>(path);
    wrapper->subpath_polygons = wrapper->path->toSubpathPolygons();
  }

  virtual TexturePtr CreateTexture(RGBAImage *image) override
  {
    // Create the object that will store the texture data
    auto texture = QPainterRenderContextTexture::New();

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
    auto *q_texture = static_cast<QPainterRenderContextTexture *>(texture);

    QPixmap result(q_texture->qpixmap.size());
    result.fill(Qt::transparent); // Fill with transparency initially
    QPainter rpainter(&result);
    rpainter.setCompositionMode(QPainter::CompositionMode_Source);
    rpainter.drawPixmap(0, 0, q_texture->qpixmap); // Draw the original pixmap

    rpainter.setCompositionMode(QPainter::CompositionMode_Multiply);
    rpainter.fillRect(result.rect(), QColor::fromRgbF(color[0], color[1], color[2]));
    rpainter.end(); // End painting

    auto new_texture = QPainterRenderContextTexture::New();
    new_texture->qpixmap = result;
    new_texture->Modified();
    return TexturePtr(new_texture.GetPointer());
  }

  virtual void DrawPath(Path2D *path) override
  {
    auto *wrapper = static_cast<QPainterRenderContextPath2D *>(path);
    for(const auto &poly : std::as_const(wrapper->subpath_polygons))
      painter.drawPolyline(poly);
  }

  virtual ContourSet2DPtr CreateContourSet() override
  {
    // Create the object that will store the path data
    auto wrapper = QPainterRenderContextContourSet2D::New();

    // Check if the device is a compatible OpenGL device
    wrapper->use_gl = false;
    auto *device = dynamic_cast<QOpenGLPaintDevice *>(this->painter.device());
    if(device && device->context() && device->context() == QOpenGLContext::currentContext())
    {
      auto fmt = device->context()->format();
      if(fmt.majorVersion() >= 3)
        wrapper->use_gl = true;
      else if (fmt.majorVersion() == 2 && fmt.minorVersion() >= 1 &&
               glfunc.hasOpenGLFeature(QOpenGLFunctions::Buffers) &&
               glfunc.hasOpenGLFeature(QOpenGLFunctions::Shaders))
        wrapper->use_gl = true;
    }

    // Depending on gl, we create a path or a vertex buffer object
    if(wrapper->use_gl)
    {
      // Create a shader program
      wrapper->m_program = new QOpenGLShaderProgram;
      wrapper->m_program->addShaderFromSourceCode(QOpenGLShader::Vertex, vertexShaderSourceCore);
      wrapper->m_program->addShaderFromSourceCode(QOpenGLShader::Geometry, geometryShaderSourceCore);
      wrapper->m_program->addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentShaderSourceCore);
      wrapper->m_program->bindAttributeLocation("vertex", 0);
      wrapper->m_program->bindAttributeLocation("vertexColor", 1);
      wrapper->m_program->link();

      // Create a vertex array object and a buffer
      wrapper->m_vao = new QOpenGLVertexArrayObject();
      wrapper->m_vao->create();
      QOpenGLVertexArrayObject::Binder vao_binder(wrapper->m_vao);
      wrapper->m_vbo = new QOpenGLBuffer();
      wrapper->m_vbo->create();
      wrapper->m_vbo->bind();
      glfunc.glEnableVertexAttribArray(0);
      glfunc.glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), nullptr);
      wrapper->m_vbo->release();

      // Add a color buffer
      wrapper->m_rgba_vbo = new QOpenGLBuffer();
      wrapper->m_rgba_vbo->create();
      wrapper->m_rgba_vbo->bind();
      glfunc.glEnableVertexAttribArray(1);
      glfunc.glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 0, nullptr);
      wrapper->m_rgba_vbo->release();

      wrapper->m_program->release();
    }

    wrapper->Modified();
    return ContourSet2DPtr(wrapper.GetPointer());
  }

#ifdef OLDCODE
  virtual void AddContoursToSet(ContourSet2D *cset, vtkPolyData *pd) override
  {
    auto *wrapper = dynamic_cast<QPainterRenderContextContourSet2D *>(cset);

    vtkSmartPointer<vtkPoints>    points = pd->GetPoints();
    vtkSmartPointer<vtkCellArray> lines = pd->GetLines();
    if (points && lines)
    {
      vtkIdType        npts;
      const vtkIdType *pts;
      double p[3];

      lines->InitTraversal();
      while (lines->GetNextCell(npts, pts))
      {
        if(wrapper->use_gl)
        {
          /*
          for (vtkIdType i = 0; i < npts; ++i)
          {
            points->GetPoint(pts[i], p);
            wrapper->m_vertex_coords << (GLfloat)(p[0]) << (GLfloat)(p[1]);
          }
          wrapper->m_strip_sizes << npts;
          */
          for (vtkIdType i = 0; i < npts-1; ++i)
          {
            points->GetPoint(pts[i], p);
            wrapper->m_vertex_coords << (GLfloat)(p[0]) << (GLfloat)(p[1]);
            points->GetPoint(pts[i+1], p);
            wrapper->m_vertex_coords << (GLfloat)(p[0]) << (GLfloat)(p[1]);
          }
          wrapper->m_strip_sizes << 2 * (npts-1);

        }
        else
        {
          AbstractRenderContext::VertexVector edgeVertices;
          for (vtkIdType i = 0; i < npts; ++i)
          {
            points->GetPoint(pts[i], p);
            edgeVertices.emplace_back(Vector2d(p[0], p[1]));
          }
          AddPolygonSegmentToPath(wrapper->m_path, edgeVertices, false);
        }
      }
    }
  }
#endif //OLDCODE

  virtual void AddContoursToSet(ContourSet2D                            *cset,
                                vtkPolyData                             *pd,
                                AbstractRenderContext::PolyDataColorMode mode = AbstractRenderContext::SOLID_COLOR,
                                vtkUnsignedCharArray                    *rgbvec = nullptr) override
  {
    auto                      *wrapper = dynamic_cast<QPainterRenderContextContourSet2D *>(cset);
    auto                       solid_color = painter.pen().color();
    vtkSmartPointer<vtkPoints> points = pd->GetPoints();
    vtkSmartPointer<vtkCellArray> lines = pd->GetLines();
    if (points && lines)
    {
      vtkIdType        npts;
      const vtkIdType *pts;
      double           p[3], rgba[4];

      // Pre-reserve the space in vertex arrays
      unsigned int n_strips = 0;
      for (int i = 0; i < lines->GetNumberOfCells(); i++)
        n_strips += (lines->GetCellSize(i) - 1);
      wrapper->m_vertex_coords.reserve(wrapper->m_vertex_coords.size() + n_strips * 4);
      wrapper->m_vertex_colors.reserve(wrapper->m_vertex_colors.size() + n_strips * 8);
      wrapper->m_strip_sizes.reserve(wrapper->m_strip_sizes.size() + lines->GetNumberOfCells());

      // Now iterate and add the strips and colors to the arrays
      auto iter = vtk::TakeSmartPointer(lines->NewIterator());
      for (iter->GoToFirstCell(); !iter->IsDoneWithTraversal(); iter->GoToNextCell())
      {
        iter->GetCurrentCell(npts, pts);
        for (vtkIdType i = 0; i < npts - 1; ++i)
        {
          // Add the two endpoitns of the line
          points->GetPoint(pts[i], p);
          wrapper->m_vertex_coords << (GLfloat)(p[0]) << (GLfloat)(p[1]);
          points->GetPoint(pts[i + 1], p);
          wrapper->m_vertex_coords << (GLfloat)(p[0]) << (GLfloat)(p[1]);

          // Set the color
          if (mode == AbstractRenderContext::POINT_DATA && rgbvec)
          {
            rgbvec->GetTuple(pts[i], rgba);
            for (unsigned int j = 0; j < 4; j++)
              wrapper->m_vertex_colors << (GLfloat)(rgba[j] / 255.);
            rgbvec->GetTuple(pts[i + 1], rgba);
            for (unsigned int j = 0; j < 4; j++)
              wrapper->m_vertex_colors << (GLfloat)(rgba[j] / 255.);
          }
          else if (mode == AbstractRenderContext::CELL_DATA && rgbvec)
          {
            rgbvec->GetTuple(iter->GetCurrentCellId(), rgba);
            for (unsigned int j = 0; j < 4; j++)
              wrapper->m_vertex_colors << (GLfloat)(rgba[j] / 255.);
            for (unsigned int j = 0; j < 4; j++)
              wrapper->m_vertex_colors << (GLfloat)(rgba[j] / 255.);
          }
          else
          {
            wrapper->m_vertex_colors
              << (GLfloat)(solid_color.redF()) << (GLfloat)(solid_color.greenF())
              << (GLfloat)(solid_color.blueF()) << (GLfloat)(1.);
            wrapper->m_vertex_colors
              << (GLfloat)(solid_color.redF()) << (GLfloat)(solid_color.greenF())
              << (GLfloat)(solid_color.blueF()) << (GLfloat)(1.);
          }
        }
        wrapper->m_strip_sizes << 2 * (npts - 1);
      }
    }
  }

  virtual void BuildContourSet(ContourSet2D *cset) override
  {
    auto *wrapper = dynamic_cast<QPainterRenderContextContourSet2D *>(cset);
    if(wrapper->use_gl)
    {
      QOpenGLVertexArrayObject::Binder vao_binder(wrapper->m_vao);
      wrapper->m_vbo->bind();
      wrapper->m_vbo->allocate(wrapper->m_vertex_coords.constData(),
                               wrapper->m_vertex_coords.count() * sizeof(GLfloat));
      wrapper->m_vbo->release();


      wrapper->m_rgba_vbo->bind();
      wrapper->m_rgba_vbo->allocate(wrapper->m_vertex_colors.constData(),
                                    wrapper->m_vertex_colors.count() * sizeof(GLfloat));
      wrapper->m_rgba_vbo->release();
    }
  }

  virtual void DrawContourSet(ContourSet2D *cset) override
  {
    auto *wrapper = dynamic_cast<QPainterRenderContextContourSet2D *>(cset);
    if(wrapper->use_gl)
    {
      painter.beginNativePainting();

      GLfloat lineWidthRange[2];
      glfunc.glGetFloatv(GL_ALIASED_LINE_WIDTH_RANGE, lineWidthRange);
      GLfloat lineWidthGranularity;
      glfunc.glGetFloatv(GL_LINE_WIDTH_GRANULARITY, &lineWidthGranularity);

      // Set the attribute state
      GLfloat old_line_width;
      glfunc.glGetFloatv(GL_LINE_WIDTH, &old_line_width);
      GLboolean blend = glfunc.glIsEnabled(GL_BLEND);
      GLboolean multi_sample = glfunc.glIsEnabled(GL_MULTISAMPLE);
      glfunc.glEnable(GL_BLEND);
      glfunc.glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
      glfunc.glBlendEquation(GL_FUNC_ADD);
      glfunc.glEnable(GL_MULTISAMPLE);

      // Set the projection and world matrices
      m_proj.setToIdentity();
      m_proj.ortho(0, painter.device()->width(), painter.device()->height(), 0, -1, 1);
      QTransform tt = painter.worldTransform();
      QMatrix4x4 m_world(
        tt.m11(), tt.m21(), 0, tt.dx(), tt.m12(), tt.m22(), 0, tt.dy(), 0, 0, 1, 0, 0, 0, 0, 1);

      // Run the program
      QOpenGLVertexArrayObject::Binder vao_binder(wrapper->m_vao);
      wrapper->m_program->bind();
      wrapper->m_program->setUniformValue(wrapper->m_program->uniformLocation("projMatrix"), m_proj);
      wrapper->m_program->setUniformValue(wrapper->m_program->uniformLocation("mvMatrix"), m_world);
      wrapper->m_program->setUniformValue(wrapper->m_program->uniformLocation("lineWidth"), (GLfloat) std::max(0.5, painter.pen().widthF()));
      wrapper->m_program->setUniformValue(wrapper->m_program->uniformLocation("overallAlpha"), (GLfloat) painter.pen().color().alphaF());
      wrapper->m_program->setUniformValue(wrapper->m_program->uniformLocation("viewport_size"),
                                          painter.viewport().width(), painter.viewport().height());

      // Draw the line strips
      int s0 = 0;
      int ns = 0;
      for(const auto &ss : std::as_const(wrapper->m_strip_sizes))
        ns += ss;

      glfunc.glDrawArrays(GL_LINES, 0, ns);

      // Restore attribute state
      if(!blend)
        glfunc.glDisable(GL_BLEND);
      if(!multi_sample)
        glfunc.glDisable(GL_MULTISAMPLE);
      glfunc.glLineWidth(old_line_width);

      // Done drawing
      wrapper->m_program->release();

      painter.endNativePainting();
    }
    else
    {
      // Draw each line segment separately - slow but reliable across platforms
      const auto *v = wrapper->m_vertex_coords.constData();
      const auto *rgb = wrapper->m_vertex_colors.constData();

      QPen pen;
      painter.setRenderHint(QPainter::Antialiasing);
      pen.setWidthF(painter.pen().widthF());
      double user_alpha = painter.pen().color().alphaF();
      for (int i = 0; i < wrapper->m_strip_sizes.size(); i++)
      {
        for (int j = 0; j < wrapper->m_strip_sizes[i]; j += 2)
        {
          QPointF         p0(v[0], v[1]), p1(v[2], v[3]);
          QColor          c1 = QColor::fromRgbF(rgb[0], rgb[1], rgb[2], user_alpha * rgb[3]);
          QColor          c2 = QColor::fromRgbF(rgb[4], rgb[5], rgb[6], user_alpha * rgb[7]);
          QLinearGradient grad(p0, p1);
          grad.setColorAt(0.0, c1); // color at start
          grad.setColorAt(1.0, c2);
          pen.setBrush(grad);
          painter.setPen(pen);
          painter.drawLine(p0, p1);
          v += 4;   // Move to next segment
          rgb += 8; // Move to next color pair
        }
      }
    }
  }

  virtual void DrawLine(double x0, double y0, double x1, double y1) override
  {
    painter.drawLine(QLineF(x0, y0, x1, y1));
  }

  virtual void DrawLines(const VertexVector &vertex_pairs) override
  {
    QList<QPointF> pp;
    pp.reserve(vertex_pairs.size());
    for(auto v : vertex_pairs)
      pp.append(QPointF(v[0], v[1]));
    painter.drawLines(pp);
  }

  virtual void DrawPolyLine(const VertexVector &polyline) override
  {
    QPolygonF p;
    p.reserve(polyline.size());
    for(const auto &v : polyline)
      p << QPointF(v[0], v[1]);
    painter.drawPolyline(p);
  }

  virtual void DrawPoint(double x, double y) override
  {
    painter.drawPoint(QPointF(x, y));
  }

  virtual void DrawRect(double x, double y, double w, double h) override
  {
    painter.drawRect(QRectF(x, y, w, h));
  }

  virtual void FillRect(double x, double y, double w, double h) override
  {
    painter.fillRect(QRectF(x, y, w, h), painter.brush());
  }

  virtual void DrawEllipse(double x, double y, double rx, double ry) override
  {
    painter.drawEllipse(QPointF(x,y), rx, ry);
  }

  virtual void SetPenColor(const Vector3d &rgb) override
  {
    QPen pen = painter.pen();
    QColor color = pen.color();
    color.setRgbF(rgb[0], rgb[1], rgb[2], color.alphaF());
    pen.setColor(color);
    painter.setPen(pen);
  }

  virtual void SetPenColor(const Vector3d &rgb, double opacity) override
  {
    QPen pen = painter.pen();
    pen.setColor(QColor::fromRgbF(rgb[0], rgb[1], rgb[2], opacity));
    painter.setPen(pen);
  }

  virtual void SetPenOpacity(double alpha) override
  {
    QPen pen = painter.pen();
    QColor color = pen.color();
    color.setAlphaF(alpha);
    pen.setColor(color);
    painter.setPen(pen);
  }

  virtual void SetPenWidth(double width) override
  {
    QPen pen = painter.pen();
    pen.setWidthF(width * painter.device()->devicePixelRatioF());
    pen.setCosmetic(true);
    painter.setPen(pen);
  }

  void SetPenStyleFromVTK(QPen &pen, int vtk_type)
  {
    switch(vtk_type)
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
  }

  virtual void SetPenLineType(int type) override
  {
    QPen pen = painter.pen();
    SetPenStyleFromVTK(pen, type);
    painter.setPen(pen);
  }

  virtual void SetPenAppearance(const OpenGLAppearanceElement &as) override
  {
    // TODO: check deviceratio for width
    QPen pen;
    auto c = as.GetColor();
    pen.setColor(QColor::fromRgbF(c[0], c[1], c[2], as.GetAlpha()));
    pen.setCosmetic(true);
    pen.setWidthF(as.GetLineThickness() * painter.device()->devicePixelRatioF());
    SetPenStyleFromVTK(pen, as.GetLineType());
    painter.setPen(pen);
  }

  virtual void SetBrush(const Vector3d &rgb, double alpha = 1.0) override
  {
    QBrush brush(Qt::SolidPattern);
    brush.setColor(QColor::fromRgbF(rgb[0], rgb[1], rgb[2], alpha));
    painter.setBrush(brush);
  }

  using CompositionMode = AbstractRenderContext::CompositionMode;

  virtual void SetCompositionMode(CompositionMode mode) override
  {
    switch(mode)
    {
      case AbstractRenderContext::SOURCE_OVER:
        painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
        break;
      case AbstractRenderContext::DESTINATION_OVER:
        painter.setCompositionMode(QPainter::CompositionMode_DestinationOver);
        break;
      case AbstractRenderContext::MULTIPLY:
        painter.setCompositionMode(QPainter::CompositionMode_Xor);
        break;
      case AbstractRenderContext::UNKNOWN:
        break;
    }
  }

  virtual CompositionMode GetCompositionMode() override
  {
    switch(painter.compositionMode())
    {
      case QPainter::CompositionMode_SourceOver:
        return AbstractRenderContext::SOURCE_OVER;
      case QPainter::CompositionMode_DestinationOver:
        return AbstractRenderContext::DESTINATION_OVER;
      case QPainter::CompositionMode_Multiply:
        return AbstractRenderContext::MULTIPLY;
      default:
        return AbstractRenderContext::UNKNOWN;
    }
  }

  virtual void FillViewport(const Vector3d &rgb) override
  {
    painter.save();
    painter.setWorldTransform(QTransform());
    painter.fillRect(painter.window(), QColor::fromRgbF(rgb[0], rgb[1], rgb[2]));
    painter.restore();
  }

  virtual void SetFont(const FontInfo &fi) override
  {
    QFont qfont;
    switch(fi.type)
    {
      case AbstractRendererPlatformSupport::SERIF:
        qfont = QFontDatabase::systemFont(QFontDatabase::GeneralFont); break;
      case AbstractRendererPlatformSupport::SANS:
        qfont = QFontDatabase::systemFont(QFontDatabase::GeneralFont); break;
      case AbstractRendererPlatformSupport::TYPEWRITER:
        qfont = QFontDatabase::systemFont(QFontDatabase::FixedFont); break;
    }

    qfont.setPixelSize(fi.pixel_size);
    qfont.setBold(fi.bold);
    painter.setFont(qfont);
  }

  virtual int TextWidth(const std::string &str) override
  {
    return painter.fontMetrics().horizontalAdvance(QString::fromUtf8(str.c_str()));
  }

  virtual void DrawText(const std::string &str) override
  {
    painter.setFont(QFont("Arial", 30));
    // QRect rect(0, 0, painter.device()->width(), painter.device()->height());
    // painter.drawText(rect, Qt::AlignCenter, str.c_str());
    painter.drawText(QPointF(0, 0), QString::fromUtf8(str.c_str()));
  }

  virtual void DrawText(const std::string &text,
                        double             x,
                        double             y,
                        double             w,
                        double             h,
                        int                halign,
                        int                valign) override
  {
    int ah = Qt::AlignHCenter, av = Qt::AlignVCenter;
    switch(halign)
    {
      case AbstractRendererPlatformSupport::LEFT    : ah = Qt::AlignLeft;    break;
      case AbstractRendererPlatformSupport::HCENTER : ah = Qt::AlignHCenter; break;
      case AbstractRendererPlatformSupport::RIGHT   : ah = Qt::AlignRight;   break;
    }
    switch(valign)
    {
      case AbstractRendererPlatformSupport::BOTTOM  : av = Qt::AlignBottom;  break;
      case AbstractRendererPlatformSupport::VCENTER : av = Qt::AlignVCenter; break;
      case AbstractRendererPlatformSupport::TOP     : av = Qt::AlignTop;     break;
    }

    QRectF r_tran(x, y, w, h);
    QRectF r_phys = painter.combinedTransform().mapRect(r_tran);
    double vpprs = 1.0 / painter.device()->devicePixelRatioF();
    QRectF r_logi(
      r_phys.x() * vpprs, r_phys.y() * vpprs, r_phys.width() * vpprs, r_phys.height() * vpprs);

    painter.save();
    painter.resetTransform();
    painter.drawText(r_logi, ah | av, QString::fromUtf8(text));
    painter.restore();
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
    auto *q_texture = static_cast<QPainterRenderContextTexture *>(texture);
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
    auto *q_texture = static_cast<QPainterRenderContextTexture *>(texture);
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
    auto *q_texture = static_cast<QPainterRenderContextTexture *>(texture);
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
    auto dev_h = painter.device()->height() / painter.device()->devicePixelRatio();
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

  virtual void Rotate(double angle_in_degrees) override
  {
    painter.rotate(angle_in_degrees);
  }

  virtual void Translate(double tx, double ty) override
  {
    painter.translate(tx, ty);
  }

  virtual Vector2d MapScreenOffsetToWorldOffset(const Vector2d &offset, bool physical_units = false) override
  {
    double scale = physical_units ? 1 : painter.device()->devicePixelRatioF();
    QPointF s0(0., 0.);
    QPointF s1(offset[0] * scale, -offset[1] * scale);
    QPointF w0 = painter.combinedTransform().inverted().map(s0);
    QPointF w1 = painter.combinedTransform().inverted().map(s1);
    return Vector2d(w1.x() - w0.x(), w1.y() - w0.y());
  }

  virtual double GetDevicePixelRatio() override
  {
    return painter.device()->devicePixelRatioF();
  }

protected:
  QPainter &painter;
  QOpenGLFunctions &glfunc;

  // GL stuff - organize later
  QOpenGLVertexArrayObject m_vao;
  static QOpenGLShaderProgram *m_program;
  QMatrix4x4 m_proj, m_world;
};



#endif // QPAINTERNEWRENDERCONTEXT_H
