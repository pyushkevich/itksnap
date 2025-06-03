#ifndef QTLOCALDEEPLEARNINGSERVERDELEGATE_H
#define QTLOCALDEEPLEARNINGSERVERDELEGATE_H

#include "DeepLearningSegmentationModel.h"
#include <QObject>
#include <QProcess>

class QPlainTextEdit;

class QtLocalDeepLearningServerDelegate : public QObject, public AbstractLocalDeepLearningServerDelegate
{
  Q_OBJECT;

public:
  QtLocalDeepLearningServerDelegate(QObject *parent, QPlainTextEdit *output_widget);

  virtual int StartServerIfNeeded(DeepLearningServerPropertiesModel *properties) override;

private slots:
  void onReadyReadStandardOutput();
  void onReadyReadStandardError();
  void onStarted();
  void onFinished(int exitCode, QProcess::ExitStatus status);

protected:

  void terminateProcess(QProcess *process);

  std::string m_CurrentProcessHash;
  int m_CurrentProcessPort;
  QProcess *m_Process = nullptr;
  QPlainTextEdit *m_OutputWidget = nullptr;
};

#endif // QTLOCALDEEPLEARNINGSERVERDELEGATE_H
