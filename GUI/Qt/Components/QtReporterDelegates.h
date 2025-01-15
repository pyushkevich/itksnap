#ifndef QTREQUESTDELEGATES_H
#define QTREQUESTDELEGATES_H

#include <QWidget>
#include "UIReporterDelegates.h"
#include "IPCHandler.h"

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

  bool CanReportSize() override;

  Vector2ui GetViewportSize() override;

  float GetViewportPixelRatio() override;

  Vector2ui GetLogicalViewportSize() override;

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

class QtSystemInfoDelegate : public SystemInfoDelegate
{
public:
  virtual std::string GetApplicationDirectory() override;
  virtual std::string GetApplicationFile() override;
  virtual std::string GetApplicationPermanentDataLocation() override;
  virtual std::string GetUserDocumentsLocation() override;
  virtual std::string EncodeServerURL(const std::string &url) override;


  typedef SystemInfoDelegate::GrayscaleImage GrayscaleImage;
  typedef SystemInfoDelegate::RGBAPixelType RGBAPixelType;
  typedef SystemInfoDelegate::RGBAImageType RGBAImageType;

  virtual void LoadResourceAsImage2D(std::string tag, GrayscaleImage *image) override;
  virtual void LoadResourceAsRegistry(std::string tag, Registry &reg) override;

  virtual void WriteRGBAImage2D(std::string file, RGBAImageType *image) override;
};

class QSharedMemory;

class QtSharedMemorySystemInterface : public AbstractSharedMemorySystemInterface
{
public:
  QtSharedMemorySystemInterface();
  virtual ~QtSharedMemorySystemInterface() override;

  virtual void SetKey(const std::string &key) override;
  virtual bool Attach() override;
  virtual bool Detach() override;
  virtual bool Create(unsigned int size) override;
  virtual bool IsAttached() override;
  virtual std::string GetErrorMessage() override;
  virtual void* Data() override;
  virtual bool Lock() override;
  virtual bool Unlock() override;
  virtual int GetProcessID() override;

protected:
  QSharedMemory *m_SharedMem;
};


#endif // QTREQUESTDELEGATES_H
