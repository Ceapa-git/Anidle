#ifndef LOG_H
#define LOG_H

#include <string>
#include <memory>
#include <streambuf>
#include <fstream>

class ReplicateToFileStreamBuffer : public std::streambuf {
private:
  std::streambuf* originalBuffer;
  std::shared_ptr<std::ofstream> outputFile;
  std::string prefix;
  std::string streamName;
  bool atLineStart;

protected:
  int sync() override;
  int overflow(int) override;

public:
  ReplicateToFileStreamBuffer(std::shared_ptr<std::ofstream>, std::ostream&, const std::string&, const std::string&);
  ~ReplicateToFileStreamBuffer();
};

void initLogCout(std::shared_ptr<std::ofstream>);
void initLogCerr(std::shared_ptr<std::ofstream>);

#endif // LOG_H
