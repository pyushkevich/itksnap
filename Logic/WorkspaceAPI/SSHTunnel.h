#ifndef SSHTUNNEL_H
#define SSHTUNNEL_H

#include <libssh/libssh.h>
#include <string>
#include <map>
#include <variant>

/**
 * A streamlined SSH tunnel that uses just a single static function to run a ssh
 * tunnel in a loop. All communication with the caller is done using a callback
 * function, which can be used to report errors to the user and to terminate the
 * tunnel. The SSH session itself is opened (and authenticated) by the caller,
 * typically with OpenSSHSession() from SSHLogin.h.
 */
class SSHTunnel
{
public:

  enum CallbackType
  {
    CB_ERROR,               // Error
    CB_WARNING,             // Warning
    CB_READY,               // Tunnel is ready, returns bound hostname/port
    CB_TERMINATION_CHECK    // Check whether the tunnel should be terminated
  };

  static constexpr int RC_USER_ABORT = 0;
  static constexpr int RC_SSH_ERROR = -1;
  static constexpr int RC_SOCKET_ERROR = -2;

  struct ErrorInfo
  {
    std::string error_message;
  };

  struct ReadyInfo
  {
    std::string hostname;
    int local_port;
  };

  // Union of different info structures
  using CallbackInfo = std::variant<ErrorInfo, ReadyInfo>;

  /** A response from the callback - integer code and optional text response */
  using CallbackResponse = std::pair<int, std::string>;
  using Callback = CallbackResponse (*) (CallbackType, CallbackInfo, void *);

  /**
   * Run an SSH tunnel over an already authenticated @p session, forwarding a
   * local port to @p remote_host:@p remote_port as seen from the SSH server.
   * With @p remote_host == nullptr, forwards to the SSH server itself (using
   * its HostName from ~/.ssh/config, so that host aliases work).
   * Takes ownership of @p session and frees it on return. Runs until the
   * callback requests termination (CB_TERMINATION_CHECK) or an error occurs.
   */
  static int run(ssh_session session,
                 const char *remote_host,
                 int         remote_port,
                 Callback    callback,
                 void       *callback_data,
                 bool        verbose = false);

protected:
  // static int ssh_error_callback(ssh_session session, Callback callback, void *cdata);
  // static int socket_error_callback(Callback callback, void *cdata);

  class SessionGuard
  {
  public:
    using ChannelMap = std::map<int, ssh_channel>;
    SessionGuard(ssh_session s, bool verbose) : session(s), verbose(verbose) {};
    ~SessionGuard();

    void AddTunnel(int socket_id, ssh_channel channel);
    void RemoveTunnel(int socket_id);
    const ChannelMap &GetTunnels() { return channel_map; }

  protected:
    void CleanupTunnel(int socket_fd, ssh_channel channel);

    ssh_session session;
    bool verbose;
    ChannelMap channel_map;
  };

  struct TunnelConnection
  {
    ssh_channel channel;
    int socket_fd;
  };

};

#endif // SSHTUNNEL_H
