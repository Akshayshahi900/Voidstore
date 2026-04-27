#include "server.h"
#include "resp_parser.h"
#include "commands.h"
#include "persistence.h"

#include <iostream>
#include <cstring>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <unordered_map>
#include <fcntl.h>
#include <errno.h>

std::unordered_map<int, std::string> clientBuffers;
std::unordered_map<int, std::string> writeBuffers;

static void closeClient(int epoll_fd, int fd)
{
  epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, NULL);
  close(fd);
  clientBuffers.erase(fd);
  writeBuffers.erase(fd);
  std::cout << "Client disconnected: " << fd << std::endl;
}

void startServer()
{
  loadDatabase();

  int serverSocket = socket(AF_INET, SOCK_STREAM, 0);

  int opt = 1;
  setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

  sockaddr_in serverAddress{};
  serverAddress.sin_family = AF_INET;
  serverAddress.sin_port = htons(8080);
  serverAddress.sin_addr.s_addr = INADDR_ANY;

  bind(serverSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress));
  listen(serverSocket, 5);

  std::cout << "Server listening on port 8080...\n";

  int epoll_fd = epoll_create1(0);

  epoll_event ev{};
  ev.events = EPOLLIN;
  ev.data.fd = serverSocket;
  epoll_ctl(epoll_fd, EPOLL_CTL_ADD, serverSocket, &ev);

  epoll_event events[64];

  while (true)
  {
    int n = epoll_wait(epoll_fd, events, 64, -1);

    for (int i = 0; i < n; i++)
    {
      int fd = events[i].data.fd;

      // ── New connection ────────────────────────────────────────────────
      if (fd == serverSocket)
      {
        int client = accept(serverSocket, nullptr, nullptr);
        if (client < 0)
          continue;

        fcntl(client, F_SETFL, O_NONBLOCK);

        epoll_event client_ev{};
        client_ev.events = EPOLLIN;
        client_ev.data.fd = client;
        epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client, &client_ev);

        std::cout << "New client connected: " << client << std::endl;
        continue;
      }

      // ── Existing client ───────────────────────────────────────────────

      // 1. Drain the socket into the read buffer
      bool peer_closed = false;
      bool client_error = false;

      while (true)
      {
        char temp[4096];
        ssize_t bytes = recv(fd, temp, sizeof(temp), 0);

        if (bytes > 0)
        {
          clientBuffers[fd].append(temp, bytes);
        }
        else if (bytes == 0)
        {
          // Peer shut down its write side.
          // Process whatever is buffered, send replies, THEN close.
          peer_closed = true;
          break;
        }
        else
        {
          if (errno == EAGAIN || errno == EWOULDBLOCK)
            break; // no more data right now; normal for non-blocking fd

          client_error = true;
          break;
        }
      }

      if (client_error)
      {
        closeClient(epoll_fd, fd);
        continue;
      }

      // 2. Parse every complete RESP command and build responses
      {
        std::string response;
        std::string &buf = clientBuffers[fd];

        while (!buf.empty())
        {
          std::vector<std::string> args;
          bool ok = parseOneCommand(buf, args);

          if (!ok)
          {
            // Incomplete frame — parseOneCommand consumed nothing.
            // Wait for more bytes from epoll before retrying.
            break;
          }

          if (!args.empty())
            response += handleCommand(args);
        }

        writeBuffers[fd] += response;
      }

      // 3. Flush write buffer to socket
      {
        std::string &wb = writeBuffers[fd];
        bool send_error = false;

        while (!wb.empty())
        {
          ssize_t sent = send(fd, wb.c_str(), wb.size(), 0);

          if (sent > 0)
          {
            wb.erase(0, sent);
          }
          else if (sent < 0)
          {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
              break; // TX buffer full; come back next epoll wake-up

            send_error = true;
            break;
          }
        }

        if (send_error)
        {
          closeClient(epoll_fd, fd);
          continue;
        }
      }

      // 4. Close only after replies are sent (or if peer already closed)
      if (peer_closed)
        closeClient(epoll_fd, fd);
    }
  }
}
