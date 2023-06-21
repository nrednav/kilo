#ifndef APPEND_BUFFER_H
#define APPEND_BUFFER_H

#include <string>
#include <stdlib.h>
#include <unistd.h>

class AppendBuffer {
public:
  AppendBuffer();
  ~AppendBuffer();

  void flush();
  void append(const std::string& text);
  void clear();

  const int& get_length() const;
  const char* get_contents() const;

private:
  char* contents{nullptr};
  int length{0};
};

#endif // !APPEND_BUFFER_H
