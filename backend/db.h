#ifndef DB_H
#define DB_H

#include "types.h"

#include <mongocxx/database.hpp>
#include <mongocxx/client.hpp>
#include <string>

mongocxx::client createDBClient(std::string);
void createCollection(mongocxx::database&, std::string);

Body parseDocument(const bsoncxx::stdx::optional<bsoncxx::document::value>&);

#endif // DB_H
