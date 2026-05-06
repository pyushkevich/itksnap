#pragma once

#include "AbstractSSHAuthDelegate.h"
#include <QObject>

/**
 * Qt implementation of AbstractSSHAuthDelegate.
 * Uses QInputDialog to prompt the user for SSH credentials.
 * Must be created on the main thread.
 */
class QtSSHAuthDelegate : public QObject, public AbstractSSHAuthDelegate
{
public:
  explicit QtSSHAuthDelegate(QWidget *parent = nullptr);

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
  QWidget *m_Parent;
};
