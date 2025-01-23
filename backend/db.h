#ifndef DB_H
#define DB_H

#include <mongocxx/database.hpp>
#include <mongocxx/client.hpp>
#include <string>

mongocxx::client createDBClient(std::string);
void createCollection(mongocxx::database&, std::string);

#endif // DB_H
