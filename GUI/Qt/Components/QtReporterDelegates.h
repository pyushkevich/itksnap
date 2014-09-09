#ifndef QTREQUESTDELEGATES_H
#define QTREQUESTDELEGATES_H

#include <QWidget>
#include "UIReporterDelegates.h"

class QProgressDialog;
class QGLWidget;

/**
  An implementation of a viewport reporter for Qt.

  CAVEAT: the reported must be destroyed before the client widget
  */
class QtViewportReporter : public ViewportSizeReporter
{
public:

  irisITKObjectMacro(QtViewportReporter, ViewportSizeReporter)

  /** Set the widget that we report on */
  void SetClientWidget(QWidget *widget);

  bool CanReportSize();

  Vector2ui GetViewportSize();

  float GetViewportPixelRatio();

  Vector2ui GetLogicalViewportSize();

protected:

  QtViewportReporter();
  virtual ~QtViewportReporter();

  QWidget *m_ClientWidget;


  /** This helper class intercepts resize events from the widget */
  class EventFilter : public QObject
  {
  public:
    bool eventFilter(QObject *object, QEvent *event);
    QtViewportReporter *m_Owner;
  };

  EventFilter *m_Filter;

};

class QtProgressReporterDelegate : public ProgressReporterDelegate
{
public:
  QtProgressReporterDelegate();

  void SetProgressDialog(QProgressDialog *dialog);
  void SetProgressValue(double);

private:
  QProgressDialog *m_Dialog;
};

class QtTextRenderingDelegate : public TextRenderingDelegate
{
public:

  virtual void RenderTextInOpenGL(
      const char *text,
      int x, int y, int w, int h,
      int font_size,
      int align_horiz, int align_vert,
      unsigned char rgba[]);

protected:

};

class QtSystemInfoDelegate : public SystemInfoDelegate
{
public:
  virtual std::string GetApplicationDirectory();
  virtual std::string GetApplicationFile();
  virtual std::string GetApplicationPermanentDataLocation();

  typedef itk::Image<unsigned char, 2> GrayscaleImage;

  virtual void LoadResourceAsImage2D(std::string tag, GrayscaleImage *image);
  virtual void LoadResourceAsRegistry(std::string tag, Registry &reg);
};

#endif // QTREQUESTDELEGATES_H
