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

typedef std::map<std::string, std::string> EscapeMap;

struct Window {
  int width;
  int height;
};

struct CursorPosition {
  int x;
  int y;
};

enum EditorKey {
  Left = 1000,
  Right,
  Up,
  Down,
  Delete,
  PageUp,
  PageDown,
  Home,
  End,
};

class Editor {
public:
  Editor();
  ~Editor();

private:
  std::unique_ptr<Terminal> terminal;
  Window* window;
  std::unique_ptr<AppendBuffer> screen_buffer;
  CursorPosition cursor_position;
  EscapeMap escape_map;

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
