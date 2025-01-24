#ifndef SECURITY_H
#define SECURITY_H

#include <cstddef>
#include <string>

void setJwtKey(const std::string&);
std::string createJwt(const std::string&, const std::string&, int);
bool isJwtValid(const std::string&);
std::string createJwtKey(std::size_t);
std::tuple<std::string, std::string> extractHostAndUsername(const std::string&);

std::string hashPassword(const std::string&);

#endif // SECURITY_H
