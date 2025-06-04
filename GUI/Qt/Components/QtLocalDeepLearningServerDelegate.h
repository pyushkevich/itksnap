#ifndef QTLOCALDEEPLEARNINGSERVERDELEGATE_H
#define QTLOCALDEEPLEARNINGSERVERDELEGATE_H

#include "DeepLearningSegmentationModel.h"
#include <QObject>
#include <QProcess>

class QProcessOutputTextWidget;

class QtLocalDeepLearningServerDelegate : public QObject, public AbstractLocalDeepLearningServerDelegate
{
  Q_OBJECT;

public:
  QtLocalDeepLearningServerDelegate(QObject *parent, QProcessOutputTextWidget *output_widget);

  virtual int StartServerIfNeeded(DeepLearningServerPropertiesModel *properties) override;

private slots:
  void onStarted();
  void onFinished(int exitCode, QProcess::ExitStatus status);

protected:

  void terminateProcess(QProcess *process);

  std::string m_CurrentProcessHash;
  int m_CurrentProcessPort;
  QProcess *m_Process = nullptr;
  QProcessOutputTextWidget *m_OutputWidget = nullptr;
};

#endif // QTLOCALDEEPLEARNINGSERVERDELEGATE_H
