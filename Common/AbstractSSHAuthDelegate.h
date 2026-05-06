#pragma once
#include <string>

/**
 * Delegate interface for interactive SSH authentication prompts.
 *
 * Implement this in the GUI layer (QtSSHAuthDelegate) and inject it into
 * IRISApplication so that the Logic layer can request credentials without
 * taking a direct Qt dependency.
 *
 * All methods block until the user responds or cancels.  They are called
 * from whatever thread invokes RemoteImageSource::Download() — currently
 * always the main thread, so a blocking Qt dialog is safe.
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
};
