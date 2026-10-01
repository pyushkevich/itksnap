#include "SSHTunnelWorkerThread.h"
#include "IRISException.h"
#include "SSHLogin.h"
#include <QDebug>

SSHTunnelWorkerThread::~SSHTunnelWorkerThread()
{
  qDebug() << "Destructor SSHTunnelWorkerThread " << this;
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
  // Open an authenticated session. Any prompts go through the auth delegate,
  // between connection attempts, never from inside libssh.
  SSHConnectParams params;
  params.host = m_Hostname.toStdString();
  params.username = m_SSHUserName.toStdString();
  params.keyfile = m_SSHPrivateKeyFile.toStdString();

  ssh_session session = nullptr;
  try
  {
    session = OpenSSHSession(params, m_AuthDelegate);
  }
  catch (IRISException &exc)
  {
    qCritical() << "ERROR: " << exc.what();
    emit tunnelError(QString::fromUtf8(exc.what()));
    return;
  }

  // Run the tunnel loop; it takes ownership of the session
  int rc = SSHTunnel::run(session,
                          nullptr, // the SSH server itself
                          m_RemotePort,
                          &SSHTunnelWorkerThread::static_callback,
                          this);

  qDebug() << "SSHTunnel::run exited with RC=" << rc;
}

SSHTunnel::CallbackResponse
SSHTunnelWorkerThread::callback(CallbackType ctype, CallbackInfo info)
{
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
