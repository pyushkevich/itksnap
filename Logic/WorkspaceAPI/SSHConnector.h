#ifndef SSHCONNECTOR_H
#define SSHCONNECTOR_H

#include <libssh/libssh.h>
#include <libssh/callbacks.h>
#include <map>
#include <memory>
#include <set>
#include <string>

/** Where and how to connect. Anything left empty is filled from ~/.ssh/config. */
struct SSHConnectParams
{
  std::string host;
  std::string username; // explicit username (e.g. from a URL); may be empty
  std::string keyfile;  // extra identity file to try; may be empty
  int         port = 0; // 0 = use ~/.ssh/config / default
  std::string config_file; // SSH config file to use instead of ~/.ssh/config;
                           // empty = SSHConnector::GetDefaultConfigFile()
  long        timeout_sec = 10;
  bool        verbose = false;
};

/**
 * Secrets supplied by the caller for one SSHConnector::Attempt(). Jump-host
 * secrets are keyed by the jump host name reported by GetPromptHost().
 */
struct SSHCredentials
{
  std::string                        username; // overrides SSHConnectParams::username
  std::string                        password;
  std::string                        passphrase;
  bool                               passphrase_declined = false;
  std::map<std::string, std::string> jump_passwords;
  std::map<std::string, std::string> jump_passphrases;
  std::set<std::string>              jump_passphrases_declined;

  /** Overwrite all secrets before the strings are released */
  void Clear();
};

/**
 * Non-interactive SSH connection and authentication, including ProxyJump hosts
 * from ~/.ssh/config.
 *
 * This class never prompts. Each call to Attempt() uses the credentials it is
 * given (plus ssh-agent and key files) and, if that is not enough, reports
 * which secret is missing for which host. The caller obtains the secret by
 * whatever means it likes and calls Attempt() again. OpenSSHSession() in
 * SSHLogin.h implements that loop on top of AbstractSSHAuthDelegate.
 *
 * Keeping prompts out of here matters: libssh authenticates jump hosts from a
 * detached thread of its own while ssh_connect() blocks the calling thread, so
 * a prompt issued from inside libssh deadlocks against a GUI thread that is
 * blocked in ssh_connect(), and can outlive the ssh_connect() that started it.
 */
class SSHConnector
{
public:
  enum Status
  {
    OK,
    NEED_PASSWORD,        // password for GetPromptUser()@GetPromptHost()
    NEED_PASSPHRASE,      // passphrase for key GetPromptKey()
    NEED_JUMP_PASSWORD,   // as above, for jump host GetPromptHost()
    NEED_JUMP_PASSPHRASE, // as above, for jump host GetPromptHost()
    CONNECT_ERROR         // fatal; see GetError()
  };

  explicit SSHConnector(const SSHConnectParams &params);
  ~SSHConnector();

  SSHConnector(const SSHConnector &) = delete;
  SSHConnector &operator=(const SSHConnector &) = delete;

  /** Connect (or reconnect, if needed) and try to authenticate */
  Status Attempt(const SSHCredentials &creds);

  /** Host / user / key that the last non-OK status refers to */
  const std::string &GetPromptHost() const { return m_PromptHost; }
  const std::string &GetPromptUser() const { return m_PromptUser; }
  const std::string &GetPromptKey() const { return m_PromptKey; }

  /** Error from the last attempt, suitable for showing next to a prompt */
  const std::string &GetError() const { return m_Error; }

  /** Whether the username was given explicitly or by ~/.ssh/config */
  bool HasConfiguredUsername() const { return m_HasConfiguredUsername; }

  /** Hand over the authenticated session; the caller must disconnect/free it */
  ssh_session Release();

  /**
   * Process-wide SSH config file used in place of ~/.ssh/config whenever
   * SSHConnectParams::config_file is empty (empty = ~/.ssh/config). Intended
   * for testing (ITK-SNAP --ssh-config); set it once at startup, before any
   * connections are made.
   */
  static void               SetDefaultConfigFile(const std::string &file);
  static const std::string &GetDefaultConfigFile();

  /** State for libssh's passphrase (auth_function) callback */
  struct PassphraseState
  {
    std::string value;
    bool        requested = false; // a key needed a passphrase we did not have
    bool        used = false;      // we supplied a passphrase to libssh
  };

  /** Per-hop state shared with libssh's jump thread; defined in the .cxx */
  struct JumpBlock;

private:
  SSHConnectParams m_Params;
  ssh_session      m_Session = nullptr;
  std::string      m_ConnectedUser;
  bool             m_NeedReconnect = false;
  bool             m_HasConfiguredUsername = false;

  // Read by libssh's detached jump thread; see DisposeJumpBlock()
  std::unique_ptr<JumpBlock> m_JumpBlock;

  // Passphrase callback for the target host (libssh keeps a pointer to it)
  PassphraseState      m_PassphraseState;
  ssh_callbacks_struct m_SessionCallbacks;

  // Key auth is only repeated on the same connection when a new passphrase
  // is supplied, so that password retries do not use up MaxAuthTries
  bool        m_KeysTried = false;
  bool        m_KeyNeedsPassphrase = false;
  bool        m_PassphraseRejected = false;
  std::string m_LastPassphraseTried;

  std::string m_PromptHost, m_PromptUser, m_PromptKey, m_Error;

  Status Connect(const SSHCredentials &creds, const std::string &user);
  Status Authenticate(const SSHCredentials &creds);
  void   Disconnect();
  void   DisposeJumpBlock(bool connected);
};

#endif // SSHCONNECTOR_H
