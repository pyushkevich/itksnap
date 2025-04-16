#include "IRISException.h"
#include <QDebug>
#include <QApplication>

#ifndef _WIN32
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#endif 

#include <libssh/libssh.h>
#include <set>
#include "SSHTunnel.h"

#include <QDebug>

/*

ssh_session
ssh_create_forward(const char *remote_host,
                   int         local_port,
                   int         remote_port,
                   const char *username,
                   const char *password,
                   const char *keyfile)
{
  // TODO: this is in the wrong place
  ssh_init();

  // Create a new session
  std::cout << "Creating tunnel to " << remote_host << std::endl;
  ssh_set_log_level(SSH_LOG_TRACE);
  ssh_session session = ssh_new();
  if (!session)
    throw IRISException("Error creating SSH session");

  // Set the host and user options
  std::cout << "Setting hostname to " << remote_host << std::endl;
  if(ssh_options_set(session, SSH_OPTIONS_HOST, remote_host) != SSH_OK)
  {
    std::string err(ssh_get_error(session));
    ssh_free(session);
    throw IRISException("Setting option failed: %s", err.c_str());
  }
  ssh_options_set(session, SSH_OPTIONS_USER, username);

  // Connect to remote
  if (ssh_connect(session) != SSH_OK)
  {
    std::string err(ssh_get_error(session));
    ssh_free(session);
    throw IRISException("SSH Connection failed: %s", err.c_str());
  }

  // Authenticate using the private key
  bool auth = false;
  std::string err_pubkey, err_password;
  if (ssh_userauth_publickey_auto(session, NULL, NULL) == SSH_AUTH_SUCCESS)
  {
    std::cout << "Successfully authenticated using auto-detected key" << std::endl;
    auth = true;
  }
  else
  {
    err_pubkey = ssh_get_error(session);
  }

  if (!auth && password && ssh_userauth_password(session, nullptr, password) != SSH_AUTH_SUCCESS)
  {
    std::cout << "Successfully authenticated using password" << std::endl;
    auth = true;
  }
  else
  {
    err_password = ssh_get_error(session);
  }

  if(!auth)
  {
    ssh_disconnect(session);
    ssh_free(session);

    throw IRISException(
      "SSH Authentication failed.\nPublic key attempt error: %s\nPassword attempt error: %s\n",
      err_pubkey.c_str(),
      err_password.c_str());
  }

  return session;
}

ssh_channel ssh_create_channel(ssh_session &session, const char *remote_host, int remote_port, int local_port)
{
  // Open a direct TCP/IP tunnel
  typedef std::chrono::high_resolution_clock Clock;
  auto t0 = Clock::now();
  ssh_channel channel = ssh_channel_new(session);
  auto t1 = Clock::now();
  if (!channel)
  {
    std::string err(ssh_get_error(session));
    ssh_disconnect(session);
    ssh_free(session);
    throw IRISException("Creating channel failed: %s", err.c_str());
  }

  auto t2 = Clock::now();
  auto rc_of = ssh_channel_open_forward(channel, remote_host, remote_port, "localhost", local_port);
  auto t3 = Clock::now();
  if (rc_of != SSH_OK)
  {
    ssh_channel_free(channel);
    std::string err(ssh_get_error(session));
    ssh_disconnect(session);
    ssh_free(session);
    throw IRISException("Opening tunnel failed: %s", err.c_str());
  }

  std::cout << "New channel created!" << std::endl;
  std::cout << "ssh_channel_new time: " << std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count() << "µs" << std::endl;
  std::cout << "ssh_channel_open_forward time: " << std::chrono::duration_cast<std::chrono::microseconds>(t3 - t2).count() << "µs" << std::endl;
  return channel;
}


int
main(int argc, char **argv)
{
  // Create a server socket
  int server_socket = socket(AF_INET, SOCK_STREAM, 0);

  // Bind server_socket to address
  struct sockaddr_in address, bound_address;
  socklen_t address_size = sizeof(address);
  address.sin_family = AF_INET;
  address.sin_port = htons(0);
  address.sin_addr.s_addr = INADDR_ANY;
  bind(server_socket, (struct sockaddr *) &address, address_size);

  // Get the port to which the socket is bound
  bzero(&bound_address, sizeof(bound_address));
  socklen_t bound_address_size = sizeof(bound_address);
  getsockname(server_socket, (struct sockaddr *) &bound_address, &bound_address_size);
  char bound_ip[16];
  inet_ntop(AF_INET, &bound_address.sin_addr, bound_ip, sizeof(bound_ip));
  int bound_port = ntohs(bound_address.sin_port);

  const char *remote_host = "lambda-clam", *username="pauly2";
  int remote_port = 8911;

  // Buffer for IO
  constexpr int buffer_size = 1024 * 1024;
  char *buffer = new char[buffer_size];

  // Create an SSH tunnel
  try
  {
    // Create a forwarding connection
    ssh_session session = ssh_create_forward(remote_host, bound_port, remote_port, username, nullptr, nullptr);

    // Put the socket into listen mode
    listen(server_socket, 4);

    // Create a list of client socket / ssh channel pairs
    using TunnelConnection = std::pair<int, ssh_channel>;
    std::set<TunnelConnection> tcon;

    fd_set fs;
    std::vector<ssh_channel> ch_in, ch_out;

    qInfo() << "*** Tunnel running on IP " << bound_ip << " port " << bound_port;

    // Main loop - wait for incoming stuff
    forever
    {
      // Create an fd_set consisting of all sockets that we can listen to
      int max_fd = server_socket;
      FD_ZERO(&fs);
      FD_SET(server_socket, &fs);

      // Create a list of channels to be passed to ssh_select
      ch_in.reserve(tcon.size() + 1);
      ch_in.clear();
      ch_out.resize(tcon.size() + 1, nullptr);
      for(auto it : tcon)
      {
        FD_SET(it.first, &fs);
        max_fd = std::max(max_fd, it.first);
        ch_in.push_back(it.second);
      }
      ch_in.push_back(nullptr);

      // Set the timeout to wait
      timeval timeout = { 0, 200000 };

      // SSH select: wait for action on at least one of the sockets/channels
      int rc = ssh_select(ch_in.data(), ch_out.data(), max_fd+1, &fs, &timeout);
      //qDebug() << "--- ssh_select returned " << rc << " ---";

      // Handle new connection
      if(FD_ISSET(server_socket, &fs))
      {
        qDebug() << "... Activity on server socket";

        // Accept returns the client socket fd
        int client_socket = accept(server_socket, nullptr, nullptr);
        if(client_socket < 0)
          throw IRISException("Error in accept: %d", errno);

        // Create a channel for this socket
        ssh_channel channel = ssh_create_channel(session, remote_host, remote_port, bound_port);

        // Add as a new pair
        tcon.insert(std::make_pair(client_socket, channel));
      }

      // Handle data from socket
      std::set <TunnelConnection> to_delete;
      for(auto it: tcon)
      {
        // Check if a socket needs to be read
        if(FD_ISSET(it.first, &fs))
        {
          qDebug() << "... Activity on socket " << it.first;

          int n_read = recv(it.first, buffer, buffer_size, 0);
          if(n_read > 0)
          {
            qDebug() << "Read " << n_read << " bytes from socket " << it.first;
            int n_written = ssh_channel_write(it.second, buffer, n_read);
            if(n_written != n_read)
            {
              throw IRISException("ssh_channel_write returned %d, expected %d, error: %s",
                                  n_written,
                                  n_read,
                                  ssh_get_error(session));
            }
            qDebug() << "Wrote " << n_written << " bytes to channel";
          }
          else if(n_read == 0)
          {
            qDebug() << "Socket is closed";
            to_delete.insert(it);
          }
          else if(n_read < 0)
          {
            throw IRISException("Error in reading from socket %d: %d", it.first, errno);
          }
        }

        // Check if a channel needs to be read
        if(std::find(ch_out.begin(), ch_out.end(), it.second) != ch_out.end())
        {
          qDebug() << "... Activity on channel for socket " << it.first;

          // Yes, channel indicated that it needs to be read
          if(ssh_channel_poll(it.second, 1) > 0)
          {
            // The channel has something in the error stream
            int n_read = ssh_channel_read(it.second, buffer, sizeof(buffer) - 1, 1);
            std::string errmsg(buffer, n_read);
            throw IRISException("Channel for socket %d returned error message %s", it.first, errmsg.c_str());
          }

          int n_avail = ssh_channel_poll(it.second, 0);
          qDebug() << "ssh_channel_poll returned " << n_avail;
          if(n_avail == SSH_EOF)
          {
            // End of file from the remote socket - terminate the connection
            qDebug() << "EOF from channel on socket " << it.first;
            to_delete.insert(it);
          }
          else if(n_avail > 0)
          {
            // TODO: handle case if n_avail > buffer size
            int n_read = ssh_channel_read(it.second, buffer, n_avail, 0);
            if(n_read != n_avail)
            {
              throw IRISException("ssh_channel_read returned %d, expected %d, error: %s",
                                  n_read,
                                  n_avail,
                                  ssh_get_error(session));
            }

            qDebug() << "Read " << n_read << " bytes from channel for socket " << it.first;
            int n_written = send(it.first, buffer, n_read, 0);
            if(n_written != n_read)
              throw IRISException("Error in writing to socket %d: %d", it.first, errno);
          }
          else if(n_avail < 0)
          {
            throw IRISException("ssh_channel_poll error: %s", ssh_get_error(session));
          }

          if(ssh_channel_is_eof(it.second))
          {
            qDebug() << "EOF from channel on socket " << it.first;
            to_delete.insert(it);
          }
        }
      }

      // Handle the deleted stuff
      for(auto itd : to_delete)
      {
        ssh_channel_send_eof (itd.second);
        ssh_channel_close(itd.second);
        ssh_channel_free(itd.second);
        shutdown(itd.first, SHUT_RDWR);
        close(itd.first);
        tcon.erase(itd);
        qDebug() << "Disconnected from socket " << itd.first;
      }
    }
  }
  catch (IRISException &exc)
  {
    qCritical() << "IRISException: " << exc.what();
    close(server_socket);
  }
}

 */

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
  SSHTunnel::run("lambda-clam", 8911, "pauly2", nullptr, tunnel_callback, nullptr);
}


/*
int main(int argc, char **argv)
{
  // Establish an SSH tunnel
  try {
    // Create a tunnel channel
    SmartPtr<DeepLearningServerPropertiesModel> m = DeepLearningServerPropertiesModel::New();
    m->SetUseSSHTunnel(true);
    m->SetHostname("lambda-clam");
    m->SetPort(8911);
    m->SetSSHUsername("pauly2");

    // Create the server
    QApplication app(argc, argv);
    QApplication::setApplicationDisplayName("SSH Tunnel");
    SSHTunnelServer server(nullptr);
    server.SetServerProperties(m);
    server.createTunnel();
    return app.exec();
  }
  catch(IRISException &exc)
  {
    qCritical() << "Exception: " << exc.what();
  }

}
*/
