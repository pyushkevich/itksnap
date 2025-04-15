#include "SSHTunnel.h"
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <libssh/libssh.h>
#include <set>
#include <iostream>

/*

SSHTunnel::SSHTunnel()
{
  ssh_init();
}

SSHTunnel::~SSHTunnel()
{
  ssh_finalize();
}

void
SSHTunnel::CreateTunnel(const char *remote_host,
                        int         local_port,
                        int         remote_port,
                        const char *username,
                        const char *password,
                        const char *keyfile)
{
  // TODO: this is in the wrong place
  ssh_init();

  // Set the ports
  m_LocalPort = local_port;
  m_RemotePort = remote_port;
  m_RemoteHost = remote_host;

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

  // Open a direct TCP/IP tunnel
  CreateChannel(session);
  m_Session = session;
}

bool
SSHTunnel::ReestablishChannel()
{
  if(ssh_channel_is_eof(m_Channel))
  {
    ssh_channel_close(m_Channel);
    ssh_channel_free(m_Channel);
    CreateChannel(m_Session);
    return true;
  }
  return false;
}

void
SSHTunnel::CreateChannel(ssh_session &session)
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
  auto rc_of = ssh_channel_open_forward(channel, m_RemoteHost.c_str(), m_RemotePort, "localhost", m_LocalPort);
  auto t3 = Clock::now();
  if (rc_of != SSH_OK)
  {
    ssh_channel_free(channel);
    std::string err(ssh_get_error(session));
    ssh_disconnect(session);
    ssh_free(session);
    throw IRISException("Opening tunnel failed: %s", err.c_str());
  }

  m_Channel = channel;
  std::cout << "ssh_channel_new time: " << std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count() << "µs" << std::endl;
  std::cout << "ssh_channel_open_forward time: " << std::chrono::duration_cast<std::chrono::microseconds>(t3 - t2).count() << "µs" << std::endl;
}

*/

int
SSHTunnel::run(const char *remote_host,
               int         remote_port,
               const char *username,
               const char *keyfile,
               Callback    callback,
               void       *callback_data)
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

  // Buffer for IO
  constexpr int buffer_size = 1024 * 1024;
  std::vector<char> buffer(buffer_size, 0);

  // TODO: this is in the wrong place
  ssh_init();
  ssh_set_log_level(SSH_LOG_TRACE);

  // Create a new session
  std::cout << "Creating tunnel to " << remote_host << std::endl;
  ssh_session session = ssh_new();

  // Define an easy to call error function
  auto fail = [callback, callback_data](int rc, const char *message, ...) {

    // Format the message
    char buffer[1024];
    va_list args;
    va_start(args, message);
    vsnprintf(buffer, 1024, message, args);
    va_end (args);

    // Call the error callback
    callback(CB_ERROR, ErrorInfo({buffer}), callback_data);
    return rc;
  };

  if (!session)
    return fail(RC_SSH_ERROR, "Error creating SSH session");

  // Create a session object that will perform cleanup on exit
  SessionGuard sguard(session);

  // Set the host option
  std::cout << "Setting hostname to " << remote_host << std::endl;
  if (ssh_options_set(session, SSH_OPTIONS_HOST, remote_host) != SSH_OK)
    return fail(RC_SSH_ERROR, "Error setting ssh hostname to %s: %s", remote_host, ssh_get_error(session));

  // Set the username option
  if (username && strlen(username) > 0)
  {
    std::cout << "Setting username to " << username << std::endl;
    if (ssh_options_set(session, SSH_OPTIONS_USER, username) != SSH_OK)
      return fail(RC_SSH_ERROR, "Error setting ssh usename to %s: %s", username, ssh_get_error(session));
  }

  // Connect to remote
  if (ssh_connect(session) != SSH_OK)
    return fail(RC_SSH_ERROR, "SSH connection failed: %s", ssh_get_error(session));

  // Authenticate using the private key
  bool        auth = false;
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

  // Prompt for the password via callback
  if(!auth)
  {
    // If there is no username, we cannot continue
    if(!strlen(username))
      return fail(RC_SSH_ERROR, "No username provided for SSH authentication");
    std::string error_msg;
    while(true)
    {
      // Do the callback
      auto rc = callback(
        CB_PROMPT_PASSWORD, PromptPasswordInfo({ remote_host, username, error_msg }), callback_data);

      // Interrupted by the user - did not provide password
      if(rc.first)
        return RC_USER_ABORT;

      // Authorize using the password.
      if(ssh_userauth_password(session, nullptr, rc.second.c_str()) == SSH_AUTH_SUCCESS)
      {
        std::cout << "Successfully authenticated using password" << std::endl;
        auth = true;
        break;
      }
      else
      {
        error_msg = ssh_get_error(session);
      }
    }
  }

  // Put the socket into listen mode
  listen(server_socket, 4);

  // At this point, we are ready to serve tunnel corrections
  if(callback(CB_READY, ReadyInfo({bound_ip, bound_port}), callback_data).first)
    return RC_USER_ABORT;

  // Data for ssh_select
  fd_set fs;
  std::vector<ssh_channel> ch_in, ch_out;

  // Infinite loop
  while (true)
  {
    // Create an fd_set consisting of all sockets that we can listen to
    int max_fd = server_socket;
    FD_ZERO(&fs);
    FD_SET(server_socket, &fs);

    // Create a list of channels to be passed to ssh_select
    ch_in.reserve(sguard.GetTunnels().size() + 1);
    ch_in.clear();
    ch_out.resize(sguard.GetTunnels().size() + 1, nullptr);
    for (auto it : sguard.GetTunnels())
    {
      FD_SET(it.first, &fs);
      max_fd = std::max(max_fd, it.first);
      ch_in.push_back(it.second);
    }
    ch_in.push_back(nullptr);

    // Set the timeout to wait
    timeval timeout = { 0, 200000 };

    // Check if the user wants to abort
    if(callback(CB_TERMINATION_CHECK, CallbackInfo(), callback_data).first)
      return RC_USER_ABORT;

    // SSH select: wait for action on at least one of the sockets/channels
    int rc = ssh_select(ch_in.data(), ch_out.data(), max_fd + 1, &fs, &timeout);

    // Handle new connection
    if (FD_ISSET(server_socket, &fs))
    {
      std::cout << "... Activity on server socket" << std::endl;

      // Accept returns the client socket fd
      int client_socket = accept(server_socket, nullptr, nullptr);
      if (client_socket < 0)
        return fail(RC_SOCKET_ERROR, "Error establishing server socket: %s", strerror(errno));

      // Open a direct TCP/IP tunnel
      typedef std::chrono::high_resolution_clock Clock;
      auto t0 = Clock::now();
      ssh_channel channel = ssh_channel_new(session);
      auto t1 = Clock::now();
      if (!channel)
        return fail(RC_SSH_ERROR, "Error creating SSH channel: %s", ssh_get_error(session));

      // Add as a new pair
      sguard.AddTunnel(client_socket, channel);

      // Open a forward
      auto t2 = Clock::now();
      auto rc_of = ssh_channel_open_forward(channel, remote_host, remote_port, "localhost", bound_port);
      auto t3 = Clock::now();
      if (rc_of != SSH_OK)
        return fail(RC_SSH_ERROR,
                    "Error opening SSH tunnel to host %s port %d: %s",
                    remote_host,
                    remote_port,
                    ssh_get_error(session));
    }

    // Handle data from socket
    std::set<int> to_delete;
    for (auto it : sguard.GetTunnels())
    {
      // Check if a socket needs to be read
      if (FD_ISSET(it.first, &fs))
      {
        std::cout << "... Activity on socket " << it.first << std::endl;

        int n_read = recv(it.first, buffer.data(), buffer_size, 0);
        if (n_read > 0)
        {
          std::cout << "Read " << n_read << " bytes from socket " << it.first << std::endl;
          int n_written = ssh_channel_write(it.second, buffer.data(), n_read);
          if (n_written != n_read)
            fail(RC_SSH_ERROR,
                 "Error writing to SSH tunnel, only %d of %d bytes written: %s",
                 n_written,
                 n_read,
                 ssh_get_error(session));
          std::cout << "Wrote " << n_written << " bytes to channel" << std::endl;
        }
        else if (n_read == 0)
        {
          std::cout << "Socket is closed" << std::endl;
          to_delete.insert(it.first);
        }
        else if (n_read < 0)
          return fail(
            RC_SOCKET_ERROR, "Error reading from client socket (%d): %s", it.first, strerror(errno));
      }

      // Check if a channel needs to be read
      if (std::find(ch_out.begin(), ch_out.end(), it.second) != ch_out.end())
      {
        std::cout <<  "... Activity on channel for socket " << it.first << std::endl;

        // Yes, channel indicated that it needs to be read
        if (ssh_channel_poll(it.second, 1) > 0)
        {
          // The channel has something in the error stream
          int         n_read = ssh_channel_read(it.second, buffer.data(), buffer_size - 1, 1);
          std::string errmsg(buffer.data(), n_read);
          return fail(RC_SSH_ERROR, "SSH server sent error message: %s", errmsg.c_str());
        }

        int n_avail = ssh_channel_poll(it.second, 0);
        std::cout << "ssh_channel_poll returned " << n_avail << std::endl;
        if (n_avail == SSH_EOF)
        {
          // End of file from the remote socket - terminate the connection
          std::cout <<  "EOF from channel on socket " << it.first << std::endl;
          to_delete.insert(it.first);
        }
        else if (n_avail > 0)
        {
          // TODO: handle case if n_avail > buffer size
          int n_read = ssh_channel_read(it.second, buffer.data(), n_avail, 0);
          if (n_read != n_avail)
            return fail(RC_SSH_ERROR,
                        "Error reading from SSH tunnel, only %d of %d bytes read: %s",
                        n_read,
                        n_avail,
                        ssh_get_error(session));

          std::cout << "Read " << n_read << " bytes from channel for socket " << it.first << std::endl;
          int n_written = send(it.first, buffer.data(), n_read, 0);
          if (n_written != n_read)
            return fail(
              RC_SOCKET_ERROR, "Error writing to client socket %d, %s", it.first, strerror(errno));
        }
        else if (n_avail < 0)
          return fail(RC_SSH_ERROR, "Error polling SSH channel: %s", strerror(errno));

        if (ssh_channel_is_eof(it.second))
        {
          std::cout << "EOF from channel on socket " << it.first << std::endl;
          to_delete.insert(it.first);
        }
      }
    }

    // Handle the deleted stuff
    for (auto itd : to_delete)
      sguard.RemoveTunnel(itd);
  }
}


SSHTunnel::SessionGuard::~SessionGuard()
{
  for(auto itd : channel_map)
    CleanupTunnel(itd.first, itd.second);

  if(ssh_is_connected(session))
    ssh_disconnect(session);

  ssh_free(session);
}

void
SSHTunnel::SessionGuard::AddTunnel(int socket_id, ssh_channel channel)
{
  RemoveTunnel(socket_id);
  channel_map[socket_id] = channel;
}

void
SSHTunnel::SessionGuard::RemoveTunnel(int socket_id)
{
  auto itd = channel_map.find(socket_id);
  if(itd != channel_map.end())
  {
    CleanupTunnel(itd->first, itd->second);
    channel_map.erase(itd);
  }
}

void
SSHTunnel::SessionGuard::CleanupTunnel(int socket_fd, ssh_channel channel)
{
  if (ssh_channel_is_open(channel))
  {
    ssh_channel_send_eof(channel);
    ssh_channel_close(channel);
  }
  ssh_channel_free(channel);
  shutdown(socket_fd, SHUT_RDWR);
  close(socket_fd);
  std::cout << "Disconnected from socket " << socket_fd << std::endl;
}
