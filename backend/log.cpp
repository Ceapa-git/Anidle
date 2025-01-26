#include "log.h"

#include <cstring>
#include <ctime>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>

ReplicateToFileStreamBuffer::ReplicateToFileStreamBuffer(
  std::shared_ptr<std::ofstream> fileStream,
  std::ostream& stream,
  const std::string& colorPrefix,
  const std::string& streamName
) : originalBuffer(stream.rdbuf()),
    atLineStart(true),
    prefix(colorPrefix),
    streamName(streamName),
    outputFile(fileStream) {
  stream.rdbuf(this);
}

ReplicateToFileStreamBuffer::~ReplicateToFileStreamBuffer() {
  if (originalBuffer) {
    std::ostream& stream = streamName == "cout" ? std::cout : std::cerr;
    stream.rdbuf(originalBuffer);
  }
}

int ReplicateToFileStreamBuffer::sync() {
  if (originalBuffer->pubsync() == -1 || !outputFile->flush()) {
    return -1;
  }
  return 0;
}

int ReplicateToFileStreamBuffer::overflow(int c) {
  if (c == EOF) {
    return EOF;
  }

  if (atLineStart && c != '\n') {
    std::time_t now = std::time(nullptr);
    std::string timeBuffer(30, '\0');
    std::strftime(timeBuffer.data(), timeBuffer.size(), "%d/%m/%Y : %H:%M:%S", std::localtime(&now));
    timeBuffer.resize(std::strlen(timeBuffer.data()));
    std::string timestamp = "[" + timeBuffer + "] ";

    originalBuffer->sputn(timestamp.c_str(), timestamp.size());
    *outputFile << timestamp;

    originalBuffer->sputn(prefix.c_str(), prefix.size());
    *outputFile << prefix;
  }

  if (originalBuffer->sputc(c) == EOF || !(*outputFile << static_cast<char>(c))) {
    return EOF;
  }

  if (c == '\n') {
    originalBuffer->sputn("\033[37m", 5);
    *outputFile << "\033[37m";

    outputFile->flush();
    originalBuffer->pubsync();
  }

  atLineStart = (c == '\n');
  return c;
}

void initLogCout(std::shared_ptr<std::ofstream> file) {
  static ReplicateToFileStreamBuffer logCoutBuffer(file, std::cout, "\033[32m", "cout");
}

void initLogCerr(std::shared_ptr<std::ofstream> file) {
  static ReplicateToFileStreamBuffer logCerrBuffer(file, std::cerr, "\033[31m", "cerr");
}
