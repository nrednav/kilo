#ifndef TERMINAL_H
#define TERMINAL_H

#include <termios.h>
#include <sys/termios.h>
#include <unistd.h>
#include <cstdlib>
#include <string>
#include <ctype.h>
#include <errno.h>

class Terminal {
public:
  Terminal();
  ~Terminal();
  void terminate(const std::string& reason);
  void disable_raw_mode();

private:
  struct termios original_termios;
  struct termios termios;

  void enable_raw_mode();
};

#endif // !TERMINAL_H
