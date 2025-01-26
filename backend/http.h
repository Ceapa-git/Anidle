#ifndef RESPONSE_H
#define RESPONSE_H

#include "types.h"

#include <string>

HttpRequest parseHttpRequest(const std::string&);
std::string createHttpRequest(const std::string&, const HttpRequest&);

std::string createResponse(ResponseStatus, Body);

#endif // RESPONSE_H
