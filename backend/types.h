#ifndef TYPE_H
#define TYPE_H

#include <string>
#include <map>
#include <functional>

enum Method {
  NONE = 0,
  GET = 1,
  POST = 2,
  PUT = 4,
  DELETE = 8,
  UNKNOWN = 16
};

struct Body {
  enum class Type {
    VALUE,
    OBJECT,
    ARRAY
  };

  Type type;
  std::string value;
  std::map<std::string, Body> object;
  std::vector<Body> array;
};

struct Route {
  Route* next = nullptr;
  Route* nested = nullptr;
  std::string path = "";
  Method method = Method::NONE;
  std::function<std::string(const std::string&, const Body&)> handler = nullptr; // query params, and body
}; 

struct HttpRequest {
  Method method;
  std::string methodStr;
  std::string path;
  std::string queryParams;
  std::map<std::string, std::string> headers;
  Body body;
};

struct ServerOptions {
  int port;
  bool debug = false;
  Route* routes;
};

enum ResponseStatus {
  OK = 200,
  NOT_FOUND = 404
};

#endif // TYPE_H
