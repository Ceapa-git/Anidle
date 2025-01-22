#include "response.h"
#include "server.h"
#include "types.h"

#include <iostream>
#include <fstream>
#include <streambuf>
#include <string>
#include <cstdlib>


struct CoutBuf : public std::streambuf {
  std::ofstream& file;
  std::streambuf* original;
  CoutBuf(std::ofstream& f, std::streambuf* orig) : file(f), original(orig) {}
  int overflow(int c) override {
    if (c != EOF) {
      file << "\033[37m" << static_cast<char>(c) << "\033[0m"; // White
      file.flush();
      original->sputc(c);
    }
    return c;
  }
};

struct CerrBuf : public std::streambuf {
  std::ofstream& file;
  std::streambuf* original;
  CerrBuf(std::ofstream& f, std::streambuf* orig) : file(f), original(orig) {}
  int overflow(int c) override {
    if (c != EOF) {
      file << "\033[31m" << static_cast<char>(c) << "\033[0m"; // Red
      file.flush();
      original->sputc(c);
    }
    return c;
  }
};

ServerOptions options;
bool logToFile = false;
std::string apiToken;

void processCliArgs(int argc, char** argv) {
  for (int i = 1; i < argc; i++) {
    std::string arg(argv[i]);
    if (arg == "-d" || arg == "--debug") {
      options.debug = true;
    } else if (arg == "-l" || arg == "--log") {
      logToFile = true;
    }
  }
}

int main(int argc, char** argv) {
  options.port = 8080;
  processCliArgs(argc, argv);

  apiToken = std::getenv("TOKEN");
  if (options.debug) {
    std::cout << "Token: " << apiToken << "\n";
  }

  Route base;
  base.path = "";
  base.method = Method::GET;
  base.handler = [](const std::string& params, const Body& body) {
    Body b;
    b.type = Body::Type::VALUE;
    b.value = "running";
    std::string response = createResponse(OK, b);
    return response;
  };

  Route routeA;
  routeA.path = "a";
  routeA.method = Method::GET;
  routeA.handler = [](const std::string& params, const Body& body) {
    Body ra;
    ra.type = Body::Type::OBJECT;
    ra.object["a"].type = Body::Type::VALUE;
    ra.object["a"].value = "asdf";
    ra.object["b"].type = Body::Type::VALUE;
    ra.object["b"].value = "123.4";
    ra.object["c"].type = Body::Type::VALUE;
    ra.object["c"].value = "false";
    ra.object["d"].type = Body::Type::OBJECT;
    ra.object["d"].object["e"].type = Body::Type::VALUE;
    ra.object["d"].object["e"].value = "false";
    ra.object["d"].object["f"].type = Body::Type::ARRAY;
    Body rg;
    rg.type = Body::Type::VALUE;
    rg.value = "asdf";
    Body rh;
    rh.type = Body::Type::VALUE;
    rh.value = "1123a";
    Body rj;
    rj.type = Body::Type::OBJECT;
    rj.object["k"].type = Body::Type::VALUE;
    rj.object["k"].value = "true";
    ra.object["d"].object["f"].array.push_back(rg);
    ra.object["d"].object["f"].array.push_back(rh);
    ra.object["d"].object["f"].array.push_back(rj);
    std::string response = createResponse(OK, ra);
    return response;
  };
  base.nested = &routeA;

  options.routes = &base;

  if (!logToFile) {
    createServer(options);
    return 0;
  }

  std::streambuf* coutBuf = std::cout.rdbuf();
  std::streambuf* cerrBuf = std::cerr.rdbuf();

  std::ofstream logFile("log");
  if (!logFile.is_open()) {
    std::cerr << "Failed to open log file!\n";
    return 1;
  }
  CoutBuf coutLogBuf(logFile, coutBuf);
  CerrBuf cerrLogBuf(logFile, cerrBuf);

  std::cout.rdbuf(&coutLogBuf);
  std::cerr.rdbuf(&cerrLogBuf);

  createServer(options);

  std::cout.rdbuf(coutBuf);
  std::cerr.rdbuf(cerrBuf);
  logFile.close();
  return 0;
}
