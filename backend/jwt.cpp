#include "jwt.h"
#include "jwt-cpp/jwt.h"

#include <chrono>
#include <cstddef>
#include <cstdlib>
#include <exception>
#include <random>
#include <string>

std::string jwtKey;

void setJwtKey(const std::string& key) {
  jwtKey = key;
}

std::string createJwt(const std::string& ip, const std::string& username, int duration) {
  auto now = std::chrono::system_clock::now();
  auto expiresAt = now + std::chrono::seconds(duration);

  std::string token = jwt::create()
    .set_issued_at(now)
    .set_not_before(now)
    .set_expires_at(expiresAt)
    .set_payload_claim("ip", jwt::claim(ip))
    .set_payload_claim("username", jwt::claim(username))
    .set_issuer(std::getenv("JWT_ISSUER"))
    .sign(jwt::algorithm::hs256{jwtKey});

  return token;
}

bool isJwtValid(const std::string& token) {
  try {
    auto decoded = jwt::decode(token);

    auto verifier = jwt::verify()
      .allow_algorithm(jwt::algorithm::hs256(jwtKey))
      .with_issuer(std::getenv("JWT_ISSUER"));
    verifier.verify(decoded);
    return true;
  } catch (const std::exception& e) {
    std::cerr << "[JWT] Verification error: " << e.what() << "\n";
    return false;
  }
}

std::string createJwtKey(std::size_t length) {
  static const char charset[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789-_";

  static std::random_device rd;
  static std::mt19937 generator(rd());

  std::uniform_int_distribution<std::size_t> dist(0, sizeof(charset) - 2);

  std::string key;
  key.reserve(length);

  for (std::size_t i = 0; i < length; ++i) {
    key.push_back(charset[dist(generator)]);
  }

  return key;
}

