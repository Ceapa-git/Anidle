#include "server.h"
#include "route.h"

#include <iostream>
#include <fstream>
#include <streambuf>
#include <string>


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
bool log = false;

void processCliArgs(int argc, char** argv) {
  for (int i = 1; i < argc; i++) {
    std::string arg(argv[i]);
    std::cout << arg << "\n";
    if (arg == "-d" || arg == "--debug") {
      options.debug = true;
    } else if (arg == "-l" || arg == "--log") {
      log = true;
    }
  }
}

int main(int argc, char** argv) {
  options.port = 8080;
  processCliArgs(argc, argv);

  Route base;
  base.path = "/";
  base.method = Method::GET;
  base.handler = [](const std::string& params, const std::string& body) {
    std::cout << "GET /";
  };

  if (!log) {
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
