#include "../AppendBuffer/AppendBuffer.h"

AppendBuffer::AppendBuffer() : contents(nullptr), length{0} {}

AppendBuffer::~AppendBuffer() {
  free(contents);
}

void AppendBuffer::flush() {
  write(STDOUT_FILENO, contents, length);
}

void AppendBuffer::append(const std::string& text) {
  char* memory = (char*)realloc(contents, length + text.length());

  if (memory == nullptr) {
    throw std::runtime_error{
        "append: could not realloc memory for append buffer"};
  }

  std::memcpy(&memory[length], text.c_str(), text.length());

  contents = memory;
  length += text.length();
}

void AppendBuffer::clear() {
  free(contents);
  contents = nullptr;
  length = 0;
}

const int& AppendBuffer::get_length() const {
  return length;
}

const char* AppendBuffer::get_contents() const {
  return contents;
}
