#ifndef SERVER_H
#define SERVER_H

#include "route.h"

struct ServerOptions {
  int port;
  bool debug = false;
  Route* routes;
};

int createServer(const ServerOptions&);

#endif // SERVER_H
