#include "QtFrameBufferOpenGLWidget.h"
#include "AbstractContextBasedRenderer.h"
#include <QOpenGLFramebufferObject>
#include <QOpenGLPaintDevice>
#include "QPainterRenderContext.h"
#include <chrono>
#include <cstdio>

QtFrameBufferOpenGLWidget::QtFrameBufferOpenGLWidget(QWidget *parent)
  : QOpenGLWidget(parent)
{}

QtFrameBufferOpenGLWidget::~QtFrameBufferOpenGLWidget()
{
  if (m_FrameBufferObject)
    delete m_FrameBufferObject;
}

void
QtFrameBufferOpenGLWidget::SetRenderer(AbstractContextBasedRenderer *r)
{
  this->m_Renderer = r;
}

void
QtFrameBufferOpenGLWidget::updateFBO()
{
  if (m_FrameBufferObject)
    delete m_FrameBufferObject;

  int vppr = this->devicePixelRatio();

  QOpenGLFramebufferObjectFormat format;
  format.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
  format.setSamples(4); // Enable multisampling for antialiasing
  m_FrameBufferObject = new QOpenGLFramebufferObject(width() * vppr, height() * vppr, format);
}

void
QtFrameBufferOpenGLWidget::initializeGL()
{
  initializeOpenGLFunctions();
  updateFBO();

  QOpenGLContext *context = QOpenGLContext::currentContext();
  if (context)
  {
    qDebug() << "OpenGL Vendor:" << reinterpret_cast<const char *>(glGetString(GL_VENDOR));
    qDebug() << "OpenGL Renderer:" << reinterpret_cast<const char *>(glGetString(GL_RENDERER));
    qDebug() << "OpenGL Version:" << reinterpret_cast<const char *>(glGetString(GL_VERSION));
  }
  else
  {
    qDebug() << "No OpenGL context available.";
  }
}

void
QtFrameBufferOpenGLWidget::resizeGL(int w, int h)
{
  updateFBO();
}

void
QtFrameBufferOpenGLWidget::paintGL()
{
  if (!m_FrameBufferObject)
    return;

  auto u0 = std::chrono::system_clock::now();

  m_FrameBufferObject->bind();
  int                vppr = this->devicePixelRatio();
  QOpenGLPaintDevice device;
  device.setDevicePixelRatio(vppr);
  device.setSize(QSize(m_FrameBufferObject->width(), m_FrameBufferObject->height()));

  auto     t0 = std::chrono::system_clock::now();
  QPainter painter(&device);
  painter.setRenderHint(QPainter::Antialiasing, true);
  QPainterRenderContext context(painter);
  m_Renderer->Render(&context);

  auto t1 = std::chrono::system_clock::now();

  m_FrameBufferObject->release();

  // Save screenshot if needed
  if (m_ScreenshotRequest.size())
  {
    m_FrameBufferObject->toImage().save(QString::fromUtf8(m_ScreenshotRequest));
    m_ScreenshotRequest.clear();
  }

  QOpenGLFramebufferObject::blitFramebuffer(
    nullptr,                                      // Target is the default framebuffer (the screen)
    QRect(0, 0, vppr * width(), vppr * height()), // Target rectangle
    m_FrameBufferObject,                          // Source framebuffer
    QRect(0, 0, m_FrameBufferObject->width(), m_FrameBufferObject->height()), // Source rectangle
    GL_COLOR_BUFFER_BIT, // Copy only the color buffer
    GL_NEAREST           // Blit scaling: no interpolation
  );
  auto u1 = std::chrono::system_clock::now();

  static int                                      n_calls = 0;
  static double                                   total_ms_t = 0, total_ms_u;
  const std::chrono::duration<double, std::milli> fp_ms_t = t1 - t0, fp_ms_u = u1 - u0;
  total_ms_t += fp_ms_t.count();
  total_ms_u += fp_ms_u.count();
  n_calls++;
  if (n_calls % 100 == 0)
  {
    char buffer[256];
    snprintf(buffer, 256, "QPainter Render Timing: FBO = %4.3f ms, Blit = %4.3f ms",
             total_ms_t / n_calls, (total_ms_u - total_ms_t) / n_calls);
    qDebug() << buffer;
    total_ms_t = 0;
    total_ms_u = 0;
    n_calls = 0;
  }
}

void
QtFrameBufferOpenGLWidget::setScreenshotRequest(const std::string &filename)
{
  m_ScreenshotRequest = filename;
}

QtDirectRenderOpenGLWidget::QtDirectRenderOpenGLWidget(QWidget *parent)
  : QOpenGLWidget(parent)
{
  this->setFormat(QSurfaceFormat::defaultFormat());
}

void
QtDirectRenderOpenGLWidget::SetRenderer(AbstractContextBasedRenderer *r)
{
  this->renderer = r;
}

void
QtDirectRenderOpenGLWidget::initializeGL()
{
  initializeOpenGLFunctions();
  QOpenGLContext *context = QOpenGLContext::currentContext();
  if (context)
  {
    qDebug() << "OpenGL Vendor:" << reinterpret_cast<const char *>(glGetString(GL_VENDOR));
    qDebug() << "OpenGL Renderer:" << reinterpret_cast<const char *>(glGetString(GL_RENDERER));
    qDebug() << "OpenGL Version:" << reinterpret_cast<const char *>(glGetString(GL_VERSION));
  }
  else
  {
    qDebug() << "No OpenGL context available.";
  }
}

void
QtDirectRenderOpenGLWidget::paintGL()
{
  auto     t0 = std::chrono::system_clock::now();
  QPainter painter(this);
  painter.setRenderHint(QPainter::Antialiasing, true);
  QPainterRenderContext context(painter);

  renderer->Render(&context);
  auto t1 = std::chrono::system_clock::now();

  static int                                      n_calls = 0;
  static double                                   total_ms = 0;
  const std::chrono::duration<double, std::milli> fp_ms = t1 - t0;
  total_ms += fp_ms.count();
  n_calls++;
  if (n_calls % 100 == 0)
  {
    std::cout << "Average paintGL duration last 100 runs: " << total_ms / n_calls << "ms."
              << std::endl;
    total_ms = 0;
    n_calls = 0;
  }
}
