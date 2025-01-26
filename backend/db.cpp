#include "db.h"
#include "http.h"

#include <iostream>
#include <mongocxx/client.hpp>
#include <mongocxx/uri.hpp>
#include <mongocxx/client.hpp>
#include <mongocxx/database.hpp>
#include <bsoncxx/json.hpp>
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

Body parseDocument(const bsoncxx::stdx::optional<bsoncxx::document::value>& document) {
  std::string str = bsoncxx::to_json(document->view());
  std::cerr << "json: " << str << "\n";
  std::istringstream stream(str);
  return parseBody(stream);
}

