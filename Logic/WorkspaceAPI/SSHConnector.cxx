#include "SSHConnector.h"

#include <algorithm>
#include <atomic>
#include <cstring>
#include <iostream>
#include <mutex>

#ifdef _WIN32
#  include <winsock2.h>
#  define SSHCONNECTOR_SHUT_RDWR SD_BOTH
#else
#  include <sys/socket.h>
#  define SSHCONNECTOR_SHUT_RDWR SHUT_RDWR
#endif

// libssh connects through ProxyJump hosts itself, calling back into the
// application for each hop, only from version 0.11. Older versions turn a
// ProxyJump directive into a ProxyCommand that runs the system ssh binary,
// so jump hosts still work, but there is nothing for us to hook into.
#if LIBSSH_VERSION_INT >= SSH_VERSION_INT(0, 11, 0)
#  define SSHCONNECTOR_PROXYJUMP_CALLBACKS 1
#else
#  define SSHCONNECTOR_PROXYJUMP_CALLBACKS 0
#endif

namespace
{

// Read a string option from a session; empty if unset
std::string
GetOption(ssh_session session, ssh_options_e type)
{
  std::string result;
  char       *value = nullptr;
  if (ssh_options_get(session, type, &value) == SSH_OK && value)
  {
    result = value;
    ssh_string_free_char(value);
  }
  return result;
}

// The local account name, which libssh uses when no username is given
std::string
LocalUsername()
{
  ssh_session tmp = ssh_new();
  if (!tmp)
    return std::string();
  ssh_options_set(tmp, SSH_OPTIONS_USER, nullptr);
  std::string user = GetOption(tmp, SSH_OPTIONS_USER);
  ssh_free(tmp);
  return user;
}

// Apply the SSH config file (empty = ~/.ssh/config plus the system one)
void
ParseConfig(ssh_session session, const std::string &config_file)
{
  ssh_options_parse_config(session, config_file.empty() ? nullptr : config_file.c_str());
}

// The "User" that the SSH config assigns to host alias @p host, if any
std::string
ConfiguredUsername(const std::string &host, const std::string &config_file)
{
  ssh_session tmp = ssh_new();
  if (!tmp)
    return std::string();
  ssh_options_set(tmp, SSH_OPTIONS_HOST, host.c_str());
  ParseConfig(tmp, config_file);
  std::string user = GetOption(tmp, SSH_OPTIONS_USER);
  ssh_free(tmp);
  return user;
}

// libssh reports a key it could not unlock as "Failed to read private key: <file>"
std::string
KeyFileFromError(const std::string &error)
{
  static const std::string prefix = "Failed to read private key: ";
  auto pos = error.find(prefix);
  return pos == std::string::npos ? std::string() : error.substr(pos + prefix.size());
}

// libssh asks for key passphrases through this callback. It never prompts:
// it hands over the passphrase supplied by the caller, or records that one is
// needed and declines, in which case libssh moves on to the next key.
int
PassphraseCallback(const char *, char *buf, size_t len, int, int, void *userdata)
{
  auto *state = static_cast<SSHConnector::PassphraseState *>(userdata);
  if (state->value.empty() || state->value.size() >= len)
  {
    state->requested = true;
    return SSH_ERROR;
  }
  memcpy(buf, state->value.c_str(), state->value.size() + 1);
  state->used = true;
  return SSH_OK;
}

void
InitPassphraseCallbacks(ssh_callbacks_struct &cb, SSHConnector::PassphraseState *state)
{
  memset(&cb, 0, sizeof(cb));
  ssh_callbacks_init(&cb);
  cb.userdata = state;
  cb.auth_function = PassphraseCallback;
}

// Installed in place of our callbacks once authentication is over, so that
// libssh does not keep pointers into memory we are about to free
ssh_callbacks
EmptyCallbacks()
{
  static ssh_callbacks_struct empty = [] {
    ssh_callbacks_struct cb;
    memset(&cb, 0, sizeof(cb));
    ssh_callbacks_init(&cb);
    return cb;
  }();
  return &empty;
}

} // anonymous namespace


void
SSHCredentials::Clear()
{
  auto wipe = [](std::string &s) { std::fill(s.begin(), s.end(), '\0'); s.clear(); };
  wipe(password);
  wipe(passphrase);
  for (auto &kv : jump_passwords)
    wipe(kv.second);
  for (auto &kv : jump_passphrases)
    wipe(kv.second);
  jump_passwords.clear();
  jump_passphrases.clear();
}


/**
 * ProxyJump support. libssh connects to each jump host from a detached thread
 * of its own, taking one ssh_jump_callbacks_struct per hop from a list that
 * stores our pointers (not copies). Everything those callbacks touch lives
 * here, never on a caller's stack, and the callbacks only use the secrets
 * copied in before ssh_connect(); they never block on the user.
 */
struct SSHConnector::JumpBlock
{
  // Callback structs supplied per connection; ProxyJump chains longer than
  // this fall back to libssh's default (agent / key file) authentication
  static constexpr int MAX_HOPS = 4;

  struct Hop
  {
    JumpBlock                *block = nullptr;
#if SSHCONNECTOR_PROXYJUMP_CALLBACKS
    ssh_jump_callbacks_struct jump_cb{};
#endif
    ssh_callbacks_struct      session_cb{};
    PassphraseState           passphrase;

    // Written by the jump thread; read by the caller only once stage == 2
    std::string host; // as named by ProxyJump (an alias, possibly)
    std::string user;
    std::string key_hint;
    std::string error;
    Status      need = OK;

    // 0 = not used, 1 = jump thread inside our callbacks, 2 = finished with us
    std::atomic<int> stage{ 0 };
  };

  Hop                                hops[MAX_HOPS];
  std::map<std::string, std::string> passwords, passphrases;
  std::set<std::string>              passphrases_declined;
  std::string                        config_file;
  bool                               verbose = false;

  // The target session, while its ssh_connect() is still running; guarded by
  // mutex, and reset by the caller as soon as ssh_connect() returns
  std::mutex  mutex;
  ssh_session outer = nullptr;

  static int BeforeConnection(ssh_session jump_session, void *userdata);
  static int VerifyKnownHost(ssh_session jump_session, void *userdata);
  static int Authenticate(ssh_session jump_session, void *userdata);

  // Record the outcome for the caller. Must be the hop's last access to the
  // block, since the block may be freed as soon as the caller sees stage 2.
  static int Finish(Hop *hop, Status need, const std::string &error)
  {
    std::lock_guard<std::mutex> lock(hop->block->mutex);
    hop->need = need;
    hop->error = error;
    hop->stage.store(2, std::memory_order_release);

    // When a hop fails, libssh's jump thread exits without closing its end of
    // the socket pair that feeds the target session, so the target's
    // ssh_connect() would sit out its full timeout before failing. Shut the
    // target's end down so that it fails (and we can prompt) right away.
    if (need != OK && hop->block->outer)
    {
      socket_t fd = ssh_get_fd(hop->block->outer);
      if (fd != SSH_INVALID_SOCKET)
        shutdown(fd, SSHCONNECTOR_SHUT_RDWR);
    }
    return need == OK ? SSH_OK : SSH_ERROR;
  }
};

int
SSHConnector::JumpBlock::BeforeConnection(ssh_session jump_session, void *userdata)
{
  auto *hop = static_cast<Hop *>(userdata);
  hop->stage.store(1, std::memory_order_release);

  hop->host = GetOption(jump_session, SSH_OPTIONS_HOST);

  // When ProxyJump does not name a user, libssh sets the jump session's user
  // to the local account, which stops a "User" line in that host's
  // ~/.ssh/config entry from applying. Restore OpenSSH's behavior. (A jump
  // spec that explicitly names the local account is indistinguishable, and
  // gets the configured user instead - an acceptable corner case.)
  std::string cfg_user = ConfiguredUsername(hop->host, hop->block->config_file);
  if (!cfg_user.empty() && GetOption(jump_session, SSH_OPTIONS_USER) == LocalUsername())
    ssh_options_set(jump_session, SSH_OPTIONS_USER, cfg_user.c_str());

  // Apply the jump host's own ~/.ssh/config entry (HostName, Port,
  // IdentityFile, ...). libssh would do this inside ssh_connect() anyway; doing
  // it here makes the order explicit relative to the user fix above.
  ParseConfig(jump_session, hop->block->config_file);
  hop->user = GetOption(jump_session, SSH_OPTIONS_USER);

  if (hop->block->verbose)
    std::cout << "Connecting to jump host " << hop->user << "@" << hop->host << std::endl;
  return SSH_OK;
}

int
SSHConnector::JumpBlock::VerifyKnownHost(ssh_session jump_session, void *userdata)
{
  // Same policy as libssh's default for jump hosts (only known hosts are
  // accepted), but with an error message that tells the user what to do
  auto *hop = static_cast<Hop *>(userdata);
  if (ssh_session_is_known_server(jump_session) == SSH_KNOWN_HOSTS_OK)
    return SSH_OK;

  return Finish(hop,
                CONNECT_ERROR,
                "The host key of jump host " + hop->host +
                  " is unknown or has changed. Connect to it once with the ssh command to "
                  "verify and record its key in ~/.ssh/known_hosts.");
}

int
SSHConnector::JumpBlock::Authenticate(ssh_session jump_session, void *userdata)
{
  auto      *hop = static_cast<Hop *>(userdata);
  JumpBlock *block = hop->block;

  auto it_pp = block->passphrases.find(hop->host);
  hop->passphrase.value = it_pp != block->passphrases.end() ? it_pp->second : std::string();
  bool declined = block->passphrases_declined.count(hop->host) > 0;

  // Agent and key files first, with passphrases supplied non-interactively
  InitPassphraseCallbacks(hop->session_cb, &hop->passphrase);
  ssh_set_callbacks(jump_session, &hop->session_cb);
  int rc = ssh_userauth_publickey_auto(jump_session, nullptr, nullptr);
  ssh_set_callbacks(jump_session, EmptyCallbacks());

  if (rc == SSH_AUTH_SUCCESS)
  {
    if (block->verbose)
      std::cout << "Authenticated to jump host " << hop->host << " using a key" << std::endl;
    return Finish(hop, OK, std::string());
  }
  hop->key_hint = KeyFileFromError(ssh_get_error(jump_session));

  // A password is only on hand once the user has been asked for one
  auto it_pw = block->passwords.find(hop->host);
  if (it_pw != block->passwords.end())
  {
    if (ssh_userauth_password(jump_session, nullptr, it_pw->second.c_str()) == SSH_AUTH_SUCCESS)
    {
      if (block->verbose)
        std::cout << "Authenticated to jump host " << hop->host << " using password" << std::endl;
      return Finish(hop, OK, std::string());
    }
    return Finish(hop, NEED_PASSWORD, ssh_get_error(jump_session));
  }

  if (!declined && hop->passphrase.used)
    return Finish(hop, NEED_PASSPHRASE, "Incorrect passphrase");
  if (!declined && hop->passphrase.requested)
    return Finish(hop, NEED_PASSPHRASE, std::string());
  return Finish(hop, NEED_PASSWORD, std::string());
}


namespace
{
std::string &
DefaultConfigFile()
{
  static std::string file;
  return file;
}
} // anonymous namespace

void
SSHConnector::SetDefaultConfigFile(const std::string &file)
{
  DefaultConfigFile() = file;
}

const std::string &
SSHConnector::GetDefaultConfigFile()
{
  return DefaultConfigFile();
}

SSHConnector::SSHConnector(const SSHConnectParams &params)
  : m_Params(params)
{
  if (m_Params.config_file.empty())
    m_Params.config_file = GetDefaultConfigFile();
  ssh_init();
  InitPassphraseCallbacks(m_SessionCallbacks, &m_PassphraseState);
}

SSHConnector::~SSHConnector()
{
  Disconnect();
  std::fill(m_PassphraseState.value.begin(), m_PassphraseState.value.end(), '\0');
  std::fill(m_LastPassphraseTried.begin(), m_LastPassphraseTried.end(), '\0');
}

SSHConnector::Status
SSHConnector::Attempt(const SSHCredentials &creds)
{
  std::string user = creds.username.empty() ? m_Params.username : creds.username;

  // OpenSSH servers drop the connection if the username changes between
  // authentication attempts, so a new username means a new connection
  if (m_Session && (m_NeedReconnect || (!user.empty() && user != m_ConnectedUser)))
    Disconnect();

  if (!m_Session)
  {
    Status status = Connect(creds, user);
    if (status != OK)
      return status;
  }

  return Authenticate(creds);
}

SSHConnector::Status
SSHConnector::Connect(const SSHCredentials &creds, const std::string &user)
{
  m_PromptHost = m_Params.host;
  m_PromptUser = user;
  m_PromptKey.clear();
  m_Error.clear();

  if (m_Params.verbose)
    std::cout << "Creating SSH session to " << m_Params.host << std::endl;

  m_Session = ssh_new();
  if (!m_Session)
  {
    m_Error = "Error creating SSH session";
    return CONNECT_ERROR;
  }

  auto fail = [this](const std::string &what) {
    m_Error = what + ": " + ssh_get_error(m_Session);
    Disconnect();
    return CONNECT_ERROR;
  };

  if (ssh_options_set(m_Session, SSH_OPTIONS_HOST, m_Params.host.c_str()) != SSH_OK)
    return fail("Error setting SSH hostname to " + m_Params.host);

  if (!user.empty() && ssh_options_set(m_Session, SSH_OPTIONS_USER, user.c_str()) != SSH_OK)
    return fail("Error setting SSH username to " + user);

  if (!m_Params.keyfile.empty() &&
      ssh_options_set(m_Session, SSH_OPTIONS_ADD_IDENTITY, m_Params.keyfile.c_str()) != SSH_OK)
    return fail("Error setting SSH identity file to " + m_Params.keyfile);

  // Apply ~/.ssh/config (fills in User, IdentityFile, Port, ProxyJump, etc.
  // not already set explicitly)
  ParseConfig(m_Session, m_Params.config_file);

  // An explicit port overrides whatever ~/.ssh/config may have set
  if (m_Params.port > 0)
  {
    unsigned int uport = static_cast<unsigned int>(m_Params.port);
    if (ssh_options_set(m_Session, SSH_OPTIONS_PORT, &uport) != SSH_OK)
      return fail("Error setting SSH port to " + std::to_string(m_Params.port));
  }

  long timeout = m_Params.timeout_sec;
  ssh_options_set(m_Session, SSH_OPTIONS_TIMEOUT, &timeout);

  // libssh fills in the local account name during ssh_connect(), so check now
  // whether a username was given explicitly or by ~/.ssh/config
  m_HasConfiguredUsername = !GetOption(m_Session, SSH_OPTIONS_USER).empty();

  // Callbacks for ProxyJump hosts, if ~/.ssh/config specifies any. Unused
  // entries are harmless: libssh only ever pops one per hop.
  m_JumpBlock.reset(new JumpBlock());
  m_JumpBlock->passwords = creds.jump_passwords;
  m_JumpBlock->passphrases = creds.jump_passphrases;
  m_JumpBlock->passphrases_declined = creds.jump_passphrases_declined;
  m_JumpBlock->config_file = m_Params.config_file;
  m_JumpBlock->verbose = m_Params.verbose;
  m_JumpBlock->outer = m_Session;
  for (auto &hop : m_JumpBlock->hops)
  {
    hop.block = m_JumpBlock.get();
#if SSHCONNECTOR_PROXYJUMP_CALLBACKS
    hop.jump_cb.userdata = &hop;
    hop.jump_cb.before_connection = &JumpBlock::BeforeConnection;
    hop.jump_cb.verify_knownhost = &JumpBlock::VerifyKnownHost;
    hop.jump_cb.authenticate = &JumpBlock::Authenticate;
    ssh_options_set(m_Session, SSH_OPTIONS_PROXYJUMP_CB_LIST_APPEND, &hop.jump_cb);
#endif
  }

  int rc_connect = ssh_connect(m_Session);
  {
    // Jump callbacks must not touch the session from here on
    std::lock_guard<std::mutex> lock(m_JumpBlock->mutex);
    m_JumpBlock->outer = nullptr;
  }

  if (rc_connect != SSH_OK)
  {
    m_Error = ssh_get_error(m_Session);
    Status status = CONNECT_ERROR;

    // If a jump host turned us away, report that instead of the less
    // specific error from the target connection
    for (auto &hop : m_JumpBlock->hops)
    {
      if (hop.stage.load(std::memory_order_acquire) == 2 && hop.need != OK)
      {
        m_PromptHost = hop.host;
        m_PromptUser = hop.user;
        m_PromptKey = hop.key_hint.empty() ? "SSH private key" : hop.key_hint;
        if (!hop.error.empty() || hop.need == CONNECT_ERROR)
          m_Error = hop.error;
        else
          m_Error.clear();
        status = hop.need == NEED_PASSWORD     ? NEED_JUMP_PASSWORD
                 : hop.need == NEED_PASSPHRASE ? NEED_JUMP_PASSPHRASE
                                               : CONNECT_ERROR;
        break;
      }
    }

    Disconnect();
    return status;
  }

  DisposeJumpBlock(true);

  m_ConnectedUser = GetOption(m_Session, SSH_OPTIONS_USER);
  m_PromptUser = m_ConnectedUser;
  m_NeedReconnect = false;
  m_KeysTried = false;
  m_KeyNeedsPassphrase = false;
  m_PassphraseRejected = false;
  m_LastPassphraseTried.clear();
  return OK;
}

SSHConnector::Status
SSHConnector::Authenticate(const SSHCredentials &creds)
{
  m_PromptHost = m_Params.host;
  m_PromptUser = m_ConnectedUser;
  m_Error.clear();

  // Agent and key files; repeated only if there is a new passphrase to try
  bool new_passphrase = !creds.passphrase.empty() && creds.passphrase != m_LastPassphraseTried;
  if (!m_KeysTried || new_passphrase)
  {
    m_PassphraseState.value = creds.passphrase;
    m_PassphraseState.requested = false;
    m_PassphraseState.used = false;

    ssh_set_callbacks(m_Session, &m_SessionCallbacks);
    int rc = ssh_userauth_publickey_auto(m_Session, nullptr, nullptr);
    ssh_set_callbacks(m_Session, EmptyCallbacks());

    m_KeysTried = true;
    m_LastPassphraseTried = creds.passphrase;
    if (rc == SSH_AUTH_SUCCESS)
    {
      if (m_Params.verbose)
        std::cout << "Authenticated using a key" << std::endl;
      return OK;
    }

    std::string hint = KeyFileFromError(ssh_get_error(m_Session));
    m_PromptKey = !hint.empty()                ? hint
                  : !m_Params.keyfile.empty() ? m_Params.keyfile
                                              : std::string("SSH private key");
    m_KeyNeedsPassphrase = m_PassphraseState.requested;
    m_PassphraseRejected = m_PassphraseState.used;
    if (!ssh_is_connected(m_Session))
      m_NeedReconnect = true;
  }

  if (!creds.password.empty() && !m_NeedReconnect)
  {
    int rc = ssh_userauth_password(m_Session, nullptr, creds.password.c_str());
    if (rc == SSH_AUTH_SUCCESS)
    {
      if (m_Params.verbose)
        std::cout << "Authenticated using password" << std::endl;
      return OK;
    }

    // SSH_AUTH_ERROR typically means the server gave up on this connection
    // (e.g. too many failures); the next attempt starts a new one
    m_Error = ssh_get_error(m_Session);
    if (rc == SSH_AUTH_ERROR || !ssh_is_connected(m_Session))
      m_NeedReconnect = true;
    return NEED_PASSWORD;
  }

  if (!creds.passphrase_declined && m_PassphraseRejected)
  {
    m_Error = "Incorrect passphrase";
    return NEED_PASSPHRASE;
  }
  if (!creds.passphrase_declined && m_KeyNeedsPassphrase)
    return NEED_PASSPHRASE;

  return NEED_PASSWORD;
}

ssh_session
SSHConnector::Release()
{
  ssh_session session = m_Session;
  if (session)
    ssh_set_callbacks(session, EmptyCallbacks());
  m_Session = nullptr;
  return session;
}

void
SSHConnector::Disconnect()
{
  DisposeJumpBlock(false);
  if (m_Session)
  {
    if (ssh_is_connected(m_Session))
      ssh_disconnect(m_Session);
    ssh_free(m_Session);
    m_Session = nullptr;
  }
}

void
SSHConnector::DisposeJumpBlock(bool connected)
{
  if (!m_JumpBlock)
    return;

  // Once ssh_connect() succeeds, every hop has finished authenticating (the
  // target's banner can only arrive through fully established hops), so the
  // block is no longer referenced. After a failure (e.g. timeout), a hop may
  // still be inside our callbacks on libssh's detached jump thread, which
  // nobody can wait for; in that case the block is deliberately leaked
  // rather than freed under that thread's feet.
  bool in_flight = false;
  if (!connected)
    for (auto &hop : m_JumpBlock->hops)
      if (hop.stage.load(std::memory_order_acquire) == 1)
        in_flight = true;

  if (in_flight)
    m_JumpBlock.release();
  else
    m_JumpBlock.reset();
}
