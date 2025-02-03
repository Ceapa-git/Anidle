#include "json.h"
#include "types.h"

#include <cctype>
#include <stdexcept>
#include <bsoncxx/types.hpp>
#include <bsoncxx/oid.hpp>
#include <sstream>

static void skipWhitespace(std::istream& is) {
  while (std::isspace(is.peek())) {
    is.get();
  }
}

static bool startsWith(std::istream& is, const std::string& token) {
  std::string temp(token.size(), '\0');
  is.read(&temp[0], token.size());
  if (!is.good()) {
    for (int i = static_cast<int>(temp.size()) - 1; i >= 0; --i) {
      if (temp[i] != '\0') {
        is.putback(temp[i]);
      }
    }
    return false;
  }
  if (temp == token) {
    return true;
  }
  for (int i = static_cast<int>(temp.size()) - 1; i >= 0; --i) {
    is.putback(temp[i]);
  }
  return false;
}

static std::string parseStringValue(std::istream& is) {
  std::string result;
  while (true) {
    char c = static_cast<char>(is.get());
    if (!is.good()) {
      throw std::runtime_error("Unexpected end of stream in string literal");
    }
    if (c == '"') {
      break;
    }
    if (c == '\\') {
      char e = static_cast<char>(is.get());
      if (!is.good()) {
        throw std::runtime_error("Unexpected end of stream after backslash");
      }
      switch (e) {
      case '"': result.push_back('"'); break;
      case '\\': result.push_back('\\'); break;
      case '/': result.push_back('/'); break;
      case 'b': result.push_back('\b'); break;
      case 'f': result.push_back('\f'); break;
      case 'n': result.push_back('\n'); break;
      case 'r': result.push_back('\r'); break;
      case 't': result.push_back('\t'); break;
      default: result.push_back(e); break;
      }
    } else {
      result.push_back(c);
    }
  }
  return result;
}

static bsoncxx::types::b_oid parseOidFromHex(const std::string& hex) {
  if (hex.size() != 24) {
    throw std::runtime_error("Invalid OID string length");
  }
  bsoncxx::oid boid(hex);
  return bsoncxx::types::b_oid{boid};
}

static Json parseValue(std::istream& is);

static Json parseObject(std::istream& is) {
  Json body;
  body.type = Json::Type::OBJECT;
  skipWhitespace(is);
  if (is.peek() == '}') {
    is.get();
    return body;
  }
  while (true) {
    skipWhitespace(is);
    if (is.get() != '"') {
      throw std::runtime_error("Expected '\"'");
    }
    std::string key = parseStringValue(is);
    skipWhitespace(is);
    if (is.get() != ':') {
      throw std::runtime_error("Expected ':'");
    }
    skipWhitespace(is);
    body.object[key] = parseValue(is);
    skipWhitespace(is);
    char next = static_cast<char>(is.peek());
    if (next == '}') {
      is.get();
      break;
    } else if (next == ',') {
      is.get();
      continue;
    } else {
      throw std::runtime_error("Expected ',' or '}'");
    }
  }
  if (body.object.size() == 1) {
    auto it = body.object.find("$oid");
    if (it != body.object.end() && it->second.type == Json::Type::VALUE) {
      body.type = Json::Type::OID;
      body.oid = parseOidFromHex(it->second.value);
      body.object.clear();
    }
  }
  return body;
}

static Json parseArray(std::istream& is) {
  Json body;
  body.type = Json::Type::ARRAY;
  skipWhitespace(is);
  if (is.peek() == ']') {
    is.get();
    return body;
  }
  while (true) {
    skipWhitespace(is);
    body.array.push_back(parseValue(is));
    skipWhitespace(is);
    char next = static_cast<char>(is.peek());
    if (next == ']') {
      is.get();
      break;
    } else if (next == ',') {
      is.get();
      continue;
    } else {
      throw std::runtime_error("Expected ',' or ']'");
    }
  }
  return body;
}

static Json parseValue(std::istream& is) {
  skipWhitespace(is);
  if (!is.good()) {
    throw std::runtime_error("No more input");
  }
  char c = static_cast<char>(is.peek());
  if (c == '{') {
    is.get();
    return parseObject(is);
  } else if (c == '[') {
    is.get();
    return parseArray(is);
  } else if (c == '"') {
    Json body;
    body.type = Json::Type::VALUE;
    is.get();
    body.value = parseStringValue(is);
    return body;
  } else if (std::isdigit(c) || c == '-') {
    Json body;
    body.type = Json::Type::VALUE;
    std::string numStr;
    while (std::isdigit(is.peek()) || is.peek() == '-' || is.peek() == '.' ||
           is.peek() == 'e' || is.peek() == 'E' || is.peek() == '+') {
      numStr.push_back(static_cast<char>(is.get()));
    }
    body.value = numStr;
    return body;
  } else if (startsWith(is, "true")) {
    Json body;
    body.type = Json::Type::VALUE;
    body.value = "true";
    return body;
  } else if (startsWith(is, "false")) {
    Json body;
    body.type = Json::Type::VALUE;
    body.value = "false";
    return body;
  } else if (startsWith(is, "null")) {
    Json body;
    body.type = Json::Type::VALUE;
    body.value = "null";
    return body;
  } else {
    throw std::runtime_error("Unexpected character");
  }
}

Json buildJson(std::istream& stream) {
  skipWhitespace(stream);
  Json root = parseValue(stream);
  skipWhitespace(stream);
  return root;
}

static std::string escapeString(const std::string& input) {
  std::ostringstream oss;
  for (char c : input) {
    switch (c) {
    case '"':  oss << "\\\""; break;
    case '\\': oss << "\\\\"; break;
    case '\b': oss << "\\b";  break;
    case '\f': oss << "\\f";  break;
    case '\n': oss << "\\n";  break;
    case '\r': oss << "\\r";  break;
    case '\t': oss << "\\t";  break;
    default:
      if (static_cast<unsigned char>(c) < 0x20) {
        oss << "\\u00" << std::hex << (int)(unsigned char)c;
      } else {
        oss << c;
      }
      break;
    }
  }
  return oss.str();
}

static bool isBooleanOrNull(const std::string& v) {
  return (v == "true" || v == "false" || v == "null");
}

static bool isNumber(const std::string& v) {
  if (v.empty()) return false;
  size_t i = 0;
  if (v[i] == '-' || v[i] == '+') {
    i++;
    if (i >= v.size()) return false;
  }
  bool dotFound = false;
  bool expFound = false;
  bool hasDigits = false;
  for (; i < v.size(); ++i) {
    char c = v[i];
    if (std::isdigit(c)) {
      hasDigits = true;
      continue;
    }
    if (c == '.' && !dotFound && !expFound) {
      dotFound = true;
      continue;
    }
    if ((c == 'e' || c == 'E') && !expFound && hasDigits) {
      expFound = true;
      if (i + 1 < v.size() && (v[i + 1] == '+' || v[i + 1] == '-')) {
        i++;
      }
      if (i + 1 >= v.size()) return false;
      continue;
    }
    return false;
  }
  return hasDigits;
}

static std::string jsonToStringImpl(const Json& json) {
  switch (json.type) {
  case Json::Type::VALUE: {
    const std::string& v = json.value;
    if (isBooleanOrNull(v) || isNumber(v)) {
      return v;
    } else {
      return "\"" + escapeString(v) + "\"";
    }
  }
  case Json::Type::OBJECT: {
    std::ostringstream oss;
    oss << "{";
    bool first = true;
    for (const auto& kv : json.object) {
      if (!first) {
        oss << ",";
      }
      first = false;
      oss << "\"" << escapeString(kv.first) << "\":" << jsonToStringImpl(kv.second);
    }
    oss << "}";
    return oss.str();
  }
  case Json::Type::ARRAY: {
    std::ostringstream oss;
    oss << "[";
    bool first = true;
    for (auto& el : json.array) {
      if (!first) {
        oss << ",";
      }
      first = false;
      oss << jsonToStringImpl(el);
    }
    oss << "]";
    return oss.str();
  }
  case Json::Type::OID: {
    bsoncxx::oid rawOid(json.oid.value);
    std::string hexStr = rawOid.to_string();
    std::ostringstream oss;
    oss << "{\"$oid\":\"" << escapeString(hexStr) << "\"}";
    return oss.str();
  }
  }
  return "null";
}

std::string jsonToString(const Json& json) {
  return jsonToStringImpl(json);
}

