#include "server.h"

int main() {
  ServerOptions options;
  options.port = 8080;
  createServer(options);
  return 0;
}
