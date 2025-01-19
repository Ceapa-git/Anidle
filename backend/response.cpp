#include "response.h"
#include "types.h"
#include <regex>
#include <string>

bool isNumber(const std::string& str) {
  std::regex number("^[+-]?([0-9]*[.])?[0-9]+$");
  return std::regex_match(str, number);
}

std::string bodyToString(const Body& body) {
  std::string str;
  if (body.type == Body::Type::OBJECT) {
    str += "{";
    for (const auto& [key, value] : body.object) {
      str += "\"" + key + "\":" + bodyToString(value) + ",";
    }
    str.pop_back();
    str += "}";
  } else if (body.type == Body::Type::VALUE) {
    if (isNumber(body.value)) {
      str += body.value;
    } else if (body.value == "true" || body.value == "false") {
      str += body.value;
    } else {
      str += "\"" + body.value + "\"";
    }
  }
  return str;
}

std::string createResponse(ResponseStatus status, Body body) {
  std::string response;

  switch (status) {
  case OK:
    response += "HTTP/1.1 200 OK\r\n";
    break;
  case NOT_FOUND:
    response += "HTTP/1.1 404 Not Found\r\n";
    break;
  }

  response += "Connection: close\r\n";

  if (body.type == Body::Type::VALUE) {
    response += "Content-Type: text/plain\r\n";
    response += "Content-Length: " + std::to_string(body.value.length()) + "\r\n";
    response += "\r\n";
    response += body.value;
  } else if (body.type == Body::Type::OBJECT) {
    response += "Content-Type: application/json\r\n";
    response += "\r\n";
    response += bodyToString(body);
  }

  return response;
}
