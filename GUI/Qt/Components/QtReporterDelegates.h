#ifndef QTREQUESTDELEGATES_H
#define QTREQUESTDELEGATES_H

#include <QWidget>
#include "UIReporterDelegates.h"

class QProgressDialog;

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

  bool CanReportSize() ITK_OVERRIDE;

  Vector2ui GetViewportSize() ITK_OVERRIDE;

  float GetViewportPixelRatio() ITK_OVERRIDE;

  Vector2ui GetLogicalViewportSize() ITK_OVERRIDE;

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
  virtual std::string GetUserDocumentsLocation();
  virtual std::string EncodeServerURL(const std::string &url);


  typedef SystemInfoDelegate::GrayscaleImage GrayscaleImage;
  typedef SystemInfoDelegate::RGBAPixelType RGBAPixelType;
  typedef SystemInfoDelegate::RGBAImageType RGBAImageType;

  virtual void LoadResourceAsImage2D(std::string tag, GrayscaleImage *image);
  virtual void LoadResourceAsRegistry(std::string tag, Registry &reg);

  virtual void WriteRGBAImage2D(std::string file, RGBAImageType *image);
};

#endif // QTREQUESTDELEGATES_H
