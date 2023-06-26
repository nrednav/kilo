#ifndef EDITOR_H
#define EDITOR_H

#include <vector>
#include <string>
#include <map>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <iostream>
#include <stdio.h>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <fstream>

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

struct Line {
  int size;
  char* contents;
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
  Editor(char* filename);
  ~Editor();
  void open(char* filename);

private:
  std::unique_ptr<Terminal> terminal{nullptr};
  std::unique_ptr<AppendBuffer> screen_buffer{nullptr};
  Window* window{nullptr};
  CursorPosition cursor_position;
  EscapeMap escape_map;
  std::vector<std::string> lines;
  int vertical_scroll_offset;

  void initialize();
  void refresh_screen();
  void draw();
  void process_input();
  void display_welcome_message();
  void move_cursor(int key);
  void scroll();

  int read_key();
  Window* create_window();
  CursorPosition get_cursor_position();
};

#endif // !EDITOR_H
