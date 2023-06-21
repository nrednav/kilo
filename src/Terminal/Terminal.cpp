#include "Terminal.h"
#include <cstdlib>

Terminal::Terminal() {
  this->enable_raw_mode();
}

Terminal::~Terminal() {
  this->disable_raw_mode();
}

void Terminal::enable_raw_mode() {
  if (tcgetattr(STDIN_FILENO, &this->termios) == -1)
    this->terminate("tcgetattr");

  struct termios raw = this->termios;
  raw.c_iflag &= ~(ICRNL | IXON | BRKINT | INPCK | ISTRIP);
  raw.c_oflag &= ~(OPOST);
  raw.c_cflag |= ~(CS8);
  raw.c_lflag &= ~(ECHO | ICANON | ISIG | IEXTEN);
  raw.c_cc[VMIN] = 0;
  raw.c_cc[VTIME] = 1;

  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1)
    this->terminate("tcsetattr");
}

void Terminal::disable_raw_mode() {
  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &this->termios) == -1)
    this->terminate("tcsetattr");
}

void Terminal::terminate(const std::string& reason) {
  write(STDOUT_FILENO, "\x1b[2J", 4);
  write(STDOUT_FILENO, "\x1b[H", 3);

  perror(reason.c_str());
  exit(1);
}
