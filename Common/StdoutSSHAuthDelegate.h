#pragma once
#include "AbstractSSHAuthDelegate.h"

/**
 * AbstractSSHAuthDelegate implementation that prompts for credentials on
 * the terminal.  Passwords and passphrases are read with echo disabled via
 * termios (POSIX) or SetConsoleMode (Windows).  EOF (Ctrl-D / Ctrl-Z)
 * cancels the prompt and causes the method to return false.
 */
class StdoutSSHAuthDelegate : public AbstractSSHAuthDelegate
{
public:
  bool PromptForPassword(const std::string &host,
                         const std::string &username,
                         const std::string &prompt,
                         std::string       &password) override;

  bool PromptForPassphrase(const std::string &keyfile,
                           const std::string &prompt,
                           std::string       &passphrase) override;

  bool PromptForUsernameAndPassword(const std::string &host,
                                    const std::string &prompt,
                                    std::string       &username,
                                    std::string       &password) override;

private:
  // Read one line from stdin, with or without terminal echo.
  // Returns false on EOF / Ctrl-D (cancellation).
  static bool readLine(const char *label, std::string &out, bool echo);
};
