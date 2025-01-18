#include "server.h"

#include <iostream>
#include <string>
#include <cstring>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

bool running = false;

int validateOptions(ServerOptions options) {
  if (options.port < 1000 || options.port > 9999) {
    std::cerr << "Options not in range (1000,9999).\n";
    return 1;
  }

  return 0;
}

int createServer(ServerOptions options) {
  if (validateOptions(options) != 0) {
    return 1;
  }

  int serverFd = socket(AF_INET, SOCK_STREAM, 0);
  if (serverFd < 0) {
    std::cerr << "Failed to create socket.\n";
    return 1;
  }

  int opt = 1;
  if (setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
    std::cerr << "Failed to set socket options.\n";
    close(serverFd);
    return 1;
  }

  sockaddr_in serverAddr;
  std::memset(&serverAddr, 0, sizeof(serverAddr));
  serverAddr.sin_family = AF_INET;
  serverAddr.sin_addr.s_addr = INADDR_ANY;
  serverAddr.sin_port = htons(options.port);

  if (bind(serverFd, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
    std::cerr << "Failed to bind socket.\n";
    close(serverFd);
    return 1;
  }

  if (listen(serverFd, 10) < 0) {
    std::cerr << "Failed to liste.\n";
    close(serverFd);
    return 1;
  }

  std::cout << "Server is listening on port " << options.port << " ...\n";
  running = true;

  while (running) {
    sockaddr_in clientAddr;
    socklen_t clientLen = sizeof(clientAddr);

    int clientFd = accept(serverFd, (struct sockaddr*)&clientAddr, &clientLen);
    if (clientFd < 0) {
      std::cerr << "Failed to accept connection.\n";
      continue;
    }

    char buffer[1024];
    std::memset(buffer, 0, sizeof(buffer));
    ssize_t bytesReceived = recv(clientFd, buffer, sizeof(buffer) - 1, 0);
    if (bytesReceived < 0) {
      std::cerr << "Failed to receive data.\n";
      close(clientFd);
      continue;
    }

    std::cout << "Received request:\n" << buffer << "\n";

    std::string response =
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/html\r\n"
            "Connection: close\r\n"
            "\r\n"
            "<!DOCTYPE html>"
            "<html>"
            "<head><title>Simple C++ HTTP Server</title></head>"
            "<body><h1>Hello, World!</h1></body>"
            "</html>";

    ssize_t bytes_sent = send(clientFd, response.c_str(), response.size(), 0);
    if (bytes_sent < 0) {
        std::cerr << "Failed to send response.\n";
    }

    close(clientFd);
  }
  
  close(serverFd);
  return 0;
}

