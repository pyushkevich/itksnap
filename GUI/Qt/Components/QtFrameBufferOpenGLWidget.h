#ifndef QTFRAMEBUFFEROPENGLWIDGET_H
#define QTFRAMEBUFFEROPENGLWIDGET_H

#include <QOpenGLWidget>
#include <QOpenGLFunctions>

class AbstractNewRenderer;
class QOpenGLFramebufferObject;

/**
 * A simple OpenGL widget with a framebuffer to use with AbstractNewRenderer,
 * supports multisampling
 */
class QtFrameBufferOpenGLWidget : public QOpenGLWidget, protected QOpenGLFunctions
{
public:
  QtFrameBufferOpenGLWidget(QWidget *parent);

  ~QtFrameBufferOpenGLWidget();

  void SetRenderer(AbstractNewRenderer *r);

  void updateFBO();

  void initializeGL() override;

  void resizeGL(int w, int h) override;

  void paintGL() override;

  void setScreenshotRequest(const std::string &filename);

private:
  AbstractNewRenderer *m_Renderer;
  std::string m_ScreenshotRequest;

  QOpenGLFramebufferObject* m_FrameBufferObject = nullptr;
};


/**
 * An OpenGL widget connected to an AbstractNewRenderer, does not use internal frame
 * buffer and might not support multisampling. Provided as a backup.
 */
class QtDirectRenderOpenGLWidget : public QOpenGLWidget, protected QOpenGLFunctions
{
public:
  QtDirectRenderOpenGLWidget(QWidget *parent);

  void SetRenderer(AbstractNewRenderer *r);

  void initializeGL() override;

  void paintGL() override;

private:
  AbstractNewRenderer *renderer;
};



#endif // QTFRAMEBUFFEROPENGLWIDGET_H
