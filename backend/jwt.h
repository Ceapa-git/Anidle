#ifndef JWT_H
#define JWT_H

#include <cstddef>
#include <string>

void setJwtKey(std::string);
std::string createJwt(const std::string&, const std::string&, int);
bool isJwtValid(const std::string&);
std::string createJwtKey(std::size_t);

#endif // JWT_H
