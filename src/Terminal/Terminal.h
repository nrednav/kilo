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

private:
  struct termios termios;

  void enable_raw_mode();
  void disable_raw_mode();
};

#endif // !TERMINAL_H
