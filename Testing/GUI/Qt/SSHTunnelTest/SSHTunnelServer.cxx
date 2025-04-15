#include "SSHTunnelServer.h"
#include <QNetworkInterface>
#include "DeepLearningSegmentationModel.h"
#include "IRISException.h"
#include <QMessageBox>






#ifdef __OLDCODE__

SSHTunnelServer::SSHTunnelServer(QObject *parent)
  : QObject(parent)
  , m_Tunnel(nullptr)
{
  // initServer();
  // connect(m_Server, &QTcpServer::newConnection, this, &SSHTunnelServer::handleConnection);
}

void
SSHTunnelServer::SetServerProperties(const DeepLearningServerPropertiesModel *model)
{
  // Make a copy of the server properties - no pointers so we can be safely moved
  // to another thread
  m_Hostname = QString::fromStdString(model->GetHostname());
  m_SSHUserName = QString::fromStdString(model->GetSSHUsername());
  m_SSHPrivateKeyFile = QString::fromStdString(model->GetSSHPrivateKeyFile());
  m_RemotePort = model->GetPort();
}

void
SSHTunnelServer::createTunnel()
{
  // Create a new tunnel object
  if(m_Tunnel)
    delete m_Tunnel;
  m_Tunnel = new SSHTunnel();

  // Initialize the server (we need the port number)
  initServer();
  connect(m_Server, &QTcpServer::newConnection, this, &SSHTunnelServer::handleConnection);

  // Establish the connection
  try
  {
    // Start the tunnel
    m_Tunnel->CreateTunnel(m_Hostname.toUtf8(),
                           this->GetLocalPort(),
                           m_RemotePort,
                           m_SSHUserName.toUtf8(),
                           NULL,
                           m_SSHPrivateKeyFile.toUtf8());

    // Let them know that the tunnel is running
    emit tunnelEstablished(this->GetLocalPort());
  }
  catch (IRISException &exc)
  {
    emit tunnelCreationFailed(QString(exc.what()));
  }

  qInfo() << "Server is running on port " << m_Server->serverPort();

  // Run the tunnel
  // while(m_Server->isListening())
  // {
  //  qDebug() << "Server is running on port " << m_Server->serverPort();
  //  sleep(5);
  //}
}

int
SSHTunnelServer::GetLocalPort()
{
  return m_Server->isListening() ? m_Server->serverPort() : 0;
}

void
SSHTunnelServer::initServer()
{
  // ssh_set_log_level(SSH_LOG_TRACE);
  m_Server = new QTcpServer(this);
  if (!m_Server->listen(QHostAddress::Any, 0))
  {
    qCritical() << "Server failed to start";
    return;
  }
  qInfo() << "Server listening on port " << m_Server->serverPort();


  const QList<QHostAddress> ipAddressesList = QNetworkInterface::allAddresses();
  QString                   ipAddress;
  // use the first non-localhost IPv4 address
  for (const QHostAddress &entry : ipAddressesList)
  {
    if (entry != QHostAddress::LocalHost && entry.toIPv4Address())
    {
      ipAddress = entry.toString();
      break;
    }
  }
  // if we did not find one, use IPv4 localhost
  if (ipAddress.isEmpty())
    ipAddress = QHostAddress(QHostAddress::LocalHost).toString();

  qDebug()
    << QString("The server is running on IP: %1 port: %2").arg(ipAddress).arg(m_Server->serverPort());
}

/*

void
SSHTunnelServer::handleConnection()
{
  qDebug() << "Entered handleConnection()";
  qDebug() << "Blocking state is " << ssh_is_blocking(m_Tunnel->GetSession());
  auto *socket = m_Server->nextPendingConnection();
  connect(socket, &QTcpSocket::readyRead, this, [socket, this]() {
    qDebug() << QString("Entering readyRead handler");
    m_Tunnel->ReestablishChannel();
    // Write the data to the SSH channel
    QByteArray data = socket->readAll();
    qDebug() << QString("Read %1 bytes from socket").arg(data.size());
    ssh_channel_write(m_Tunnel->GetChannel(), data.data(), data.size());
    ssh_channel_send_eof(m_Tunnel->GetChannel());
    qDebug() << QString("Wrote %1 bytes to ssh channel").arg(data.size());

    // Now wait for the response
    static constexpr unsigned int bsize = 1024 * 1024;
    char                          buffer[bsize];
    while (true)
    {
      int n_read = ssh_channel_read(m_Tunnel->GetChannel(), buffer, bsize, 0);
      // int eof = ssh_channel_is_eof(m_Tunnel->GetChannel());
      qDebug() << QString("Read %1 bytes from ssh channel").arg(n_read);
      if (n_read > 0)
      {
        QByteArray ba(buffer, n_read);
        socket->write(ba);
        qDebug() << QString("Wrote byte array of size %1 bytes to socket").arg(ba.size());
      }
      else if(n_read == 0)
      {
        socket->flush();
        qDebug() << QString("Flushed the socket");
        break;
      }
      else
      {
        qCritical() << "Error!";
      }
    }
    qDebug() << QString("Exiting readyRead handler");
  });
  connect(socket, &QTcpSocket::disconnected, socket, &QTcpSocket::deleteLater);
  socket->waitForDisconnected();
}

*/

void
SSHTunnelServer::handleConnection()
{
  qDebug() << "Entered handleConnection()";
  auto *socket = m_Server->nextPendingConnection();
  connect(socket, &QTcpSocket::readyRead, this, [socket, this]() {
    qDebug() << QString("Entering readyRead handler");
    if(m_Tunnel->ReestablishChannel())
    {
      qDebug() << "Reestablished channel due to EOF";
    }
    else
    {
      qDebug() << "Did not have to reestablish channel";
    }

    // Write the data to the SSH channel
    QByteArray data = socket->readAll();
    qDebug() << QString("Read %1 bytes from socket").arg(data.size());

    // Loop this because channel may fail
    for (unsigned int i = 0; i < 5; i++)
    {
      qDebug() << QString("=== Tunnel attempt %1 of %2 ===").arg(i+1).arg(5);
      // Write the packet to channel
      int written = 0;
      int n;
      do
      {
        n = ssh_channel_write(m_Tunnel->GetChannel(), data.constData() + written, data.size() - written);
        written += n;
      } while ((written != data.size()) && (n != 0));

      qDebug() << QString("Wrote %1 bytes to ssh channel.").arg(written);

      // Wait for a response
      ssh_channel c_in[2] = { m_Tunnel->GetChannel(), nullptr };
      ssh_channel c_out[2];
      timeval timeout = {60,0};
      fd_set set;
      FD_ZERO (&set);

      typedef std::chrono::high_resolution_clock Clock;
      auto t0 = Clock::now();
      int rc = ssh_select(c_in, c_out, 0, &set, &timeout);
      auto t1 = Clock::now();
      qDebug() << QString("ssh_select completed in %1 µs with RC %2, have out channel = %3")
                    .arg(std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count())
                    .arg(rc)
                    .arg(c_out[0] != nullptr);

      // Check if the channel is at EOF (not sure why it would be)
      int available = ssh_channel_poll(c_out[0], 0);
      if (available <= 0)
      {
        auto        s = m_Tunnel->GetSession();
        std::string err(ssh_get_error(&s));
        qDebug() << QString("ssh_channel_poll error: %1").arg(QString::fromStdString(err));
        m_Tunnel->ReestablishChannel();
        continue;
      }

      QByteArray data;
      data.resize(available);
      // int n = ssh_channel_read_nonblocking(m_Tunnel->GetChannel(), data.data(), available, 0);
      n = ssh_channel_read(c_out[0], data.data(), available, 0);
      qDebug() << QString("Read %1 bytes from ssh channel").arg(n);
      // qDebug() << data.toStdString();
      socket->write(data);
      qDebug() << QString("Wrote byte array of size %1 bytes to socket").arg(data.size());
      break;
    }
  });
  connect(socket, &QTcpSocket::disconnected, socket, &QTcpSocket::deleteLater);
  socket->waitForDisconnected();
}

SSHTunnelManager::SSHTunnelManager(QObject *parent, const DeepLearningServerPropertiesModel *props)
{
  m_Hostname = QString::fromStdString(props->GetHostname());
  m_SSHUserName = QString::fromStdString(props->GetSSHUsername());
  m_SSHPrivateKeyFile = QString::fromStdString(props->GetSSHPrivateKeyFile());
  m_RemotePort = props->GetPort();

  if (m_SSHUserName.isEmpty())
  {
    /* We might have a config file request pending, honor this. */
    ssh_session tmp_session = ssh_new();

    if (tmp_session)
    {
      QByteArray tmp_BA = m_Hostname.toLocal8Bit();
      ssh_options_set(tmp_session, SSH_OPTIONS_HOST, tmp_BA.data());

      if (ssh_options_parse_config(tmp_session, NULL) < 0)
      {
        qWarning() << "Warning: unable to parse the SSH config file.";
      }

      char *inferred_username = NULL;
      ssh_options_get(tmp_session, SSH_OPTIONS_USER, &inferred_username);
      qDebug() << "Temporary session user name after config file parse: " << inferred_username;

      m_SSHUserName = QString::fromLocal8Bit(inferred_username);

      ssh_string_free_char(inferred_username);
      ssh_free(tmp_session);
    }
  }
}

#endif // __OLDCODE__

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
