#include "http.h"
#include "server.h"
#include "types.h"
#include "db.h"
#include "security.h"
#include "log.h"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <ios>
#include <iostream>
#include <fstream>
#include <memory>
#include <ostream>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <cstdlib>
#include <bsoncxx/json.hpp>
#include <bsoncxx/builder/stream/document.hpp>
#include <bsoncxx/builder/stream/helpers.hpp>
#include <bsoncxx/builder/basic/document.hpp>
#include <bsoncxx/builder/basic/kvp.hpp>
#include <mongocxx/client.hpp>
#include <mongocxx/instance.hpp>
#include <mongocxx/database.hpp>
#include <mongocxx/options/find.hpp>

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

bool validateRequestLogin(const HttpObject& request) {
  const auto& body = request.body;
  if (body.type != Body::Type::OBJECT) return false;
  if (body.object.find("username") == body.object.end() || 
      body.object.at("username").type != Body::Type::VALUE) return false;
  if (body.object.find("password") == body.object.end() || 
      body.object.at("password").type != Body::Type::VALUE) return false;
  return true;
}
bool validateLogin(const HttpObject& request, const mongocxx::database& db) {
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
bool validateRequestRegister(const HttpObject& request) {
  const auto& body = request.body;
  if (body.type != Body::Type::OBJECT) return false;
  if (body.object.find("username") == body.object.end() || 
      body.object.at("username").type != Body::Type::VALUE) return false;
  if (body.object.find("password") == body.object.end() || 
      body.object.at("password").type != Body::Type::VALUE) return false;
  return true;
}
bool validateRegister(const HttpObject& request, const mongocxx::database& db) {
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
void doRegister(const HttpObject& request, const mongocxx::database& db) {
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
bool validateRequestValidate(const HttpObject& request) {
  return request.headers.find("Authorization") != request.headers.end();
}
bool validateRequestRefresh(const HttpObject& request) {
  if (request.headers.find("Authorization") == request.headers.end()) return false;
  if (request.body.type != Body::Type::OBJECT  ||
      request.body.object.find("username") == request.body.object.end() ||
      request.body.object.at("username").type != Body::Type::VALUE) return false;
  return true;
}
bool checkSameOwnerOfJwt(const HttpObject& request, const std::string& token) {
  std::string requestHost = request.headers.at("Host");
  std::string requestUsername = request.body.object.at("username").value;
  requestUsername.erase(0, 1);
  requestUsername.erase(requestUsername.length() - 1);

  auto [tokenHost, tokenUsername] = extractHostAndUsername(token);
  return requestHost == tokenHost && requestUsername == tokenUsername;
}
std::string getCurrentDate() {
  auto now = std::chrono::system_clock::now();
  std::time_t nowTime = std::chrono::system_clock::to_time_t(now);
  std::tm* nowTm = std::localtime(&nowTime);

  std::ostringstream oss;
  oss << std::put_time(nowTm, "%d/%m/%Y");
  return oss.str();
}
bool isValidDate(const std::string& date) {
  std::regex dateRegex(R"(^\d{2}/\d{2}/\d{4}$)");
  if (!std::regex_match(date, dateRegex)) {
    return false;
  }

  int day, month, year;
  sscanf(date.c_str(), "%d/%d/%d", &day, &month, &year);

  if (year < 2025 || month < 1 || month > 12 || day < 1 || day > 31) {
    return false;
  }

  static const int daysInMonth[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
  if (month == 2 && year % 4 == 0 && (year % 100 != 0 || year % 400 == 0)) {
    if (day > 29) return false;
  } else if (day > daysInMonth[month - 1]) {
    return false;
  }

  return true;
}
int getRandom(int interval_max) {
  srand(time(nullptr));
  int random = rand();
  return (int)(((double) random) / RAND_MAX * (interval_max));
}
bool isTodayOrFuture(const std::string& date) {
  std::string currentDate = getCurrentDate();

  auto convertToComparable = [](const std::string& dateStr) {
    std::istringstream iss(dateStr);
    int day, month, year;
    sscanf(dateStr.c_str(), "%d/%d/%d", &day, &month, &year);
    return year * 10000 + month * 100 + day;
  };

  return convertToComparable(date) >= convertToComparable(currentDate);
}
Body getOrCreateDaily(const std::string& day, const mongocxx::database& db) {
  Body response;
  auto dailies = db["dailies"];

  bsoncxx::builder::stream::document filterBuilderDaily;
  filterBuilderDaily << "day" << day;
  auto document = dailies.find_one(filterBuilderDaily.view());
  
  if (!document) {
    auto animeDb = db["anime"];
    auto count = animeDb.count_documents({});
    int skip = 0;
    int difficulty = getRandom(9) + 1;
    std::string diff;
    if (difficulty <= 5) {
      skip = getRandom(499);
      diff = "easy";
    } else if (difficulty <= 8) {
      skip = getRandom(2999);
      diff = "medium";
    } else {
      skip = getRandom(5999);
      diff = "hard";
    }

    mongocxx::options::find optionsDaily{};
    optionsDaily.skip(skip).limit(1).projection(
      bsoncxx::builder::basic::make_document(
        bsoncxx::builder::basic::kvp(
          "_id", 1
        ), bsoncxx::builder::basic::kvp(
          "title", 1
        )
      )
    );
    auto animes = animeDb.find({}, optionsDaily);

    int i = 0;
    for (auto&& anime : animes) {
      i++;
      if (i != 1) {
        std::cerr << "[main.cpp:getOrCreateDaily] found none/more than 1 (" << i
          << "), skip: " << skip
          << ", day: " << day
          << ", count: " << count
          << "\n";
        throw std::runtime_error("getOrCreateDaily");
      }

      auto id = anime["_id"].get_oid();
      std::cout << "Created entry for " << day
        << " with anime: \"" << anime["title"].get_string().value.data()
        << "\" and difficulty: " << diff
        << "\n";
      Body daily;
      daily.type = Body::Type::OBJECT;
      daily.object["anime"].type = Body::Type::OID;
      daily.object["anime"].oid = id;
      daily.object["day"].type = Body::Type::VALUE;
      daily.object["day"].value = day;
      daily.object["difficulty"].type = Body::Type::VALUE;
      daily.object["difficulty"].value = diff;
      auto doc = createDocument(daily);
      std::cout << bsoncxx::to_json(doc) << "\n";
      dailies.insert_one(doc.view());
    }
  }

  return response;
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
  base.handler = [](const HttpObject& request) {
    Body b;
    b.type = Body::Type::VALUE;
    b.value = "running";
    return createResponse(OK, b);
  };

  // routes -----------------------------------------------------------------------

  Route login;
  login.path = "login";
  login.method = Method::POST;
  login.handler = [&](const HttpObject& request) {
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
    std::string username = request.body.object.at("username").value;
    username.erase(0, 1);
    username.erase(username.length() - 1);
    token.value = createJwt(
      request.ip,
      username,
      60 * 30
    );
    return createResponse(OK, token);
  };
  base.nested = &login;

  Route registerUser;
  registerUser.path = "register";
  registerUser.method = Method::POST;
  registerUser.handler = [&](const HttpObject& request) {
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
    std::string username = request.body.object.at("username").value;
    username.erase(0, 1);
    username.erase(username.length() - 1);
    token.value = createJwt(
      request.ip,
      username,
      60 * 30
    );
    return createResponse(OK, token);
  };
  login.next = &registerUser;

  Route validate;
  validate.path = "validate";
  validate.method = Method::POST;
  validate.handler = [](const HttpObject& request) {
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
  refresh.handler = [](const HttpObject& request) {
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
      std::string username = request.body.object.at("username").value;
      username.erase(0, 1);
      username.erase(username.length() - 1);
      token.value = createJwt(
        request.ip,
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

  Route daily;
  daily.path = "daily";
  daily.method = Method::GET;
  daily.handler = [&](const HttpObject& request) {
    std::string day = getCurrentDate();
    if (request.queryParams.find("day") != request.queryParams.end()) {
      day = request.queryParams.at("day");
      if (!isValidDate(day) || isTodayOrFuture(day)) {
        Body invalid;
        invalid.type = Body::Type::VALUE;
        invalid.value = "request not valid";
        return createResponse(BAD_REQUEST, invalid);
      }
    }
    Body response = getOrCreateDaily(day, db);
    return createResponse(OK, response);
  };
  refresh.next = &daily;

  // end routes -------------------------------------------------------------------

  options.routes = &base;

  if (!logToFile) {
    createServer(options);
    return 0;
  }

  auto logFile = std::make_shared<std::ofstream>("logs/backend", std::ios_base::app);
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
