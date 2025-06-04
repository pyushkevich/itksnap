#include "QtLocalDeepLearningServerDelegate.h"
#include "QProcessOutputTextWidget.h"
#include <QTimer>
#include <QDir>
#include <QPlainTextEdit>
#include <QTcpServer>

quint16
findAvailablePort()
{
  QTcpServer server;
  if (server.listen(QHostAddress::LocalHost, 0))
  {
    quint16 port = server.serverPort();
    server.close(); // You can close it if you just need the port number
    return port;
  }
  else
  {
    qWarning() << "Could not find an available port:" << server.errorString();
    return 0;
  }
}


QtLocalDeepLearningServerDelegate::QtLocalDeepLearningServerDelegate(QObject *parent,
                                                                     QProcessOutputTextWidget *output_widget)
  : QObject(parent)
{
  this->m_OutputWidget = output_widget;
}

int
QtLocalDeepLearningServerDelegate::StartServerIfNeeded(DeepLearningServerPropertiesModel *properties)
{
  // Is a local server configured?
  if(properties && !properties->GetRemoteConnection())
  {
    // Are we already running a local server with these properties?
    if(properties->GetHash() == m_CurrentProcessHash && m_Process && m_Process->state() != QProcess::NotRunning)
    {
      // Return code of 0 means keep same port
      return 0;
    }

    // Shutdown the current process and start over
    if(m_Process)
      terminateProcess(m_Process);
    m_Process = new QProcess(this);

    // Set environment for the process
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert("FORCE_COLOR", "1");
    m_Process->setProcessEnvironment(env);

    // Listen to process events
    connect(m_Process,
            &QProcess::readyReadStandardOutput,
            m_OutputWidget,
            &QProcessOutputTextWidget::appendStdout);
    connect(m_Process,
            &QProcess::readyReadStandardOutput,
            m_OutputWidget,
            &QProcessOutputTextWidget::appendStderr);
    connect(m_Process,
            QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this,
            &QtLocalDeepLearningServerDelegate::onFinished);
    connect(m_Process,
            &QProcess::started,
            this,
            &QtLocalDeepLearningServerDelegate::onStarted);

    // Find the executable to run
    QDir venv_dir(QString::fromStdString(properties->GetLocalPythonVEnvPath()));
    QString venvPython;
#if defined(Q_OS_WIN)
    venvPython = venv_dir.filePath("Scripts/python.exe");
#else
    venvPython = venv_dir.filePath("bin/python");
#endif

    // Configure the arguments to run
    auto port = findAvailablePort();
    QStringList args;
    args << "-m" << "itksnap_dls" << "--port" << QString("%1").arg(port) << "--use-colors";
    if(properties->GetNoSSLVerify())
      args << "-k";
    m_Process->start(venvPython, args);

    // Show the text output
    m_OutputWidget->clear();
    m_OutputWidget->show();

    // Return the port number on which we started the process
    return port;
  }
  else
  {
    // Shutdown the current process and start over
    if(m_Process)
    {
      terminateProcess(m_Process);
      m_Process = nullptr;
    }

    // Hide the text output
    m_OutputWidget->hide();

    // Means failed to start
    return -1;
  }
}

void
QtLocalDeepLearningServerDelegate::onStarted()
{
  auto *process = dynamic_cast<QProcess *>(sender());
  if(process)
  {
    m_OutputWidget->clear();
    m_OutputWidget->show();
    m_OutputWidget->appendPlainText("---------------------------------------\n");
    m_OutputWidget->appendPlainText(
      QString("Running %1 %2\n").arg(process->program(), process->arguments().join(" ")));
    m_OutputWidget->appendPlainText("---------------------------------------\n");
  }
}

void
QtLocalDeepLearningServerDelegate::onFinished(int exitCode, QProcess::ExitStatus status)
{
  auto *process = dynamic_cast<QProcess *>(sender());
  if(process)
  {
    m_OutputWidget->appendPlainText(QString("Process finished with code %1\n").arg(exitCode));
  }
}

void
QtLocalDeepLearningServerDelegate::terminateProcess(QProcess *process)
{
  // Attempt graceful termination first
  process->terminate();

  // Delete when finished (either normally or by kill)
  QObject::connect(process, &QProcess::finished, process, &QObject::deleteLater);
  QObject::connect(process, &QProcess::errorOccurred, process, &QObject::deleteLater);
}
