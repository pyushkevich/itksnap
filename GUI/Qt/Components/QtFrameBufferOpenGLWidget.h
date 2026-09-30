#ifndef QTFRAMEBUFFEROPENGLWIDGET_H
#define QTFRAMEBUFFEROPENGLWIDGET_H

#include <QOpenGLWidget>
#include <QOpenGLFunctions>

class AbstractContextBasedRenderer;
class QOpenGLFramebufferObject;
class QOpenGLPaintDevice;
class QPainter;
class ShaderTest;
class QPainterRenderContext;
class VTKTest;
class QScreen;

/**
 * A simple OpenGL widget with a framebuffer to use with AbstractRenderer,
 * supports multisampling
 */
class QtFrameBufferOpenGLWidget : public QOpenGLWidget, protected QOpenGLFunctions
{
public:
  QtFrameBufferOpenGLWidget(QWidget *parent);

  ~QtFrameBufferOpenGLWidget();

  void SetRenderer(AbstractContextBasedRenderer *r);


  void initializeGL() override;

  void resizeGL(int w, int h) override;

  void paintGL() override;

  void setScreenshotRequest(const std::string &filename);

  virtual void showEvent(QShowEvent *event) override;

protected:
  void updateFBO();
  void connectCurrentScreen(bool force);

private slots:
  void onScreenChanged();
  void onScreenGeometryChanged();

private:
  AbstractContextBasedRenderer *m_Renderer;
  std::string m_ScreenshotRequest;

  QOpenGLFramebufferObject* m_FrameBufferObject = nullptr;
  ShaderTest *shader_test = nullptr;
  VTKTest *vtk_test = nullptr;

  QMetaObject::Connection m_CurrentScreenChangedConnection, m_CurrentScreenGeometryChangedConnection;
};


/**
 * An OpenGL widget connected to an AbstractRenderer, does not use internal frame
 * buffer and might not support multisampling. Provided as a backup.
 */
class QtDirectRenderOpenGLWidget : public QOpenGLWidget, protected QOpenGLFunctions
{
public:
  QtDirectRenderOpenGLWidget(QWidget *parent);

  void SetRenderer(AbstractContextBasedRenderer *r);

  void initializeGL() override;

  void paintGL() override;

private:
  AbstractContextBasedRenderer *renderer;
};



#endif // QTFRAMEBUFFEROPENGLWIDGET_H
