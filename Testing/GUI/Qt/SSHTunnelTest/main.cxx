#include <QDebug>
#include <cstring>
#include <iostream>
#include "IRISException.h"
#include "SSHLogin.h"
#include "SSHTunnel.h"
#include "StdoutSSHAuthDelegate.h"

SSHTunnel::CallbackResponse tunnel_callback(SSHTunnel::CallbackType type, SSHTunnel::CallbackInfo info, void *)
{
  switch(type)
  {
    case SSHTunnel::CB_ERROR:
    {
      auto message = std::get<SSHTunnel::ErrorInfo>(info).error_message;
      qCritical() << "ERROR: " << message;
      break;
    }
    case SSHTunnel::CB_WARNING:
    {
      auto message = std::get<SSHTunnel::ErrorInfo>(info).error_message;
      qWarning() << "WARNING: " << message;
      break;
    }
    case SSHTunnel::CB_READY:
    {
      auto ready_info = std::get<SSHTunnel::ReadyInfo>(info);
      qInfo() << "TUNNEL RUNNING ON HOST " << ready_info.hostname << " PORT " << ready_info.local_port;
      break;
    }
    case SSHTunnel::CB_TERMINATION_CHECK:
    {
      break;
    }
  }

  return std::make_pair(0, std::string());
}

int main(int argc, char **argv)
{
  if(argc < 4)
  {
    std::cerr << "Usage: ssh_tunnel_test <hostname> <port> <username> [keyfile] [--login-only] [--config file]" << std::endl;
    std::cerr << "  Logs in to <hostname> (prompting on the terminal for any passwords or " << std::endl;
    std::cerr << "  passphrases, including for ProxyJump hosts in ~/.ssh/config), then forwards" << std::endl;
    std::cerr << "  a local port to <hostname>:<port>. Pass \"\" for <username> to use the default." << std::endl;
    std::cerr << "  With --login-only, exits with status 0 as soon as login succeeds." << std::endl;
    std::cerr << "  With --config, reads that SSH config file instead of ~/.ssh/config." << std::endl;
    return -1;
  }

  SSHConnectParams params;
  params.host = argv[1];
  params.username = argv[3];
  params.verbose = true;
  bool login_only = false;
  for (int i = 4; i < argc; i++)
  {
    if (!strcmp(argv[i], "--login-only"))
      login_only = true;
    else if (!strcmp(argv[i], "--config") && i + 1 < argc)
      params.config_file = argv[++i];
    else
      params.keyfile = argv[i];
  }

  StdoutSSHAuthDelegate auth;
  ssh_session session = nullptr;
  try
  {
    session = OpenSSHSession(params, &auth);
  }
  catch (IRISException &exc)
  {
    std::cerr << exc.what() << std::endl;
    return -1;
  }

  if (login_only)
  {
    std::cout << "Login successful" << std::endl;
    ssh_disconnect(session);
    ssh_free(session);
    return 0;
  }

  return SSHTunnel::run(session, nullptr, atoi(argv[2]), tunnel_callback, nullptr, true);
}
