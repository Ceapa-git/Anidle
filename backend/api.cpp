#include "api.h"
#include "http.h"
#include "types.h"
#include "server.h"

#include <iostream>
#include <stdexcept>
#include <string>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>

const std::string host = std::getenv("MAL_HOST");

std::string malRequest(const HttpRequest& request) {
  std::string url = createHttpRequest(host, request);
  
  struct addrinfo hints{}, *serverInfo;
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;

  if (getaddrinfo(host.c_str(), "80", &hints, &serverInfo) != 0) {
    std::cerr << "Failed to resolve host\n";
    throw std::runtime_error("Failed to resolve host");
  }

  int sock = socket(serverInfo->ai_family, serverInfo->ai_socktype, serverInfo->ai_protocol);
  if (sock == -1) {
    std::cerr << "Failed to create socket\n";
    throw std::runtime_error("Failed to create socket");
  }

  if (connect(sock, serverInfo->ai_addr, serverInfo->ai_addrlen) == -1) {
    std::cerr << "Failed to connect\n";
    close(sock);
    throw std::runtime_error("Failed to connect");
  }

  return "";
}

