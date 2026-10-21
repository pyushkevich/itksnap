#ifndef SSHLOGIN_H
#define SSHLOGIN_H

#include "SSHConnector.h"

class AbstractSSHAuthDelegate;

/**
 * Open an authenticated SSH session, asking the user for whatever secrets
 * (passwords, key passphrases, for the target or any ProxyJump host) turn out
 * to be needed. This is the one place where SSH code asks for credentials, and
 * it does so only through @p auth, between connection attempts - never from
 * inside libssh.
 *
 * With @p auth == nullptr only non-interactive methods (ssh-agent, key files
 * without passphrase) are used.
 *
 * Returns the session, which the caller must disconnect and free. Throws
 * IRISException with the specific SSH error on failure or if the user cancels.
 */
ssh_session OpenSSHSession(const SSHConnectParams &params, AbstractSSHAuthDelegate *auth);

#endif // SSHLOGIN_H
