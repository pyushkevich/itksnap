#include <QDebug>
#include <QApplication>
#include <iostream>
#include "SSHTunnel.h"

SSHTunnel::CallbackResponse tunnel_callback(SSHTunnel::CallbackType type, SSHTunnel::CallbackInfo info, void *)
{
  SSHTunnel::CallbackResponse rc;
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
    case SSHTunnel::CB_PROMPT_PASSWORD:
    case SSHTunnel::CB_PROMPT_PASSKEY:
    {
      // TODO: prompt for password here
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
    std::cerr << "Usage: ssh_tunnel_test <hostname> <port> <username>" << std::endl;
    return -1;
  }

  return SSHTunnel::run(argv[1], atoi(argv[2]), argv[3], nullptr, tunnel_callback, nullptr);
}
