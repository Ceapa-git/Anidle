#include "db.h"
#include <mongocxx/client.hpp>
#include <mongocxx/uri.hpp>
#include <mongocxx/instance.hpp>
#include <mongocxx/client.hpp>

mongocxx::client createDBClient(std::string uri) {
  mongocxx::uri mongoUri(uri);
  mongocxx::client client(mongoUri);
  return client;
}
