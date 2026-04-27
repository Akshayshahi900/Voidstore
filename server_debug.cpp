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
  std::cout << "[CLOSE] fd=" << fd << std::endl;
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
    std::cout << "[EPOLL] n=" << n << std::endl;

    for (int i = 0; i < n; i++)
    {
      int fd = events[i].data.fd;
      std::cout << "[EVENT] fd=" << fd << " events=0x" << std::hex << events[i].events << std::dec << std::endl;

      if (fd == serverSocket)
      {
        int client = accept(serverSocket, nullptr, nullptr);
        if (client < 0)
        {
          std::cout << "[ACCEPT] failed errno=" << errno << std::endl;
          continue;
        }
        fcntl(client, F_SETFL, O_NONBLOCK);
        epoll_event client_ev{};
        client_ev.events = EPOLLIN;
        client_ev.data.fd = client;
        epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client, &client_ev);
        std::cout << "[ACCEPT] client fd=" << client << std::endl;
        continue;
      }

      // recv loop
      bool peer_closed = false;
      bool client_error = false;
      int total_read = 0;

      while (true)
      {
        char temp[4096];
        ssize_t bytes = recv(fd, temp, sizeof(temp), 0);
        std::cout << "[RECV] fd=" << fd << " bytes=" << bytes << " errno=" << errno << std::endl;

        if (bytes > 0)
        {
          clientBuffers[fd].append(temp, bytes);
          total_read += bytes;
        }
        else if (bytes == 0)
        {
          peer_closed = true;
          break;
        }
        else
        {
          if (errno == EAGAIN || errno == EWOULDBLOCK)
            break;
          client_error = true;
          break;
        }
      }

      std::cout << "[RECV DONE] fd=" << fd << " total=" << total_read
                << " buf=" << clientBuffers[fd].size()
                << " peer_closed=" << peer_closed
                << " error=" << client_error << std::endl;

      if (client_error)
      {
        closeClient(epoll_fd, fd);
        continue;
      }

      // parse loop
      std::string response;
      std::string &buf = clientBuffers[fd];
      int cmd_count = 0;

      while (!buf.empty())
      {
        std::vector<std::string> args;
        bool ok = parseOneCommand(buf, args);
        if (!ok)
          break;
        if (!args.empty())
        {
          response += handleCommand(args);
          cmd_count++;
        }
      }

      std::cout << "[PARSE] fd=" << fd << " commands=" << cmd_count
                << " response_bytes=" << response.size()
                << " remaining_buf=" << buf.size() << std::endl;

      writeBuffers[fd] += response;

      // send loop
      std::string &wb = writeBuffers[fd];
      int total_sent = 0;
      bool send_error = false;

      while (!wb.empty())
      {
        ssize_t sent = send(fd, wb.c_str(), wb.size(), 0);
        std::cout << "[SEND] fd=" << fd << " sent=" << sent << " errno=" << errno << std::endl;
        if (sent > 0)
        {
          wb.erase(0, sent);
          total_sent += sent;
        }
        else if (sent < 0)
        {
          if (errno == EAGAIN || errno == EWOULDBLOCK)
            break;
          send_error = true;
          break;
        }
      }

      std::cout << "[SEND DONE] fd=" << fd << " total_sent=" << total_sent
                << " remaining=" << wb.size() << std::endl;

      if (send_error)
      {
        closeClient(epoll_fd, fd);
        continue;
      }
      if (peer_closed)
        closeClient(epoll_fd, fd);
    }
  }
}
