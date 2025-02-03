#ifndef RESPONSE_H
#define RESPONSE_H

#include "types.h"

#include <string>

std::string urlDecode(const std::string&);
std::string urlEncode(const std::string&);
Json parseBody(std::istream&);

HttpObject parseRequest(const std::string&);
std::string createRequest(const std::string&, const HttpObject&);

HttpObject parseResponse(const std::string&);
std::string createResponse(ResponseStatus, Json);

#endif // RESPONSE_H
