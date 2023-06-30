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
#include <cstdio>
#include <stdexcept>
#include <fstream>
#include <algorithm>
#include <stdarg.h>
#include <exception>
#include <fstream>
#include <ios>

#include "../Terminal/Terminal.h"
#include "../AppendBuffer/AppendBuffer.h"

#define KILO_VERSION "0.0.1"
#define KILO_TAB_STOP 4

#define CTRL_KEY(key) (key) & 0x1f;

typedef std::map<std::string, std::string> EscapeMap;

struct Window {
  int width;
  int height;
};

struct CursorPosition {
  int x;
  int y;
};

struct StatusMessage {
  char contents[80];
  time_t timestamp{0};
};

enum EditorKey {
  Backspace = 127,
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
  Editor(const std::string& filename);
  ~Editor();
  void open(const std::string& filename);

private:
  std::unique_ptr<Terminal> terminal{nullptr};
  std::unique_ptr<AppendBuffer> screen_buffer{nullptr};
  Window* window{nullptr};
  CursorPosition cursor_position;
  EscapeMap escape_map;
  StatusMessage status_message;
  std::vector<std::string> lines;
  int vertical_scroll_offset;
  int horizontal_scroll_offset;
  std::string filename;
  int edits_count;
  bool awaiting_user_choice;

  void initialize();
  void refresh_screen();
  void draw();
  void draw_line(int line_number);
  void draw_status_bar();
  void draw_message_bar();
  void set_status_message(const char* formatted_string, ...);
  void process_input();
  void display_welcome_message();
  void move_cursor(int key);
  void scroll();
  void replace_tabs_with_spaces(int line_number);
  void insert_character(int character);
  void delete_character();
  void save_file();

  int read_key();
  Window* create_window();
  CursorPosition get_cursor_position();
};

#endif // !EDITOR_H
