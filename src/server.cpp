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

std::unordered_map<int, std::string> clientBuffers;
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
    int n = epoll_wait(epoll_fd, events, 64, -1); // count of number of  socket changes

    // looping over the events
    for (int i = 0; i < n; i++)
    {
      int fd = events[i].data.fd;

      if (fd == serverSocket)
      {
        int client = accept(serverSocket, nullptr, nullptr);

        epoll_event client_ev{};
        client_ev.events = EPOLLIN;
        client_ev.data.fd = client;

        epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client, &client_ev);

        std::cout << "New client connected: " << client << std::endl;
      }
      else
      {
        char temp[1024];

        int bytes = recv(fd, temp, sizeof(temp) - 1, 0);

        if (bytes <= 0)
        {
          close(fd);
          epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, NULL);

          std::cout << "Client disconnected: " << fd << std::endl;
          clientBuffers.erase(fd);
        }
        else
        {
          clientBuffers[fd].append(temp, bytes);

          // std::cout << "Received: " << buffer << std::endl;

          std::string response;
          // std::cout << "___________ Parsing loop ______________" << std::endl;
          while (true)
          {
            std::vector<std::string> args;
            bool ok = parseOneCommand(clientBuffers[fd], args);
            if (!ok)
            {
              break;
            }
            std::cout << "command parsed: ";
            for (auto &a : args)
              std::cout << a << " ";
            std::cout << std::endl;

            response += handleCommand(args);
          }
          if (!response.empty())
          {
            std::cout << "Sending response:\n"
                      << response << std::endl;
            ssize_t sent = send(fd, response.c_str(), response.length(), 0);

            std::cout << "Bytes sent: " << sent << std::endl;
          }
        }
      }
    }
  }
}
