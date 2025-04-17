#include "SSHTunnelWorkerThread.h"
#include <QDebug>

SSHTunnelWorkerThread::~SSHTunnelWorkerThread()
{
  qDebug() << "Destructor SSHTunnelWorkerThread " << this;
}

void
SSHTunnelWorkerThread::passwordResponse(QString password, bool abort)
{
  m_PasswordCallbackValue = password;
  m_PasswordCallbackAbort = abort;
  emit promptCompleted();
}

void
SSHTunnelWorkerThread::terminate()
{
  m_TerminateMutex.lock();
  m_Terminate = true;
  m_TerminateMutex.unlock();
}

void
SSHTunnelWorkerThread::run()
{
  // Run the tunnel loop
  int rc = SSHTunnel::run(m_Hostname.toUtf8(),
                          m_RemotePort,
                          m_SSHUserName.toUtf8(),
                          m_SSHPrivateKeyFile.toUtf8(),
                          &SSHTunnelWorkerThread::static_callback,
                          this);

  qDebug() << "SSHTunnel::run exited with RC=" << rc;
}

SSHTunnel::CallbackResponse
SSHTunnelWorkerThread::prompt(CallbackType ctype, CallbackInfo info)
{
  // Create an event loop
  QEventLoop loop;
  QObject::connect(this, &SSHTunnelWorkerThread::promptCompleted, &loop, &QEventLoop::quit);

  // Ask for input; here we need to block until the input has been provided
  if(ctype == SSHTunnel::CB_PROMPT_PASSWORD)
    emit tunnelPasswordPrompt(std::get<SSHTunnel::PromptPasswordInfo>(info));
  loop.exec();

  // Don't store password in memory!
  QString ptemp = m_PasswordCallbackValue;
  m_PasswordCallbackValue = QString();
  return std::make_pair(m_PasswordCallbackAbort ? 1 : 0, ptemp.toStdString());
}


SSHTunnel::CallbackResponse
SSHTunnelWorkerThread::callback(CallbackType ctype, CallbackInfo info)
{
  QString ptemp;
  switch (ctype)
  {
    case SSHTunnel::CB_ERROR:
    {
      auto message = std::get<SSHTunnel::ErrorInfo>(info).error_message;
      qCritical() << "ERROR: " << message;
      emit tunnelError(QString::fromStdString(message));
      break;
    }
    case SSHTunnel::CB_WARNING:
    {
      auto message = std::get<SSHTunnel::ErrorInfo>(info).error_message;
      qWarning() << "WARNING: " << message;
      emit tunnelError(QString::fromStdString(message));
      break;
    }
    case SSHTunnel::CB_READY:
    {
      auto ready_info = std::get<SSHTunnel::ReadyInfo>(info);
      emit tunnelReady(ready_info.local_port);
      qInfo() << "TUNNEL RUNNING ON HOST " << ready_info.hostname << " PORT " << ready_info.local_port;
      break;
    }
    case SSHTunnel::CB_PROMPT_PASSWORD:
    case SSHTunnel::CB_PROMPT_PASSKEY:
    {
      return prompt(ctype, info);
    }
    case SSHTunnel::CB_TERMINATION_CHECK:
    {
      m_TerminateMutex.lock();
      bool terminate = m_Terminate;
      m_TerminateMutex.unlock();
      if (terminate)
        return std::make_pair(1, std::string());
      break;
    }
  }

  return std::make_pair(0, std::string());
}

SSHTunnel::CallbackResponse
SSHTunnelWorkerThread::static_callback(CallbackType type, CallbackInfo info, void *data)
{
  return static_cast<SSHTunnelWorkerThread *>(data)->callback(type, info);
}
