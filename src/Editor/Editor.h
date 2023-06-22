#ifndef EDITOR_H
#define EDITOR_H

#include <map>
#include <sys/ioctl.h>
#include <iostream>
#include <stdexcept>
#include <stdio.h>

#include "../Terminal/Terminal.h"
#include "../AppendBuffer/AppendBuffer.h"

#define KILO_VERSION "0.0.1"
#define CTRL_KEY(key) ((key)&0x1f);

struct Window {
  int width;
  int height;
};

struct CursorPosition {
  int x;
  int y;
};

enum ArrowKey { Left = 1000, Right, Up, Down };

class Editor {
public:
  Editor();
  ~Editor();

private:
  std::unique_ptr<Terminal> terminal;
  Window* window;
  std::unique_ptr<AppendBuffer> screen_buffer;
  CursorPosition cursor_position;
  std::map<std::string, std::string> escape_map;

  void initialize();
  void refresh_screen();
  void draw();
  void process_input();
  void display_welcome_message();
  int read_key();
  void move_cursor(int key);
  Window* create_window();
  CursorPosition get_cursor_position();
};

#endif // !EDITOR_H
