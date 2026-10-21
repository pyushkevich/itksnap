#include "SSHTunnel.h"

#ifndef _WIN32
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#else
#include <ws2tcpip.h>
#endif
#include <libssh/libssh.h>
#include <set>
#include <iostream>
#include <vector>
#include <chrono>
#include <cstring>
#include <cstdio>
#include <cstdarg>
#include <algorithm>

int
SSHTunnel::run(ssh_session session,
               const char *remote_host,
               int         remote_port,
               Callback    callback,
               void       *callback_data,
               bool        verbose)
{
  // Take ownership of the session right away, so it is freed on all paths
  SessionGuard sguard(session, verbose);

  // By default, forward to the SSH server itself, under its real hostname
  // (after ~/.ssh/config HostName substitution - an alias would mean nothing
  // to the server)
  std::string forward_host;
  if (remote_host)
    forward_host = remote_host;
  else
  {
    char *host = nullptr;
    if (ssh_options_get(session, SSH_OPTIONS_HOST, &host) == SSH_OK && host)
    {
      forward_host = host;
      ssh_string_free_char(host);
    }
  }
  remote_host = forward_host.c_str();

  auto fail = [callback, callback_data](int rc, const char *message, ...) {
    char buf[1024];
    va_list args;
    va_start(args, message);
    vsnprintf(buf, 1024, message, args);
    va_end(args);
    callback(CB_ERROR, ErrorInfo({buf}), callback_data);
    return rc;
  };

  // Create a server socket
  int server_socket = socket(AF_INET, SOCK_STREAM, 0);
  if (server_socket < 0)
    return fail(RC_SOCKET_ERROR, "Error creating server socket: %s", strerror(errno));

  // RAII guard to ensure server socket is closed on all return paths
  struct ServerSocketGuard {
    int fd;
    ~ServerSocketGuard() {
#ifndef _WIN32
      ::close(fd);
#else
      ::closesocket(fd);
#endif
    }
  } server_socket_guard{server_socket};

  // Bind server_socket to address
  struct sockaddr_in address, bound_address;
  socklen_t address_size = sizeof(address);
  address.sin_family = AF_INET;
  address.sin_port = htons(0);
  address.sin_addr.s_addr = INADDR_ANY;
  if (bind(server_socket, (struct sockaddr *) &address, address_size) < 0)
    return fail(RC_SOCKET_ERROR, "Error binding server socket: %s", strerror(errno));

  // Get the port to which the socket is bound
  memset(&bound_address, '\0', sizeof(bound_address));
  socklen_t bound_address_size = sizeof(bound_address);
  if (getsockname(server_socket, (struct sockaddr *) &bound_address, &bound_address_size) < 0)
    return fail(RC_SOCKET_ERROR, "Error getting server socket address: %s", strerror(errno));
  int bound_port = ntohs(bound_address.sin_port);

  // Buffer for IO
  constexpr int buffer_size = 1024 * 1024;
  std::vector<char> buffer(buffer_size, 0);

  // Put the socket into listen mode
  if (listen(server_socket, 4) < 0)
    return fail(RC_SOCKET_ERROR, "Error listening on server socket: %s", strerror(errno));

  // At this point, we are ready to serve tunnel connections
  if(callback(CB_READY, ReadyInfo({"localhost", bound_port}), callback_data).first)
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

    // Create a list of channels to be passed to ssh_select. ch_out must be fully
    // cleared each iteration: ssh_select writes only ready channels into ch_out
    // (null-terminated list), leaving any remaining slots untouched. Stale non-null
    // entries from a prior iteration would cause false positives in the find below.
    ch_in.reserve(sguard.GetTunnels().size() + 1);
    ch_in.clear();
    ch_out.assign(sguard.GetTunnels().size() + 1, nullptr);
    for (auto it : sguard.GetTunnels())
    {
      FD_SET(it.first, &fs);
      if(it.first > max_fd)
        max_fd = it.first;
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
      if(verbose)
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
        if(verbose)
          std::cout << "... Activity on socket " << it.first << std::endl;

        int n_read = recv(it.first, buffer.data(), buffer_size, 0);
        if (n_read > 0)
        {
          if(verbose)
            std::cout << "Read " << n_read << " bytes from socket " << it.first << std::endl;
          int n_written = ssh_channel_write(it.second, buffer.data(), n_read);
          if (n_written != n_read)
            return fail(RC_SSH_ERROR,
                        "Error writing to SSH tunnel, only %d of %d bytes written: %s",
                        n_written,
                        n_read,
                        ssh_get_error(session));
          if(verbose)
            std::cout << "Wrote " << n_written << " bytes to channel" << std::endl;
        }
        else if (n_read == 0)
        {
          if(verbose)
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
        if(verbose)
          std::cout << "... Activity on channel for socket " << it.first << std::endl;

        // Yes, channel indicated that it needs to be read
        if (ssh_channel_poll(it.second, 1) > 0)
        {
          // The channel has something in the error stream
          int         n_read = ssh_channel_read(it.second, buffer.data(), buffer_size - 1, 1);
          std::string errmsg(buffer.data(), n_read);
          return fail(RC_SSH_ERROR, "SSH server sent error message: %s", errmsg.c_str());
        }

        int n_avail = ssh_channel_poll(it.second, 0);
        if(verbose)
          std::cout << "ssh_channel_poll returned " << n_avail << std::endl;
        if (n_avail == SSH_EOF)
        {
          // End of file from the remote socket - terminate the connection
          if(verbose)
            std::cout << "EOF from channel on socket " << it.first << std::endl;
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

          if(verbose)
            std::cout << "Read " << n_read << " bytes from channel for socket " << it.first
                      << std::endl;
          int n_written = send(it.first, buffer.data(), n_read, 0);
          if (n_written != n_read)
            return fail(
              RC_SOCKET_ERROR, "Error writing to client socket %d, %s", it.first, strerror(errno));
        }
        else if (n_avail < 0)
          return fail(RC_SSH_ERROR, "Error polling SSH channel: %s", ssh_get_error(session));

        if (ssh_channel_is_eof(it.second))
        {
          if(verbose)
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
#ifndef _WIN32
  shutdown(socket_fd, SHUT_RDWR);
  close(socket_fd);
#else
  shutdown(socket_fd, SD_BOTH);
  closesocket(socket_fd);
#endif
  if(verbose)
    std::cout << "Disconnected from socket " << socket_fd << std::endl;
}
