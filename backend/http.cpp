#include "http.h"
#include "types.h"

#include <iomanip>
#include <regex>
#include <sstream>
#include <iostream>
#include <string>

// common ------------------------------------------------------ common

Body parseBody(std::istream& stream) {
  Body body;
  body.type = Body::Type::OBJECT;

  std::string line;
  while (std::getline(stream, line)) {
    line.erase(0, line.find_first_not_of(" \t\r\n"));
    line.erase(line.find_last_not_of(" ,\t\r\n") + 1);
    size_t colonPos = line.find(':');
    if (colonPos != std::string::npos) {
      std::string key = line.substr(1, colonPos - 2);
      key.erase(key.find_last_not_of(" \t\r\n") + 1);

      std::string value = line.substr(colonPos + 1);
      value.erase(0, value.find_first_not_of(" \t\r\n"));

      if (value == "{") {
        Body nestedBody = parseBody(stream);
        body.object[key] = nestedBody;
      } else if (value == "}") {
        break;
      } else if (value == "[") {
        Body arrayBody;
        arrayBody.type = Body::Type::ARRAY;

        while (std::getline(stream, line)) {
          line.erase(0, line.find_first_not_of(" \t\r\n"));
          line.erase(line.find_last_not_of(" ,\t\r\n") + 1);

          if (line == "]") {
            break;
          } else if (line == "{") {
            Body nestedBody = parseBody(stream);
            arrayBody.array.push_back(nestedBody);
          } else {
            Body valueBody;
            valueBody.type = Body::Type::VALUE;
            valueBody.value = line;
            arrayBody.array.push_back(valueBody);
          }
        }

        body.object[key] = arrayBody;
      } else {
        Body valueBody;
        valueBody.type = Body::Type::VALUE;
        valueBody.value = value;
        body.object[key] = valueBody;
      }
    }
  }

  return body;
}

bool isHex(const std::string& str) {
  static std::string hexDigits = "0123456789ABCDEFabcdef";
  for (const auto& c : str) {
    if (hexDigits.find(c) == std::string::npos) {
      return false;
    }
  }
  return true;
}

std::string urlDecode(const std::string& encoded) {
  std::ostringstream decoded;
  for (size_t i = 0; i < encoded.length(); ++i) {
    if (encoded[i] == '%' && i + 2 < encoded.length() && isHex(encoded.substr(i + 1, 2))) {
      std::string hex = encoded.substr(i + 1, 2);
      decoded << static_cast<char>(std::stoi(hex, nullptr, 16));
      i += 2;
    } else if (encoded[i] == '+') {
      decoded << ' ';
    } else {
      decoded << encoded[i];
    }
  }
  return decoded.str();
}

std::string urlEncode(const std::string& raw) {
  std::ostringstream encoded;

  for (unsigned char c : raw) {
    if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
      encoded << c;
    } else if (c == ' ') {
      encoded << '+';
    } else {
      encoded << '%' << std::uppercase << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(c);
    }
  }
  return encoded.str();
}

bool isNumber(const std::string& str) {
  std::regex number("^[+-]?([0-9]*[.])?[0-9]+$");
  return std::regex_match(str, number);
}

std::string bodyToString(const Body& body) {
  std::string str;
  if (body.type == Body::Type::OBJECT) {
    str += "{";
    bool pop = false;
    for (const auto& [key, value] : body.object) {
      str += "\"" + key + "\":" + bodyToString(value) + ",";
      pop = true;
    }
    if (pop) {
      str.pop_back();
    }
    str += "}";
  } else if (body.type == Body::Type::VALUE) {
    if (isNumber(body.value)) {
      str += body.value;
    } else if (body.value == "true" || body.value == "false") {
      str += body.value;
    } else {
      str += "\"" + body.value + "\"";
    }
  } else if (body.type == Body::Type::ARRAY) {
    str += "[";
    bool pop = false;
    for (const auto& value : body.array) {
      str += bodyToString(value) + ",";
      pop = true;
    }
    if (pop) {
      str.pop_back();
    }
    str += "]";
  }
  return str;
}

// !common ---------------------------------------------------- !common

// request ---------------------------------------------------- request

Method stringToHttpMethod(const std::string& str) {
  if (str == "GET") return Method::GET;
  if (str == "POST") return Method::POST;
  if (str == "PUT") return Method::PUT;
  if (str == "DELETE") return Method::DELETE;
  return Method::UNKNOWN;
}

std::map<std::string, std::string> parseQueryParams(const std::string& query) {
  std::map<std::string, std::string> queryParams;

  std::istringstream queryStream(query);
  std::string pair;

  while (std::getline(queryStream, pair, '&')) {
    size_t equalPos = pair.find('=');
    std::string key, value;

    if (equalPos != std::string::npos) {
      key = urlDecode(pair.substr(0, equalPos));
      value = urlDecode(pair.substr(equalPos + 1));
    } else {
      key = urlDecode(pair);
    }

    queryParams[key] = value;
  }

  return queryParams;
}

HttpObject parseRequest(const std::string& raw) {
  HttpObject request;
  std::istringstream stream(raw);
  std::string line;

  if (std::getline(stream, line)) {
    std::istringstream requestLine(urlDecode(line));
    std::string methodStr, url, httpVersion;

    requestLine >> methodStr >> url >> httpVersion;
    request.method = stringToHttpMethod(methodStr);
    request.methodStr = methodStr;

    size_t queryPos = url.find('?');
    if (queryPos != std::string::npos) {
      request.path = url.substr(0, queryPos);
      request.queryParams = parseQueryParams(url.substr(queryPos + 1));
    } else {
      request.path = url;
    }
  }

  while (std::getline(stream, line) && line != "\r") {
    size_t colonPos = line.find(':');
    if (colonPos != std::string::npos) {
      std::string key = line.substr(0, colonPos);
      std::string value = line.substr(colonPos + 1);
      key.erase(0, key.find_first_not_of(" \t\r\n"));
      key.erase(key.find_last_not_of(" \t\r\n") + 1);
      value.erase(0, value.find_first_not_of(" \t\r\n"));
      value.erase(value.find_last_not_of(" \t\r\n") + 1);
      request.headers[key] = value;
    }
  }

  request.body = parseBody(stream);

  return request;
}

std::string createRequest(const std::string& host, const HttpObject& request) {
  return "";
}

// !request -------------------------------------------------- !request

// response -------------------------------------------------- response

HttpObject parseResponse(const std::string& raw) {
  HttpObject request;
  std::istringstream stream(raw);
  std::string line;

  while (std::getline(stream, line) && line != "\r"); // if headers required modify
  
  request.body = parseBody(stream);

  return request;
}

std::string createResponse(ResponseStatus status, Body body) {
  std::string response = "HTTP/1.1 ";

  switch (status) {
    case OK:
      response += "200 OK\r\n";
      break;
    case NOT_FOUND:
      response += "404 Not Found\r\n";
      break;
    case BAD_REQUEST:
      response += "400 Bad Request\r\n";
      break;
    case UNAUTHORIZED:
      response += "401 Unauthorized\r\n";
      break;
    case FORBIDDEN:
      response += "403 Forbidden\r\n";
      break;
    case CONFLICT:
      response += "409 Conflict\r\n";
      break;
    case INTERNAL_SERVER_ERROR:
      response += "500 Internal Server Error\r\n";
      break;
    default:
      response += "500 Internal Server Error\r\n"; // Fallback for unknown status
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

// !response ------------------------------------------------ !response

