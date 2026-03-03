#include <iostream>
#include <cstring>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

int main()
{
  // 1. Create socket
  int clientSocket = socket(AF_INET, SOCK_STREAM, 0);
  if (clientSocket < 0)
  {
    perror("Socket creation failed");
    return 1;
  }

  // 2. Configure server address
  sockaddr_in serverAddress{};
  serverAddress.sin_family = AF_INET;
  serverAddress.sin_port = htons(8080);

  // Convert IP string to binary
  if (inet_pton(AF_INET, "127.0.0.1", &serverAddress.sin_addr) <= 0)
  {
    perror("Invalid address");
    return 1;
  }

  // 3. Connect to server
  if (connect(clientSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0)
  {
    perror("Connection failed");
    return 1;
  }

  std::cout << "Connected to server\n";

  // 4. Send message
  const char *message = "Hello, server!";
  send(clientSocket, message, strlen(message), 0);

  // 5. Close socket
  close(clientSocket);

  return 0;
}
