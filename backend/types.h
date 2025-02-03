#ifndef TYPE_H
#define TYPE_H

#include <bsoncxx/types.hpp>
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

// TODO check if any other function needs to be refactored and move inside json.cpp
struct Json {
  enum class Type {
    VALUE,
    OBJECT,
    ARRAY,
    OID
  };

  Type type;
  std::string value;
  std::map<std::string, Json> object;
  std::vector<Json> array;
  bsoncxx::types::b_oid oid;
};

struct HttpObject {
  std::string ip;
  Method method;
  std::string methodStr;
  std::string path;
  std::map<std::string, std::string> queryParams;
  std::map<std::string, std::string> headers;
  Json body;
};

struct Route {
  Route* next = nullptr;
  Route* nested = nullptr;
  std::string path = "";
  Method method = Method::NONE;
  std::function<std::string(const HttpObject&)> handler = nullptr; // query params, and body
}; 

struct ServerOptions {
  int port;
  bool debug = false;
  Route* routes;
};

enum ResponseStatus {
  OK = 200,
  BAD_REQUEST = 400,
  UNAUTHORIZED = 401,
  FORBIDDEN = 403,
  NOT_FOUND = 404,
  CONFLICT = 409,
  INTERNAL_SERVER_ERROR = 500
};

#endif // TYPE_H
