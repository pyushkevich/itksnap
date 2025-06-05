#ifndef DEEPLEARNINGSERVEREDITOR_H
#define DEEPLEARNINGSERVEREDITOR_H

#include <QDialog>
#include <QProcess>
#include <QPlainTextEdit>
#include "SNAPComponent.h"
#include "DeepLearningSegmentationModel.h"

class QProcessOutputTextWidget;

namespace Ui
{
class DeepLearningServerEditor;
}

class PythonFinderWorker : public QObject
{
  Q_OBJECT
public slots:
  void findPythonInterpreters();
signals:
  void interpretersFound(const QStringList &interpreters);
};


class PythonProcess : public QObject
{
  Q_OBJECT

public:
  PythonProcess(const QString& pythonExe, const QStringList &args, QProcessOutputTextWidget* outputWidget, QObject* parent = nullptr);

  void start();
  bool waitForFinished();

signals:
  void finished(int exitCode, QProcess::ExitStatus status);

private slots:
  void onFinished(int exitCode, QProcess::ExitStatus status);

private:
  QProcess* m_Process;
  QProcessOutputTextWidget* m_OutputWidget;
  QString m_PythonExe;
  QStringList m_Args;
};


class DeepLearningServerEditor : public SNAPComponent
{
  Q_OBJECT

public:
  explicit DeepLearningServerEditor(QWidget *parent = nullptr);
  ~DeepLearningServerEditor();

  void SetModel(DeepLearningServerPropertiesModel *model);

private slots:
  void onModelUpdate(const EventBucket &bucket);
  void on_VEnvInstallFinished(int exitCode, QProcess::ExitStatus status);
  void on_PipUpgradePipFinished(int exitCode, QProcess::ExitStatus status);
  void on_PipInstallDLSFinished(int exitCode, QProcess::ExitStatus status);
  void on_SetupDLSFinished(int exitCode, QProcess::ExitStatus status);

  void on_btnFindVEnvFolder_clicked();
  void on_btnResetVEnvFolderToDefault_clicked();
  void on_btnFindPythonExe_clicked();

  void on_btnConfigurePackages_clicked();
  void updateVEnvStatusDisplay();

private:
  Ui::DeepLearningServerEditor *ui;
  DeepLearningServerPropertiesModel *m_Model;
  QStringList m_KnownPythonExes;
  void                               StartPythonExeSearch();
};

#endif // DEEPLEARNINGSERVEREDITOR_H
