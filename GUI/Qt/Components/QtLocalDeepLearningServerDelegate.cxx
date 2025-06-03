#include "QtLocalDeepLearningServerDelegate.h"
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


QtLocalDeepLearningServerDelegate::QtLocalDeepLearningServerDelegate(QObject        *parent,
                                                                     QPlainTextEdit *output_widget)
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
      return m_CurrentProcessPort;
    }

    // Shutdown the current process and start over
    if(m_Process)
      terminateProcess(m_Process);
    m_Process = new QProcess(this);

    // Listen to process events
    connect(m_Process,
            &QProcess::readyReadStandardOutput,
            this,
            &QtLocalDeepLearningServerDelegate::onReadyReadStandardOutput);
    connect(m_Process,
            &QProcess::readyReadStandardError,
            this,
            &QtLocalDeepLearningServerDelegate::onReadyReadStandardError);
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
    args << "-m" << "itksnap_dls" << "--port" << QString("%1").arg(port);
    m_Process->start(venvPython, args);

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
    m_OutputWidget->clear();
    m_OutputWidget->show();

    return -1;
  }
}

void
QtLocalDeepLearningServerDelegate::onReadyReadStandardOutput()
{
  auto *process = dynamic_cast<QProcess *>(sender());
  if(process)
  {
    QByteArray data = process->readAllStandardOutput();
    m_OutputWidget->appendPlainText(QString::fromLocal8Bit(data));
  }
}

void
QtLocalDeepLearningServerDelegate::onReadyReadStandardError()
{
  auto *process = dynamic_cast<QProcess *>(sender());
  if(process)
  {
    QByteArray data = process->readAllStandardError();
    m_OutputWidget->appendPlainText(QString::fromLocal8Bit(data));
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
    m_OutputWidget->appendPlainText("---------------------------------------");
    m_OutputWidget->appendPlainText(
      QString("Running %1 %2").arg(process->program(), process->arguments().join(" ")));
    m_OutputWidget->appendPlainText("---------------------------------------");
  }
}

void
QtLocalDeepLearningServerDelegate::onFinished(int exitCode, QProcess::ExitStatus status)
{
  auto *process = dynamic_cast<QProcess *>(sender());
  if(process)
  {
    m_OutputWidget->appendPlainText(QString("Process finished with code %1").arg(exitCode));
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
