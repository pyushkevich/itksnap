#include "QtFrameBufferOpenGLWidget.h"
#include "AbstractContextBasedRenderer.h"
#include <QOpenGLFramebufferObject>
#include <QOpenGLPaintDevice>
#include "QPainterRenderContext.h"
#include <chrono>
#include <cstdio>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkRenderer.h>
#include <vtkExternalOpenGLRenderWindow.h>
#include <vtkSphereSource.h>
#include <vtkOpenGLFramebufferObject.h>

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

#include <QOpenGLVertexArrayObject>
#include <QOpenGLBuffer>
#include <QOpenGLShaderProgram>

static const char *vertexShaderSourceCore =
  "#version 150\n"
  "in vec4 vertex;\n"
  "out vec3 vert;\n"
  "uniform mat4 projMatrix;\n"
  "uniform mat4 mvMatrix;\n"
  "void main() {\n"
  "   vert = vertex.xyz;\n"
  "   gl_Position = projMatrix * mvMatrix * vertex;\n"
  "}\n";

static const char *fragmentShaderSourceCore =
  "#version 150\n"
  "in highp vec3 vert;\n"
  "out highp vec4 fragColor;\n"
  "void main() {\n"
  "   highp vec3 col = vec3(0.39, 1.0, 0.0);\n"
  "   fragColor = vec4(col, 1.0);\n"
  "}\n";

class ShaderTest
{
public:
  ShaderTest(QOpenGLFunctions *f)
  {
    this->f = f;
  }

  void setupVertexAttribs()
  {
    m_logoVbo.bind();
    f->glEnableVertexAttribArray(0);
    f->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), nullptr);
    m_logoVbo.release();
  }

  void initializeGL()
  {
    glClearColor(0, 0, 0, 0);

    m_program = new QOpenGLShaderProgram;
    m_program->addShaderFromSourceCode(QOpenGLShader::Vertex, vertexShaderSourceCore);
    m_program->addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentShaderSourceCore);
    m_program->bindAttributeLocation("vertex", 0);
    m_program->bindAttributeLocation("normal", 1);
    m_program->link();

    m_program->bind();
    m_projMatrixLoc = m_program->uniformLocation("projMatrix");
    m_mvMatrixLoc = m_program->uniformLocation("mvMatrix");
    m_normalMatrixLoc = m_program->uniformLocation("normalMatrix");
    m_lightPosLoc = m_program->uniformLocation("lightPos");

    // Create a vertex array object. In OpenGL ES 2.0 and OpenGL 2.x
    // implementations this is optional and support may not be present
    // at all. Nonetheless the below code works in all cases and makes
    // sure there is a VAO when one is needed.
    m_vao.create();
    QOpenGLVertexArrayObject::Binder vaoBinder(&m_vao);

    // Setup our vertex buffer object.
    m_logoVbo.create();
    m_logoVbo.bind();
    QList<GLfloat> verts;
    verts << 100.0f << 100.0f;
    verts << 200.0f << 50.0f;
    verts << 200.0f << 150.0f;
    m_logoVbo.allocate(verts.constData(), verts.count() * sizeof(GLfloat));

    // Store the vertex attribute bindings for the program.
    setupVertexAttribs();

    // Our camera never changes in this example.
    m_camera.setToIdentity();
    m_camera.translate(0, 0, -1);

    // Light position is fixed.
    m_program->release();
  }

  void resizeGL(int w, int h)
  {
    m_proj.setToIdentity();
    m_proj.ortho(0, w, 0, h, -100, 100);
  }

  void paintGL()
  {
    // f->glClearColor(1, 0, 0, 0.1);
    // f->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    // f->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    // f->glEnable(GL_DEPTH_TEST);
    // f->glEnable(GL_CULL_FACE);

    m_world.setToIdentity();

    QOpenGLVertexArrayObject::Binder vaoBinder(&m_vao);
    m_program->bind();
    m_program->setUniformValue(m_projMatrixLoc, m_proj);
    m_program->setUniformValue(m_mvMatrixLoc, m_world);

    f->glDrawArrays(GL_TRIANGLES, 0, 3);

    m_program->release();
  }

protected:

  QOpenGLFunctions *f;
  bool m_core;
  int m_xRot = 0;
  int m_yRot = 0;
  int m_zRot = 0;
  QPoint m_lastPos;
  QOpenGLVertexArrayObject m_vao;
  QOpenGLBuffer m_logoVbo;
  QOpenGLShaderProgram *m_program = nullptr;
  int m_projMatrixLoc = 0;
  int m_mvMatrixLoc = 0;
  int m_normalMatrixLoc = 0;
  int m_lightPosLoc = 0;
  QMatrix4x4 m_proj;
  QMatrix4x4 m_camera;
  QMatrix4x4 m_world;
  static bool m_transparent;
};



class VTKTest
{
public:
  vtkSmartPointer<vtkPolyData> polyData;
  vtkSmartPointer<vtkExternalOpenGLRenderWindow> renderWindow;
  vtkSmartPointer<vtkRenderer> renderer;
  vtkSmartPointer<vtkActor> actor;
  vtkSmartPointer<vtkPolyDataMapper> mapper;

  VTKTest()
  {
    vtkNew<vtkSphereSource> sphereSource;
    sphereSource->Update();
    polyData = sphereSource->GetOutput();
  }

  void initializeGL()
  {
    // Create a VTK render window using the existing OpenGL context
    renderWindow = vtkSmartPointer<vtkExternalOpenGLRenderWindow>::New();
    // renderWindow->SetForceMakeCurrent();
    // renderWindow->SetUseOffScreenBuffers(false);
    // renderWindow->SwapBuffersOff();
    // renderWindow->SetReadyForRendering(true);

    // Create a renderer
    renderer = vtkSmartPointer<vtkRenderer>::New();
    renderWindow->AddRenderer(renderer);

    // Map the PolyData to an actor
    mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInputData(polyData);
    actor = vtkSmartPointer<vtkActor>::New();
    actor->SetMapper(mapper);
    renderer->AddActor(actor);
  }

  void resizeGL(int w, int h)
  {
    renderWindow->SetSize(w, h);
  }

  void paintGL()
  {
    renderWindow->PushState();
    renderWindow->OpenGLInitState();
    renderWindow->MakeCurrent();
    renderWindow->Start();
    renderWindow->Render();
    renderWindow->PopState();
  }

};


void
QtFrameBufferOpenGLWidget::initializeGL()
{
  initializeOpenGLFunctions();
  updateFBO();
  this->shader_test = new ShaderTest(this);
  this->shader_test->initializeGL();
  this->vtk_test = new VTKTest();
  this->vtk_test->initializeGL();

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
  this->shader_test->resizeGL(w, h);
  this->vtk_test->resizeGL(w, h);

  this->vtk_test->renderWindow->SetBackLeftBuffer(GL_COLOR_ATTACHMENT0);
  this->vtk_test->renderWindow->SetFrontLeftBuffer(GL_COLOR_ATTACHMENT0);
  this->vtk_test->renderWindow->SetSize(m_FrameBufferObject->size().width(), m_FrameBufferObject->size().height());
  this->vtk_test->renderWindow->SetOffScreenRendering(true);
  this->vtk_test->renderWindow->Modified();
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
  QPainterRenderContext context(painter, *this);
  m_Renderer->Render(&context);

  auto t1 = std::chrono::system_clock::now();

  // painter.beginNativePainting();
  // this->shader_test->paintGL();
  // this->makeCurrent();
  // this->vtk_test->paintGL();
  // painter.endNativePainting();

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
  QPainterRenderContext context(painter, *this);

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
