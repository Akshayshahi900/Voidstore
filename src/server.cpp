#include <iostream>
#include <cstring>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <unordered_map>
#include <string>
#include <sstream>

std::unordered_map<std::string, std::string> store;

int main()
{
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

  sockaddr_in clientAddress{};
  socklen_t clientSize = sizeof(clientAddress);

  // 6. Main server loop
  while (true)
  {
    int clientSocket = accept(serverSocket,
                              (struct sockaddr *)&clientAddress,
                              &clientSize);

    if (clientSocket < 0)
    {
      perror("Accept failed");
      continue;
    }

    std::cout << "Client connected\n";

    char buffer[1024];

    // Per-client loop
    while (true)
    {
      memset(buffer, 0, sizeof(buffer));

      int bytesReceived = recv(clientSocket,
                               buffer,
                               sizeof(buffer) - 1,
                               0);

      if (bytesReceived <= 0)
      {
        std::cout << "Client disconnected\n";
        break;
      }

      buffer[bytesReceived] = '\0';

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
        response = "DELETED\n";
      }
      else
      {
        response = "UNKNOWN COMMAND\n";
      }

      send(clientSocket,
           response.c_str(),
           response.length(),
           0);
    }

    close(clientSocket);
  }

  close(serverSocket);
  return 0;
}
