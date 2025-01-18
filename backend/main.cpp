#include "server.h"
#include "route.h"

#include <iostream>

int main(int argc, char** argv) {
  ServerOptions options;
  options.port = 8080;
  if (argc > 1) {
    if (argv[1][0] == 'd' || argv[1][0] == 'D') {
      options.debug = true;
    }
  }

  Route base;
  base.path = "/";
  base.method = Method::GET;
  base.handler = [](const std::string& params, const std::string& body) {
    std::cout << "GET /";
  };

  createServer(options);
  return 0;
}
