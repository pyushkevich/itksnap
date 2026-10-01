#pragma once

#include "AbstractSSHAuthDelegate.h"
#include <QObject>
#include <functional>

/**
 * Qt implementation of AbstractSSHAuthDelegate.
 * Uses QInputDialog to prompt the user for SSH credentials.
 * Must be created on the main thread.
 *
 * Prompt* methods may be called from a worker thread - e.g. the DLS SSH
 * tunnel logs in from SSHTunnelWorkerThread. Qt widgets can only be
 * created/shown on the thread that owns them (the main thread here), so every
 * prompt is routed through RunOnMainThread(), which blocks the calling thread
 * until the dialog (run on the main thread) returns a result. The main thread
 * must not itself be blocked waiting on that worker.
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

  bool PromptForAPIKey(const std::string &server,
                       const std::string &prompt,
                       std::string       &api_key) override;

private:
  QWidget *m_Parent;

  // Runs fn() on the main (GUI) thread, blocking the calling thread until it
  // completes. If already called from the main thread, runs fn() directly.
  void RunOnMainThread(const std::function<void()> &fn);
};
