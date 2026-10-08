#pragma once
#include <string>

/**
 * Delegate interface for interactive SSH authentication prompts.
 *
 * Implement this in the GUI layer (QtSSHAuthDelegate) and inject it into
 * IRISApplication so that the Logic layer can request credentials without
 * taking a direct Qt dependency.
 *
 * All methods block until the user responds or cancels.  They are called by
 * OpenSSHSession() (SSHLogin.h) between connection attempts, on whatever
 * thread is opening the session: the main thread for remote image downloads,
 * a worker thread for the DLS SSH tunnel.  GUI implementations must therefore
 * marshal their dialogs to the GUI thread (see QtSSHAuthDelegate).  They are
 * never called from inside libssh callbacks.
 */
class AbstractSSHAuthDelegate
{
public:
  virtual ~AbstractSSHAuthDelegate() = default;

  /**
   * Prompt for a login password.
   * @param host     Remote hostname
   * @param username Login username (may be empty if not known yet)
   * @param prompt   Error message from the previous failed attempt, or empty
   *                 on the first try
   * @param password [out] Password entered by the user
   * @return true if the user supplied credentials, false if they cancelled
   */
  virtual bool PromptForPassword(const std::string &host,
                                 const std::string &username,
                                 const std::string &prompt,
                                 std::string       &password) = 0;

  /**
   * Prompt for an SSH private-key passphrase.
   * @param keyfile  Path to the key file requiring the passphrase
   * @param prompt   Error message from the previous failed attempt, or empty
   *                 on the first try
   * @param passphrase [out] Passphrase entered by the user
   * @return true if the user supplied credentials, false if they cancelled
   */
  virtual bool PromptForPassphrase(const std::string &keyfile,
                                   const std::string &prompt,
                                   std::string       &passphrase) = 0;

  /**
   * Prompt for both a username and password when neither is known from
   * the URL or SSH config.
   * @param host     Remote hostname
   * @param prompt   Error message from the previous failed attempt, or empty
   * @param username [out] Username entered by the user
   * @param password [out] Password entered by the user
   * @return true if the user supplied credentials, false if they cancelled
   */
  virtual bool PromptForUsernameAndPassword(const std::string &host,
                                            const std::string &prompt,
                                            std::string       &username,
                                            std::string       &password) = 0;

  /**
   * Ask whether to trust a host whose key is not yet in ~/.ssh/known_hosts,
   * like the ssh command does on first connection. If the user agrees, the key
   * is added to known_hosts and the connection proceeds.
   * @param host        Remote hostname
   * @param key_type    Key type, e.g. "ssh-ed25519"
   * @param fingerprint Key fingerprint, e.g. "SHA256:..."
   * @return true to trust the key and connect, false to abort
   */
  virtual bool ConfirmHostKey(const std::string &host,
                              const std::string &key_type,
                              const std::string &fingerprint) = 0;

  /**
   * Prompt for a Flywheel (or other REST) API key.
   * @param server   Hostname of the remote server
   * @param prompt   Error message from the previous failed attempt, or empty
   * @param api_key  [out] Key entered by the user
   * @return true if the user supplied a key, false if they cancelled
   *
   * The default implementation delegates to PromptForPassword so that
   * subclasses that only implement the SSH interface still work.
   */
  virtual bool PromptForAPIKey(const std::string &server,
                               const std::string &prompt,
                               std::string       &api_key)
  {
    return PromptForPassword(server, "api-key", prompt, api_key);
  }
};
