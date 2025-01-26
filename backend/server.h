#ifndef SERVER_H
#define SERVER_H

#include "types.h"

std::string readFromSocket(int);

int createServer(const ServerOptions&);

#endif // SERVER_H
