#include "SSHLogin.h"
#include "AbstractSSHAuthDelegate.h"
#include "IRISException.h"

ssh_session
OpenSSHSession(const SSHConnectParams &params, AbstractSSHAuthDelegate *auth)
{
  SSHConnector   connector(params);
  SSHCredentials creds;

  auto cancel = [&](const char *what) -> IRISException {
    creds.Clear();
    return IRISException("SSH connection to %s cancelled: %s", params.host.c_str(), what);
  };

  while (true)
  {
    SSHConnector::Status status = connector.Attempt(creds);
    if (status == SSHConnector::OK)
    {
      creds.Clear();
      return connector.Release();
    }

    if (status == SSHConnector::CONNECT_ERROR)
    {
      creds.Clear();
      throw IRISException("SSH connection to %s failed: %s",
                          params.host.c_str(), connector.GetError().c_str());
    }

    if (!auth)
    {
      creds.Clear();
      throw IRISException("SSH authentication to %s failed: no key accepted and "
                          "interactive login is not available",
                          params.host.c_str());
    }

    const std::string &host = connector.GetPromptHost();
    const std::string &error = connector.GetError();

    switch (status)
    {
      case SSHConnector::NEED_HOST_KEY_CONFIRM:
        if (!auth->ConfirmHostKey(
              host, connector.GetHostKeyType(), connector.GetHostKeyFingerprint()))
          throw cancel("host key not trusted");
        creds.accepted_host_key = connector.GetHostKeyFingerprint();
        break;

      case SSHConnector::NEED_PASSWORD:
        if (!connector.HasConfiguredUsername() && creds.username.empty())
        {
          if (!auth->PromptForUsernameAndPassword(host, error, creds.username, creds.password))
            throw cancel("no credentials entered");
        }
        else if (!auth->PromptForPassword(host, connector.GetPromptUser(), error, creds.password))
          throw cancel("no password entered");
        break;

      case SSHConnector::NEED_PASSPHRASE:
        // Declining (or leaving empty) a passphrase is not fatal - it skips
        // the key and falls back to password login
        if (!auth->PromptForPassphrase(connector.GetPromptKey(), error, creds.passphrase) ||
            creds.passphrase.empty())
        {
          creds.passphrase.clear();
          creds.passphrase_declined = true;
        }
        break;

      case SSHConnector::NEED_JUMP_PASSWORD:
        if (!auth->PromptForPassword(
              host, connector.GetPromptUser(), error, creds.jump_passwords[host]))
          throw cancel("no password entered for jump host");
        break;

      case SSHConnector::NEED_JUMP_PASSPHRASE:
        if (!auth->PromptForPassphrase(connector.GetPromptKey() + " (jump host " + host + ")",
                                       error,
                                       creds.jump_passphrases[host]) ||
            creds.jump_passphrases[host].empty())
        {
          creds.jump_passphrases.erase(host);
          creds.jump_passphrases_declined.insert(host);
        }
        break;

      default:
        break;
    }
  }
}
