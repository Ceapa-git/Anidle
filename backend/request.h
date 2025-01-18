#ifndef REQUEST_H
#define REQUEST_H

#include "route.h"

#include <string>
#include <map>

struct Body {
  enum class Type {
    VALUE,
    OBJECT
  };

  Type type;
  std::string value;
  std::map<std::string, Body> object;
};

struct HttpRequest {
  Method method;
  std::string methodStr;
  std::string path;
  std::string queryParams;
  std::map<std::string, std::string> headers;
  Body body;
};

HttpRequest parseHttpRequest(char*, int);

#endif // REQUEST_H

