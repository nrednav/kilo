#ifndef EDITOR_H
#define EDITOR_H

#include <map>
#include <sys/ioctl.h>
#include <iostream>
#include <stdexcept>

#include "../Terminal/Terminal.h"
#include "../AppendBuffer/AppendBuffer.h"

#define CTRL_KEY(key) ((key)&0x1f);

struct Window {
  int width;
  int height;
};

struct CursorPosition {
  int x;
  int y;
};

class Editor {
public:
  Editor();
  ~Editor();

private:
  std::unique_ptr<Terminal> terminal;
  Window* window;
  std::unique_ptr<AppendBuffer> screen_buffer;
  std::map<std::string, std::string> escape_map;

  void initialize();
  void refresh_screen();
  void draw();
  void process_input();
  char read_key();
  Window* create_window();
  CursorPosition get_cursor_position();
};

#endif // !EDITOR_H
