#ifndef JSON_H
#define JSON_H

#include "types.h"

#include <istream>

Json buildJson(std::istream&);
std::string jsonToString(const Json&);

#endif // JSON_H

