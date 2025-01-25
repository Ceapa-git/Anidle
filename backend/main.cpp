#include "bsoncxx/builder/stream/document-fwd.hpp"
#include "bsoncxx/builder/stream/helpers.hpp"
#include "response.h"
#include "server.h"
#include "types.h"
#include "db.h"
#include "security.h"
#include "log.h"

#include <iostream>
#include <fstream>
#include <memory>
#include <ostream>
#include <string>
#include <cstdlib>
#include <bsoncxx/json.hpp>
#include <bsoncxx/builder/stream/document.hpp>
#include <mongocxx/client.hpp>
#include <mongocxx/instance.hpp>
#include <mongocxx/database.hpp>

ServerOptions options;
bool logToFile = false;
std::string apiToken;

void processCliArgs(int argc, char** argv) {
  for (int i = 1; i < argc; i++) {
    std::string arg(argv[i]);
    if (arg == "-d" || arg == "--debug") {
      options.debug = true;
    } else if (arg == "-l" || arg == "--log") {
      logToFile = true;
    }
  }
}

bool validateRequestLogin(const HttpRequest& request) {
  const auto& body = request.body;
  if (body.type != Body::Type::OBJECT) return false;
  if (body.object.find("username") == body.object.end() || 
      body.object.at("username").type != Body::Type::VALUE) return false;
  if (body.object.find("password") == body.object.end() || 
      body.object.at("password").type != Body::Type::VALUE) return false;
  return true;
}
bool validateLogin(const HttpRequest& request, const mongocxx::database& db) {
  const auto& body = request.body;
  std::string username = body.object.at("username").value;
  username.erase(0, 1);
  username.erase(username.length() - 1);
  const std::string& password = hashPassword(body.object.at("password").value);
  auto users = db["users"];
  auto filter = bsoncxx::builder::stream::document{}
    << "username" << username
    << "password" << password
    << bsoncxx::builder::stream::finalize;
  auto result = users.find_one(filter.view());
  return result ? true : false;
}
bool validateRequestRegister(const HttpRequest& request) {
  const auto& body = request.body;
  if (body.type != Body::Type::OBJECT) return false;
  if (body.object.find("username") == body.object.end() || 
      body.object.at("username").type != Body::Type::VALUE) return false;
  if (body.object.find("password") == body.object.end() || 
      body.object.at("password").type != Body::Type::VALUE) return false;
  return true;
}
bool validateRegister(const HttpRequest& request, const mongocxx::database& db) {
  const auto& body = request.body;
  std::string username = body.object.at("username").value;
  username.erase(0, 1);
  username.erase(username.length() - 1);
  auto users = db["users"];
  auto filter = bsoncxx::builder::stream::document{}
    << "username" << username
    << bsoncxx::builder::stream::finalize;
  auto result = users.find_one(filter.view());
  return result ? false : true;
}
void doRegister(const HttpRequest& request, const mongocxx::database& db) {
  const auto& body = request.body;
  std::string username = body.object.at("username").value;
  username.erase(0, 1);
  username.erase(username.length() - 1);
  const std::string& password = hashPassword(body.object.at("password").value);
  auto users = db["users"];
  bsoncxx::builder::stream::document doc_builder;
  doc_builder
    << "username" << username
    << "password" << password;
  users.insert_one(doc_builder.view());
}
bool validateRequestValidate(const HttpRequest& request) {
  return request.headers.find("Authorization") != request.headers.end();
}
bool validateRequestRefresh(const HttpRequest& request) {
  if (request.headers.find("Authorization") == request.headers.end()) return false;
  if (request.body.type != Body::Type::OBJECT  ||
      request.body.object.find("username") == request.body.object.end() ||
      request.body.object.at("username").type != Body::Type::VALUE) return false;
  return true;
}
bool checkSameOwnerOfJwt(const HttpRequest& request, const std::string& token) {
  std::string requestHost = request.headers.at("Host");
  std::string requestUsername = request.body.object.at("username").value;
  requestUsername.erase(0, 1);
  requestUsername.erase(requestUsername.length() - 1);

  auto [tokenHost, tokenUsername] = extractHostAndUsername(token);
  return requestHost == tokenHost && requestUsername == tokenUsername;
}

int main(int argc, char** argv) {
  options.port = 8080;
  processCliArgs(argc, argv);

  apiToken = std::getenv("TOKEN");
  if (options.debug) {
    std::cout << "Token: " << apiToken << "\n";
  }

  mongocxx::instance instance;
  mongocxx::client client = createDBClient(std::getenv("MONGO_URI"));
  mongocxx::database db = client[std::getenv("MONGO_DB")];
  createCollection(db, "users");
  createCollection(db, "dailies");
  createCollection(db, "scores");
  createCollection(db, "jwt");

  std::string jwtKey;

  try {
    if (db["jwt"].count_documents({}) == 0) {
      bsoncxx::builder::stream::document doc_builder;
      doc_builder << "key" << createJwtKey(32);
      db["jwt"].insert_one(doc_builder.view());
    }
    auto token = db["jwt"].find_one({});
    if (token) {
      jwtKey = token->view()["key"].get_string().value.data();
    }
  } catch (const std::exception& e) {
    std::cerr << "Error: " << e.what() << "\n";
  }

  setJwtKey(jwtKey);

  Route base;
  base.path = "";
  base.method = Method::GET;
  base.handler = [](const HttpRequest& request) {
    Body b;
    b.type = Body::Type::VALUE;
    b.value = "running";
    return createResponse(OK, b);
  };

  // routes -----------------------------------------------------------------------

  Route login;
  login.path = "login";
  login.method = Method::POST;
  login.handler = [&](const HttpRequest& request) {
    if (!validateRequestLogin(request)) {
      Body invalid;
      invalid.type = Body::Type::VALUE;
      invalid.value = "request not valid";
      return createResponse(BAD_REQUEST, invalid);
    }
    if (!validateLogin(request, db)) {
      Body invalid;
      invalid.type = Body::Type::VALUE;
      invalid.value = "username and password do not match";
      return createResponse(UNAUTHORIZED, invalid);
    }
    Body token;
    token.type = Body::Type::VALUE;
    std::string host = request.headers.at("Host");
    std::string username = request.body.object.at("username").value;
    username.erase(0, 1);
    username.erase(username.length() - 1);
    token.value = createJwt(
      host,
      username,
      60 * 30
    );
    return createResponse(OK, token);
  };
  base.nested = &login;

  Route registerUser;
  registerUser.path = "register";
  registerUser.method = Method::POST;
  registerUser.handler = [&](const HttpRequest& request) {
    if (!validateRequestRegister(request)) {
      Body invalid;
      invalid.type = Body::Type::VALUE;
      invalid.value = "request not valid";
      return createResponse(BAD_REQUEST, invalid);
    }
    if (!validateRegister(request, db)) {
      Body invalid;
      invalid.type = Body::Type::VALUE;
      invalid.value = "username already registered";
      return createResponse(CONFLICT, invalid);
    }
    doRegister(request, db);
    Body token;
    token.type = Body::Type::VALUE;
    std::string host = request.headers.at("Host");
    std::string username = request.body.object.at("username").value;
    username.erase(0, 1);
    username.erase(username.length() - 1);
    token.value = createJwt(
      host,
      username,
      60 * 30
    );
    return createResponse(OK, token);
  };
  login.next = &registerUser;

  Route validate;
  validate.path = "validate";
  validate.method = Method::POST;
  validate.handler = [](const HttpRequest& request) {
    if (!validateRequestValidate(request)) {
      Body invalid;
      invalid.type = Body::Type::VALUE;
      invalid.value = "request not valid";
      return createResponse(BAD_REQUEST, invalid);
    }
    std::string bearer = request.headers.at("Authorization");
    std::string token = bearer.substr(7);
    if (isJwtValid(token)) {
      Body valid;
      valid.type = Body::Type::VALUE;
      valid.value = "jwt valid";
      return createResponse(OK, valid);
    }
    Body invalid;
    invalid.type = Body::Type::VALUE;
    invalid.value = "jwt invalid";
    return createResponse(FORBIDDEN, invalid);
  };
  registerUser.next = &validate;

  Route refresh;
  refresh.path = "refresh";
  refresh.method = Method::POST;
  refresh.handler = [](const HttpRequest& request) {
    if (!validateRequestRefresh(request)) {
      Body invalid;
      invalid.type = Body::Type::VALUE;
      invalid.value = "request not valid";
      return createResponse(BAD_REQUEST, invalid);
    }
    std::string bearer = request.headers.at("Authorization");
    std::string token = bearer.substr(7);
    if (isJwtValid(token) && checkSameOwnerOfJwt(request, token)) {
      Body token;
      token.type = Body::Type::VALUE;
      std::string host = request.headers.at("Host");
      std::string username = request.body.object.at("username").value;
      username.erase(0, 1);
      username.erase(username.length() - 1);
      token.value = createJwt(
        host,
        username,
        60 * 30
      );
      return createResponse(OK, token);
    }
    Body invalid;
    invalid.type = Body::Type::VALUE;
    invalid.value = "jwt invalid";
    return createResponse(FORBIDDEN, invalid);
  };
  validate.next = &refresh;

  // end routes -------------------------------------------------------------------

  options.routes = &base;

  if (!logToFile) {
    createServer(options);
    return 0;
  }

  auto logFile = std::make_shared<std::ofstream>("log");
  if (!logFile->is_open()) {
    std::cerr << "Failed to open log file!\n";
    return 1;
  }

  {
    initLogCout(logFile);
    initLogCerr(logFile);

    createServer(options);
  }

  logFile->close();
  return 0;
}
