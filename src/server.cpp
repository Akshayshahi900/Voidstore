#include <iostream>
#include <cstring>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <unordered_map>
#include <string>
#include <sstream>
#include <fstream>
#include <chrono>
#include <sys/epoll.h>
#include <vector>

std::unordered_map<std::string, std::string> store;
std::ofstream aof("db.aof", std::ios::app);
std::unordered_map<std::string, std::time_t> expiry;

std::vector<std::string> parseRESP(const std::string &input)
{
  std::vector<std::string> args;
  std::istringstream stream(input);
  std::string line;

  std::getline(stream, line);
  if (!line.empty() && line.back() == '\r')
    line.pop_back();
  if (line.empty() || line[0] != '*')
    return args;
  int argCount = std::stoi(line.substr(1));

  for (int i = 0; i < argCount; i++)
  {
    std::getline(stream, line); //$length line
    std::getline(stream, line); // actual argument

    if (!line.empty() && line.back() == '\r')
      line.pop_back();
    args.push_back(line);
  }
  return args;
}
void loadDatabase()
{
  std::ifstream file("db.aof");
  std::string line;

  while (std::getline(file, line))
  {
    std::istringstream iss(line);
    std::string command;
    iss >> command;

    if (command == "SET")
    {
      std::string key, value;
      int ttl;

      iss >> key >> value;

      store[key] = value;

      if (iss >> ttl)
      {
        expiry[key] = time(NULL) + ttl;
      }
    }
    else if (command == "DEL")
    {
      std::string key;
      iss >> key;
      store.erase(key);
    }
  }
}
int main()
{
  // load database

  loadDatabase();
  // 1. Create socket
  int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
  if (serverSocket < 0)
  {
    perror("Socket creation failed");
    return 1;
  }

  // 2. Allow address reuse
  int opt = 1;
  if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
  {
    perror("setsockopt failed");
    return 1;
  }

  // 3. Configure server address
  sockaddr_in serverAddress{};
  serverAddress.sin_family = AF_INET;
  serverAddress.sin_port = htons(8080);
  serverAddress.sin_addr.s_addr = INADDR_ANY;

  // 4. Bind
  if (bind(serverSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0)
  {
    perror("Bind failed");
    return 1;
  }

  // 5. Listen
  if (listen(serverSocket, 5) < 0)
  {
    perror("Listen failed");
    return 1;
  }

  std::cout << "Server listening on port 8080...\n";

  int epoll_fd = epoll_create1(0);
  epoll_event ev;
  ev.events = EPOLLIN;
  ev.data.fd = serverSocket;

  epoll_ctl(epoll_fd, EPOLL_CTL_ADD, serverSocket, &ev);

  sockaddr_in clientAddress{};
  socklen_t clientSize = sizeof(clientAddress);
  epoll_event events[64];
  // 6. Main server loop
  while (true)
  {
    int n = epoll_wait(epoll_fd, events, 64, -1);

    for (int i = 0; i < n; i++)
    {
      int fd = events[i].data.fd;
      if (fd == serverSocket)
      {
        int newfd = accept(serverSocket, (struct sockaddr *)&clientAddress, &clientSize);
        if (newfd == -1)
        {
          perror("accept");
          continue;
        }

        epoll_event client_ev;
        client_ev.events = EPOLLIN;
        client_ev.data.fd = newfd;

        epoll_ctl(epoll_fd, EPOLL_CTL_ADD, newfd, &client_ev);

        std::cout << "New client connected: " << newfd << std::endl;
      }
      else
      {
        char buffer[1024];

        int bytes = recv(fd, buffer, sizeof(buffer) - 1, 0);

        if (bytes <= 0)
        {
          std::cout << "Client disconnected: " << fd << std::endl;

          close(fd);
          epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, NULL);
        }
        else
        {
          buffer[bytes] = '\0';

          std::cout << "Received from " << fd << ": " << buffer << std::endl;

          std::string input(buffer);
          std::vector<std::string> args = parseRESP(input);

          if (args.empty())
          {
            std::string response = "-ERR invalid RESP format\r\n";
            send(fd, response.c_str(), response.length(), 0);
            continue;
          }

          std::string command = args[0];
          std::string response;

          if (command == "SET")
          {
            if (args.size() < 3)
            {
              response = "-ERR wrong number of arguments\r\n";
            }
            else
            {
              std::string key = args[1];
              std::string value = args[2];
              int ttl = -1;

              if (args.size() == 4)
              {
                ttl = std::stoi(args[3]);
              }
              if (ttl > 0)
                expiry[key] = time(NULL) + ttl;

              store[key] = value;
              aof << "SET " << key << " " << value;
              if (ttl > 0)
                aof << " " << ttl;
              aof << std::endl;
              aof.flush();

              response = "+OK\r\n";
            }
          }
          else if (command == "GET")
          {
            if (args.size() != 2)
            {
              response = "-ERR wrong number of arguments \r\n";
            }
            else
            {
              std::string key = args[1];
              if (expiry.find(key) != expiry.end())
              {
                if (time(NULL) > expiry[key])
                {
                  store.erase(key);
                  expiry.erase(key);
                  response = "$-1\r\n";
                  send(fd, response.c_str(), response.length(), 0);
                  continue;
                }
              }
              if (store.find(key) != store.end())
              {
                std::string value = store[key];
                response = "$" + std::to_string(value.size()) + "\r\n" + value + "\r\n";
              }
              else
              {
                response = "$-1\r\n";
              }
            }
          }
          else if (command == "DEL")
          {
            if (args.size() != 2)
            {
              response = "-ERR wrong number of arguments\r\n";
            }
            else
            {
              std::string key = args[1];

              store.erase(key);
              expiry.erase(key);

              aof << "DEL " << key << std::endl;
              aof.flush();

              response = ":1\r\n";
            }
          }
          else
          {
            response = "-ERR unknown command\r\n";
          }

          send(fd, response.c_str(), response.length(), 0);
        }
      }
    }
  }
  close(serverSocket);
  return 0;
}
