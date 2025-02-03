#include "db.h"
#include "json.h"
#include "types.h"

#include <iostream>
#include <mongocxx/client.hpp>
#include <mongocxx/uri.hpp>
#include <mongocxx/client.hpp>
#include <mongocxx/database.hpp>
#include <bsoncxx/json.hpp>
#include <bsoncxx/builder/basic/document.hpp>
#include <bsoncxx/builder/basic/array.hpp>
#include <bsoncxx/builder/basic/kvp.hpp>
#include <bsoncxx/document/view.hpp>
#include <sstream>

mongocxx::client createDBClient(std::string uri) {
  mongocxx::uri mongoUri(uri);
  mongocxx::client client(mongoUri);
  return client;
}

void createCollection(mongocxx::database& db, std::string col) {
  if (!db.has_collection(col)) {
    db.create_collection(col);
  }
}

Json parseDocument(const bsoncxx::stdx::optional<bsoncxx::document::value>& document) {
  std::string str = bsoncxx::to_json(document->view());
  std::istringstream stream(str);
  return buildJson(stream);
}

bsoncxx::builder::basic::array buildArray(const Json&);
bsoncxx::builder::basic::document buildObject(const Json&);

bsoncxx::builder::basic::array buildArray(const Json& body) {
  if (body.type == Json::Type::ARRAY) {
    auto array = bsoncxx::builder::basic::array{};
    for (const auto& value : body.array) {
      if (value.type == Json::Type::VALUE) {
        array.append(value.value);
      } else if (value.type == Json::Type::OID) {
        array.append(value.oid);
      } else if (value.type == Json::Type::ARRAY) {
        array.append(buildArray(value));
      } else if (value.type == Json::Type::OBJECT) {
        array.append(buildObject(value));
      }
    }
    return array;
  } else {
    std::cerr << "[db.cpp:buildArray] body type untreated\n";
    throw std::runtime_error("buildArray failed");
  }
}
bsoncxx::builder::basic::document buildObject(const Json& body) {
  using bsoncxx::builder::basic::kvp;

  if (body.type == Json::Type::OBJECT) {
    auto object = bsoncxx::builder::basic::document{};
    for (const auto& [key, value] : body.object) {
      if (value.type == Json::Type::VALUE) {
        object.append(kvp(key, value.value));
      } else if (value.type == Json::Type::OID) {
        object.append(kvp(key, value.oid));
      } else if (value.type == Json::Type::ARRAY) {
        object.append(kvp(key, buildArray(value)));
      } else if (value.type == Json::Type::OBJECT) {
        object.append(kvp(key, buildObject(value)));
      }
    }
    return object;
  } else {
    std::cerr << "[db.cpp:buildObject] body type untreated\n";
    throw std::runtime_error("buildObject failed");
  }
}

bsoncxx::document::value createDocument(const Json& body) {
  if (body.type != Json::Type::OBJECT) {
    std::cerr << "[db.cpp:createDocument] cannot create document from non-object body\n";
    throw std::runtime_error("createDocument failed");
  }
  auto document = buildObject(body);
  return document.extract();
}

