#ifndef SERVER
#define SERVER

#include "route.h"

struct ServerOptions {
  int port;
  Route* routes;
};

int createServer(const ServerOptions&);

#endif // !SERVER

