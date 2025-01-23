#include "db.h"
#include <mongocxx/client.hpp>
#include <mongocxx/uri.hpp>
#include <mongocxx/client.hpp>
#include <mongocxx/database.hpp>

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

