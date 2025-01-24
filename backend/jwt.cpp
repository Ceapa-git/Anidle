#include "jwt.h"
#include "jwt-cpp/jwt.h"

#include <vector>
#include <stdexcept>
#include <chrono>
#include <cstddef>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <string>
#include <openssl/rand.h>
#include <openssl/bio.h>
#include <openssl/evp.h>

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
  if (jwtKey.empty()) {
    std::cerr << "[JWT] JWT Key is empty\n";
    return false;
  }
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

std::string createJwtKey(std::size_t numBytes) {
  std::vector<unsigned char> buffer(numBytes);
  if (RAND_bytes(buffer.data(), static_cast<int>(buffer.size())) != 1) {
    throw std::runtime_error("Failed to generate random bytes");
  }

  BIO* bio = BIO_new(BIO_s_mem());
  BIO* b64 = BIO_new(BIO_f_base64());

  BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
  b64 = BIO_push(b64, bio);

  BIO_write(b64, buffer.data(), buffer.size());
  BIO_flush(b64);

  BUF_MEM* memPtr;
  BIO_get_mem_ptr(b64, &memPtr);

  std::string encodedKey(memPtr->data, memPtr->length);
  BIO_free_all(b64);

  return encodedKey;
}

