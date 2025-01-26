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

HttpObject malRequest(const HttpObject& request) {
  std::string url = createRequest(host, request);
  
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
    freeaddrinfo(serverInfo);
    throw std::runtime_error("Failed to create socket");
  }

  if (connect(sock, serverInfo->ai_addr, serverInfo->ai_addrlen) == -1) {
    std::cerr << "Failed to connect\n";
    close(sock);
    freeaddrinfo(serverInfo);
    throw std::runtime_error("Failed to connect");
  }

  std::string apiRequest = "GET " + request.path;
  if (!request.queryParams.empty()) {
    apiRequest += "?";
    bool first = true;
    for (const auto& [key, value] : request.queryParams) {
      if (!first) apiRequest += "&";
      apiRequest += urlEncode(key) + "=" + urlEncode(value);
      first = false;
    }
  }

  apiRequest += " HTTP/1.1\r\n";
  apiRequest += "Host: " + host + "\r\n";
  apiRequest += "Connection: close\r\n\r\n";

  if (send(sock, apiRequest.c_str(), apiRequest.size(), 0) == -1) {
    std::cerr << "Failed to send request\n";
    close(sock);
    freeaddrinfo(serverInfo);
    throw std::runtime_error("Failed to send request");
  }

  std::string response = readFromSocket(sock);

  close(sock);
  freeaddrinfo(serverInfo);

  auto parsedRespone = parseResponse(response);
  
  return parsedRespone;
}

