#include "server.h"
#include "http.h"
#include "types.h"

#include <cstddef>
#include <iostream>
#include <string>
#include <cstring>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <csignal>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <fcntl.h>
#include <sstream>

std::atomic<bool> running(true);
std::mutex mtx;
std::condition_variable cv;
int serverFd = -1;

void signalHandler(int signal) {
  if (signal == SIGTERM || signal == SIGINT) {
    std::cout << "Received termination signal (" << signal << "). Shutting down gracefully...\n";
    running = false;
    close(serverFd);
    cv.notify_all();
  }
}

int validateOptions(const ServerOptions& options) {
  if (options.port < 1000 || options.port > 9999) {
    std::cerr << "Options not in range (1000,9999).\n";
    return 1;
  }

  return 0;
}

void printBody(const Json& body, int indent = 0) {
  std::string indentStr(indent, ' ');

  if (body.type == Json::Type::VALUE) {
    std::cout << body.value << "\n";
  } else if (body.type == Json::Type::OBJECT){
    std::cout << "\n";
    for (const auto& [key, value] : body.object) {
      std::cout << indentStr << key << ": ";
      printBody(value, indent + 2);
    }
  } else if (body.type == Json::Type::ARRAY) {
    std::cout << "\n" << indentStr << "[\n";
    for (const auto& value : body.array) {
      std::cout << indentStr << "  ";
      if (value.type == Json::Type::OBJECT) {
        std::cout << "{";
        printBody(value, indent + 4);
        std::cout << indentStr << "  }\n";
      } else {
        printBody(value, indent + 2);
      }
    }
    std::cout << indentStr << "]\n";
  }
}

Route* findRoute(Route* initial, std::string route) {
  Route* current = initial;

  while (current != nullptr && current->path != route && current->next != nullptr) {
    current = current->next;
  }

  return current;
}

std::string readFromSocket(int socket) {
  char buffer[1024];
  std::string data;
  int total = 0;

  while (true) {
    size_t bytes = recv(socket, buffer, sizeof(buffer) - 1, 0);
    if (bytes <= 0) break;
    total += bytes;
    if (total >= 5000) return data;
    buffer[bytes] = '\0';
    data += buffer;
    if (bytes < static_cast<ssize_t>(sizeof(buffer) - 1)) break;
  }

  return data;
}

void serverLoop(const ServerOptions& options) {
  while (running) {
    sockaddr_in clientAddr;
    socklen_t clientLen = sizeof(clientAddr);

    int clientFd = accept(serverFd, (struct sockaddr*)&clientAddr, &clientLen);
    if (clientFd < 0) {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        continue;
      }
      if (running) {
        std::cerr << "Failed to accept connection.\n";
      }
      continue;
    }

    char clientIp[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(clientAddr.sin_addr), clientIp, INET_ADDRSTRLEN);
    // TODO add ip bans (to be fetched from mongo)

    std::string data = readFromSocket(clientFd);

    if (data.length() < 0) {
      std::cerr << clientIp << ": Failed to receive data.\n";
      close(clientFd);
      continue;
    }

    HttpObject request = parseRequest(data);
    request.ip = clientIp;
    std::cout << request.ip << ": " << request.methodStr << " " << request.path << "\n";

    if (options.debug) {
      std::cout << "\"" << request.method
        << "\" \"" << request.path;
      for (const auto& [key, value] : request.queryParams) {
        std::cout << key << " = " << value << "\n";
      }
      std::cout << "\"\nHeaders:\n";
      for (const auto& header : request.headers) {
        std::cout << header.first << ": " << header.second << "\n";
      }
      std::cout << "Body:\n";
      printBody(request.body);
    }

    std::istringstream iss(request.path);
    std::string token;
    std::getline(iss, token, '/');
    Route* currentRoute = options.routes;
    while (currentRoute != nullptr && std::getline(iss, token, '/')) {
      if (options.debug) {
        std::cout << "Path fragment: \"" << token << "\"\n";
      }
      currentRoute = findRoute(currentRoute->nested, token);
    }

    std::string response;
    if (currentRoute != nullptr && currentRoute->method == request.method) {
        response = currentRoute->handler(request);
    } else {
      Json notFound;
      notFound.type = Json::Type::VALUE;
      notFound.value = "Not Found";
      response = createResponse(NOT_FOUND, notFound);
    }
    ssize_t bytes_sent = send(clientFd, response.c_str(), response.size(), 0);
    if (bytes_sent < 0) {
      std::cerr << "Failed to send response.\n";
    }

    close(clientFd);
  }
  
  close(serverFd);
}

int createServer(const ServerOptions& options) {
  if (validateOptions(options) != 0) {
    return 1;
  }

  serverFd = socket(AF_INET, SOCK_STREAM, 0);
  if (serverFd < 0) {
    std::cerr << "Failed to create socket.\n";
    return 1;
  }

  int flags = fcntl(serverFd, F_GETFL, 0);
  if (flags < 0) {
    std::cerr << "Failed to get socket flags.\n";
    close(serverFd);
    return 1;
  }
  if (fcntl(serverFd, F_SETFL, flags | O_NONBLOCK) < 0) {
    std::cerr << "Failed to set socket to non-blocking mode.\n";
    close(serverFd);
    return 1;
  }

  int opt = 1;
  if (setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
    std::cerr << "Failed to set socket options.\n";
    close(serverFd);
    return 1;
  }

  sockaddr_in serverAddr;
  std::memset(&serverAddr, 0, sizeof(serverAddr));
  serverAddr.sin_family = AF_INET;
  serverAddr.sin_addr.s_addr = INADDR_ANY;
  serverAddr.sin_port = htons(options.port);

  if (bind(serverFd, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
    std::cerr << "Failed to bind socket.\n";
    close(serverFd);
    return 1;
  }

  if (listen(serverFd, 10) < 0) {
    std::cerr << "Failed to liste.\n";
    close(serverFd);
    return 1;
  }

  std::signal(SIGINT, signalHandler);
  std::signal(SIGTERM, signalHandler);

  std::cout << "Server is listening ...\n";
  running = true;

  std::thread serverThread(serverLoop, options);

  {
    std::unique_lock<std::mutex> lock(mtx);
    cv.wait(lock, [] { return !running; });
  }

  if (serverThread.joinable()) {
    serverThread.join();
  }

  return 0;
}

