#ifndef SSHTUNNELWORKERTHREAD_H
#define SSHTUNNELWORKERTHREAD_H

#include <QObject>
#include <QThread>
#include <QMutex>
#include "SSHTunnel.h"

class AbstractSSHAuthDelegate;

class SSHTunnelWorkerThread : public QThread
{
  Q_OBJECT
public:
  using CallbackType = SSHTunnel::CallbackType;
  using CallbackInfo = SSHTunnel::CallbackInfo;

  /**
   * The auth delegate is used from this worker thread to ask for passwords /
   * passphrases; it must marshal any UI to the GUI thread (QtSSHAuthDelegate
   * does) and outlive the thread.
   */
  explicit SSHTunnelWorkerThread(QObject                 *parent,
                                 QString                  hostname,
                                 int                      remote_port,
                                 QString                  username,
                                 QString                  keyfile,
                                 AbstractSSHAuthDelegate *auth)
    : QThread(parent)
    , m_Hostname(hostname)
    , m_SSHUserName(username)
    , m_SSHPrivateKeyFile(keyfile)
    , m_RemotePort(remote_port)
    , m_AuthDelegate(auth)
  {}

  virtual ~SSHTunnelWorkerThread();

signals:
  void tunnelReady(int local_port);
  void tunnelError(QString message);

public slots:

  void terminate();

protected:
  void run() override;

protected:
  SSHTunnel::CallbackResponse callback(CallbackType ctype, CallbackInfo info);
  static SSHTunnel::CallbackResponse static_callback(CallbackType type, CallbackInfo info, void *data);

  QString m_Hostname, m_SSHUserName, m_SSHPrivateKeyFile;
  int m_RemotePort;
  AbstractSSHAuthDelegate *m_AuthDelegate;

  bool m_Terminate = false;
  QMutex m_TerminateMutex;
};

#endif // SSHTUNNELWORKERTHREAD_H
