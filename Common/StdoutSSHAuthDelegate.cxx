#include "StdoutSSHAuthDelegate.h"

#include <cstdio>
#include <iostream>
#include <string>

#ifdef _WIN32
#  include <windows.h>
#else
#  include <termios.h>
#  include <unistd.h>
#endif

// Read one line without terminal echo.  Returns false on EOF / cancel.
static bool readNoEcho(std::string &out)
{
#ifdef _WIN32
  HANDLE h = GetStdHandle(STD_INPUT_HANDLE);
  DWORD  mode = 0;
  bool   haveConsole = (GetConsoleMode(h, &mode) != 0);
  if (haveConsole)
    SetConsoleMode(h, mode & ~ENABLE_ECHO_INPUT);
  bool ok = static_cast<bool>(std::getline(std::cin, out));
  if (haveConsole)
    SetConsoleMode(h, mode);
  std::putchar('\n');
  return ok;
#else
  struct termios old_t;
  bool           haveTerminal = (tcgetattr(STDIN_FILENO, &old_t) == 0);
  if (haveTerminal)
  {
    struct termios new_t = old_t;
    new_t.c_lflag &= ~static_cast<tcflag_t>(ECHO);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &new_t);
  }
  bool ok = static_cast<bool>(std::getline(std::cin, out));
  if (haveTerminal)
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &old_t);
  std::putchar('\n');  // echo the newline the terminal suppressed
  std::fflush(stdout);
  return ok;
#endif
}

bool StdoutSSHAuthDelegate::readLine(const char *label, std::string &out, bool echo)
{
  std::cout << label << std::flush;
  if (echo)
    return static_cast<bool>(std::getline(std::cin, out));
  else
    return readNoEcho(out);
}

// -----------------------------------------------------------------------

bool StdoutSSHAuthDelegate::PromptForPassword(const std::string &host,
                                              const std::string &username,
                                              const std::string &prompt,
                                              std::string       &password)
{
  std::cout << "SSH authentication: " << username << "@" << host << "\n";
  if (!prompt.empty())
    std::cout << "  (" << prompt << ")\n";
  return readLine("  Password: ", password, false);
}

bool StdoutSSHAuthDelegate::PromptForPassphrase(const std::string &keyfile,
                                                const std::string &prompt,
                                                std::string       &passphrase)
{
  std::cout << "SSH key passphrase: " << keyfile << "\n";
  if (!prompt.empty())
    std::cout << "  (" << prompt << ")\n";
  return readLine("  Passphrase: ", passphrase, false);
}

bool StdoutSSHAuthDelegate::PromptForUsernameAndPassword(const std::string &host,
                                                         const std::string &prompt,
                                                         std::string       &username,
                                                         std::string       &password)
{
  std::cout << "SSH authentication: " << host << "\n";
  if (!prompt.empty())
    std::cout << "  (" << prompt << ")\n";
  if (!readLine("  Username: ", username, true))
    return false;
  return readLine("  Password: ", password, false);
}
