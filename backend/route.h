#ifndef ROUTE
#define ROUTE

#include <string>
#include <functional>

enum Method {
  GET = 1,
  POST = 2,
  PUT = 4,
  DELETE = 8
};

struct Route {
  Route* next = nullptr;
  Route* nested = nullptr;
  std::string path = "";
  int method = 0;
  std::function<void(const std::string&, const std::string&)> handler = nullptr; // query params, and body
}; 

bool hasMethod(const Route&, Method);

#endif // !ROUTE
