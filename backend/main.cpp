#include "response.h"
#include "server.h"
#include "types.h"
#include "db.h"
#include "jwt.h"

#include <iostream>
#include <fstream>
#include <ostream>
#include <streambuf>
#include <string>
#include <cstdlib>
#include <bsoncxx/json.hpp>
#include <bsoncxx/builder/stream/document.hpp>
#include <mongocxx/client.hpp>
#include <mongocxx/instance.hpp>
#include <mongocxx/database.hpp>

struct CoutBuf : public std::streambuf {
  std::ofstream& file;
  std::streambuf* original;
  CoutBuf(std::ofstream& f, std::streambuf* orig) : file(f), original(orig) {}
  int overflow(int c) override {
    if (c != EOF) {
      file << "\033[37m" << static_cast<char>(c) << "\033[0m"; // White
      file.flush();
      original->sputc(c);
    }
    return c;
  }
};

struct CerrBuf : public std::streambuf {
  std::ofstream& file;
  std::streambuf* original;
  CerrBuf(std::ofstream& f, std::streambuf* orig) : file(f), original(orig) {}
  int overflow(int c) override {
    if (c != EOF) {
      file << "\033[31m" << static_cast<char>(c) << "\033[0m"; // Red
      file.flush();
      original->sputc(c);
    }
    return c;
  }
};

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
  // TODO
  return true;
}
bool validateRequestValidate(const HttpRequest& request) {
  // TODO
  return true;
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
  login.handler = [](const HttpRequest& request) {
    if (!validateRequestLogin(request)) {
      Body invalid;
      invalid.type = Body::Type::VALUE;
      invalid.value = "not a valid login body";
      return createResponse(BAD_REQUEST, invalid);
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
      10
    );
    return createResponse(OK, token);
  };
  base.nested = &login;

  Route validate;
  validate.path = "validate";
  validate.method = Method::GET;
  validate.handler = [](const HttpRequest& request) {
    if (!validateRequestValidate(request)) {
      Body invalid;
      invalid.type = Body::Type::VALUE;
      invalid.value = "not a valid login body";
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
  login.next = &validate;

  // routes -----------------------------------------------------------------------

  options.routes = &base;

  if (!logToFile) {
    createServer(options);
    return 0;
  }

  std::streambuf* coutBuf = std::cout.rdbuf();
  std::streambuf* cerrBuf = std::cerr.rdbuf();

  std::ofstream logFile("log");
  if (!logFile.is_open()) {
    std::cerr << "Failed to open log file!\n";
    return 1;
  }
  CoutBuf coutLogBuf(logFile, coutBuf);
  CerrBuf cerrLogBuf(logFile, cerrBuf);

  std::cout.rdbuf(&coutLogBuf);
  std::cerr.rdbuf(&cerrLogBuf);

  createServer(options);

  std::cout.rdbuf(coutBuf);
  std::cerr.rdbuf(cerrBuf);
  logFile.close();
  return 0;
}
