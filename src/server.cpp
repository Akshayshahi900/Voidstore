#include <iostream>
#include <cstring>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <unordered_map>
#include <string>
#include <sstream>
#include <fstream>

std::unordered_map<std::string, std::string> store;
std::ofstream aof("db.aof", std::ios::app);

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
      iss >> key >> value;
      store[key] = value;
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
  fd_set master;
  fd_set read_fds;

  FD_ZERO(&master);
  FD_ZERO(&read_fds);

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

  FD_SET(serverSocket, &master);
  int fdmax = serverSocket;

  std::cout << "Server listening on port 8080...\n";

  sockaddr_in clientAddress{};
  socklen_t clientSize = sizeof(clientAddress);

  // 6. Main server loop
  while (true)
  {
    read_fds = master;

    if (select(fdmax + 1, &read_fds, NULL, NULL, NULL) == -1)
    {
      perror("select");
      exit(1);
    }

    for (int i = 0; i <= fdmax; i++)
    {
      if (FD_ISSET(i, &read_fds))
      {
        if (i == serverSocket)
        {
          sockaddr_in clientAddress{};
          socklen_t clientSize = sizeof(clientAddress);

          int newfd = accept(serverSocket,
                             (struct sockaddr *)&clientAddress,
                             &clientSize);

          if (newfd == -1)
          {
            perror("accept");
          }
          else
          {
            FD_SET(newfd, &master);

            if (newfd > fdmax)
              fdmax = newfd;

            std::cout << "New Client Connected: " << newfd << std::endl;
          }
        }
        else
        {
          char buffer[1024];

          int bytes = recv(i, buffer, sizeof(buffer) - 1, 0);

          if (bytes <= 0)
          {
            std::cout << "Client disconnected: " << i << std::endl;

            close(i);
            FD_CLR(i, &master);
          }
          else
          {
            buffer[bytes] = '\0';

            std::cout << "Received from " << i << ": " << buffer << std::endl;

            std::string input(buffer);
            std::istringstream iss(input);
            std::string command;
            iss >> command;

            std::string response;
            if (command == "SET")
            {
              std::string key, value;
              iss >> key >> value;

              store[key] = value;
              aof << "SET " << key << " " << value << std::endl;
              aof.flush();
              response = "OK\n";
            }
            else if (command == "GET")
            {
              std::string key;
              iss >> key;

              if (store.find(key) != store.end())
              {
                response = store[key] + "\n";
              }
              else
              {
                response = "NOT FOUND\n";
              }
            }
            else if (command == "DEL")
            {
              std::string key;
              iss >> key;

              store.erase(key);
              aof << "DEL " << key << std::endl;
              aof.flush();
              response = "DELETED\n";
            }
            else
            {
              response = "UNKNOWN COMMAND\n";
            }
            send(i, response.c_str(), response.length(), 0);
          }
        }
      }
    }
  }
  close(serverSocket);
  return 0;
}
