#include "../AppendBuffer/AppendBuffer.h"

AppendBuffer::AppendBuffer() : contents(nullptr), length{0} {}

AppendBuffer::~AppendBuffer() {
  free(this->contents);
}

void AppendBuffer::flush() {
  write(STDOUT_FILENO, this->contents, this->length);
}

void AppendBuffer::append(const std::string& text) {
  char* memory = (char*)realloc(this->contents, this->length + text.length());

  if (memory == nullptr)
    throw std::runtime_error{
        "append: could not realloc memory for append buffer"};

  std::memcpy(&memory[this->length], text.c_str(), text.length());
  this->contents = memory;
  this->length += text.length();
}

void AppendBuffer::clear() {
  free(this->contents);
  this->contents = nullptr;
  this->length = 0;
}

const int& AppendBuffer::get_length() const {
  return this->length;
}

const char* AppendBuffer::get_contents() const {
  return this->contents;
}
