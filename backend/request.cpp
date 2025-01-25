#include "request.h"
#include "types.h"

#include <sstream>
#include <iostream>
#include <string>

Method stringToHttpMethod(const std::string& str) {
  if (str == "GET") return Method::GET;
  if (str == "POST") return Method::POST;
  if (str == "PUT") return Method::PUT;
  if (str == "DELETE") return Method::DELETE;
  return Method::UNKNOWN;
}

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

HttpRequest parseHttpRequest(char* buffer, int len) {
  std::string raw(buffer, len);

  HttpRequest request;
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

