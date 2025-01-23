#ifndef DB_H
#define DB_H

#include "mongocxx/client-fwd.hpp"

#include <mongocxx/client.hpp>
#include <string>

mongocxx::client createDBClient(std::string);

#endif // DB_H
