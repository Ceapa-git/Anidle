#include "server.h"
#include "route.h"

#include <iostream>

int main() {
  ServerOptions options;
  options.port = 8080;

  Route base;
  base.path = "/";
  base.method = Method::GET;
  base.handler = [](const std::string& params, const std::string& body) {
    std::cout << "GET /";
  };

  createServer(options);
  return 0;
}
