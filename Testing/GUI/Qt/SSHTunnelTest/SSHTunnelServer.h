#ifndef SSHTUNNELSERVER_H
#define SSHTUNNELSERVER_H

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QThread>
#include <QMutex>
#include <QEventLoop>
#include "SSHTunnel.h"

class DeepLearningServerPropertiesModel;

class SSHTunnelWorkerThread : public QThread
{
  Q_OBJECT
public:
  using CallbackType = SSHTunnel::CallbackType;
  using CallbackInfo = SSHTunnel::CallbackInfo;

  explicit SSHTunnelWorkerThread(QObject *parent,
                                 QString  hostname,
                                 int      remote_port,
                                 QString  username,
                                 QString  keyfile)
    : QThread(parent)
    , m_Hostname(hostname)
    , m_RemotePort(remote_port)
    , m_SSHUserName(username)
    , m_SSHPrivateKeyFile(keyfile)
  {}

  virtual ~SSHTunnelWorkerThread();

signals:
  void tunnelReady(int local_port);
  void tunnelError(QString message);
  void tunnelPasswordPrompt(SSHTunnel::PromptPasswordInfo info);
  void promptCompleted();

public slots:

  void passwordResponse(QString password, bool abort);
  void terminate();

protected:
  void run() override;
  SSHTunnel::CallbackResponse prompt(CallbackType ctype, CallbackInfo info);

protected:
  SSHTunnel::CallbackResponse callback(CallbackType ctype, CallbackInfo info);
  static SSHTunnel::CallbackResponse static_callback(CallbackType type, CallbackInfo info, void *data);

  QString m_Hostname, m_SSHUserName, m_SSHPrivateKeyFile;
  int m_RemotePort;

  QString m_PasswordCallbackValue;
  bool m_PasswordCallbackAbort;
  QEventLoop m_PasswordCallbackLoop;
  QMutex m_PasswordMutex;

  bool m_Terminate = false;
  QMutex m_TerminateMutex;
};

/*

class SSHTunnelServer : public QObject
{
  Q_OBJECT
public:
  SSHTunnelServer(QObject *parent);

  void SetServerProperties(const DeepLearningServerPropertiesModel *model);

  int GetLocalPort();

public slots:
  void createTunnel();

signals:
  void tunnelEstablished(int localPort);
  void tunnelCreationFailed(const QString &error);

private slots:
  void handleConnection();

protected:
  // void incomingConnection(qintptr socketDescriptor) override;
  void initServer();

  // Server properties
  QString m_Hostname, m_SSHUserName, m_SSHPrivateKeyFile;
  int m_RemotePort;

  SSHTunnel *m_Tunnel;
  QTcpServer *m_Server = nullptr;
};


class SSHTunnelManager : public QThread
{
  Q_OBJECT

public:
  SSHTunnelManager(QObject *parent, const DeepLearningServerPropertiesModel *props);

protected:
  QString m_Hostname, m_SSHUserName, m_SSHPrivateKeyFile;
  int m_RemotePort;

  bool m_BreakLoop = false;
};

*/

#endif // SSHTUNNELSERVER_H
