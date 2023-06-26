#include "Editor.h"

Editor::Editor() {
  try {
    initialize();

    while (true) {
      refresh_screen();
      process_input();
    }
  } catch (std::exception const& e) {
    std::cout << e.what() << std::endl;
  }
}

Editor::~Editor() {
  terminal = nullptr;
  window = nullptr;
  screen_buffer = nullptr;
}

void Editor::initialize() {
  terminal = std::make_unique<Terminal>();
  window = create_window();
  screen_buffer = std::make_unique<AppendBuffer>();
  cursor_position = CursorPosition{0, 0};
  line_count = 0;

  escape_map["show_cursor"] = "\x1b[?25l";
  escape_map["hide_cursor"] = "\x1b[?25h";
  escape_map["cursor_pos"] = "\x1b[%d;%dH";
  escape_map["reset_cursor_pos"] = "\x1b[H";
  escape_map["clear_screen"] = "\x1b[2J";
  escape_map["bottom_right_corner"] = "\x1b[999C\x1b[999B";
  escape_map["device_status"] = "\x1b[6n";
}

Window* Editor::create_window() {
  static Window window{};

  struct winsize window_size;
  bool got_window_size = ioctl(STDOUT_FILENO, TIOCGWINSZ, &window_size) != -1;

  if (!got_window_size || window_size.ws_col == 0) {
    if (write(STDOUT_FILENO, escape_map["bottom_right_corner"].c_str(), 12) !=
        12) {
      throw std::runtime_error{
          "create_window: could not reposition cursor to bottom-right edge"};
    }

    CursorPosition cursor_pos = get_cursor_position();
    window.width = cursor_pos.x;
    window.height = cursor_pos.y;
  }

  window.width = window_size.ws_col;
  window.height = window_size.ws_row;

  return &window;
}

CursorPosition Editor::get_cursor_position() {
  CursorPosition cursor_pos{};

  char cursor_pos_buffer[32];
  unsigned int i = 0;

  if (write(STDOUT_FILENO, escape_map["device_status"].c_str(), 4) != 4) {
    throw std::runtime_error{
        "get_cursor_position: could not retrieve device status report"};
  }

  while (i < sizeof(cursor_pos_buffer) - 1) {
    if (read(STDIN_FILENO, &cursor_pos_buffer[i], 1) != 1) {
      break;
    }

    if (cursor_pos_buffer[i] == 'R') {
      break;
    }

    i++;
  }

  cursor_pos_buffer[i] = '\0';

  if (cursor_pos_buffer[0] != '\x1b' || cursor_pos_buffer[1] != '[') {
    throw std::runtime_error{
        "get_cursor_position: could not detect escape sequence"};
  }

  int num_bytes_read =
      sscanf(&cursor_pos_buffer[2], "%d;%d", &cursor_pos.x, &cursor_pos.y);

  if (num_bytes_read != 2) {
    throw std::runtime_error{
        "get_cursor_position: could not extract x or y coordinates"};
  }

  return cursor_pos;
}

void Editor::refresh_screen() {
  screen_buffer->append(escape_map["show_cursor"]);
  screen_buffer->append(escape_map["clear_screen"]);
  screen_buffer->append(escape_map["reset_cursor_pos"]);

  draw();

  // Move cursor
  char cursor_buffer[32];
  snprintf(cursor_buffer, sizeof(cursor_buffer),
           escape_map["cursor_pos"].c_str(), cursor_position.y + 1,
           cursor_position.x + 1);
  screen_buffer->append(cursor_buffer);

  screen_buffer->append(escape_map["hide_cursor"]);

  screen_buffer->flush();
  screen_buffer->clear();
}

void Editor::draw() {
  for (int row = 0; row < window->height; row++) {
    if (row >= line_count) {
      if (line_count == 0 && row == window->height / 3) {
        display_welcome_message();
      } else {
        screen_buffer->append("~");
      }
    } else {
      if (line.size > window->width) {
        line.contents[window->width] = '\0';
      }
      screen_buffer->append(line.contents);
    }

    if (row < window->height - 1) {
      screen_buffer->append("\r\n");
    }
  }
}

void Editor::process_input() {
  int key = read_key();

  switch (key) {
  case 0x1f & 'q': // Ctrl-q
    terminal->disable_raw_mode();
    terminal->terminate("quit initiated");
    break;
  case EditorKey::Home:
    cursor_position.x = 0;
    break;
  case EditorKey::End:
    cursor_position.x = window->width - 1;
    break;
  case EditorKey::PageUp:
  case EditorKey::PageDown: {
    int times = window->height;
    while (times--) {
      move_cursor(key == EditorKey::PageUp ? EditorKey::Up : EditorKey::Down);
    }
    break;
  }
  case EditorKey::Left:
  case EditorKey::Right:
  case EditorKey::Up:
  case EditorKey::Down:
    move_cursor(key);
    break;
  }
}

int Editor::read_key() {
  int num_bytes_read = 0;
  char key;

  while ((num_bytes_read = read(STDIN_FILENO, &key, 1)) != 1) {
    if (num_bytes_read == -1 && errno != EAGAIN) {
      terminal->terminate("read_key");
    }
  }

  if (key == '\x1b') {
    char sequence[3];

    if (read(STDIN_FILENO, &sequence[0], 1) != 1) {
      return '\x1b';
    }

    if (read(STDIN_FILENO, &sequence[1], 1) != 1) {
      return '\x1b';
    }

    if (sequence[0] == '[') {
      if (sequence[1] >= '0' && sequence[1] <= '9') {
        if (read(STDIN_FILENO, &sequence[2], 1) != 1) {
          return '\x1b';
        }

        if (sequence[2] == '~') {
          switch (sequence[1]) {
          case '1':
            return EditorKey::Home;
          case '3':
            return EditorKey::Delete;
          case '4':
            return EditorKey::End;
          case '5':
            return EditorKey::PageUp;
          case '6':
            return EditorKey::PageDown;
          case '7':
            return EditorKey::Home;
          case '8':
            return EditorKey::End;
          }
        }
      } else {
        switch (sequence[1]) {
        case 'A':
          return EditorKey::Up;
        case 'B':
          return EditorKey::Down;
        case 'C':
          return EditorKey::Right;
        case 'D':
          return EditorKey::Left;
        case 'H':
          return EditorKey::Home;
        case 'F':
          return EditorKey::End;
        }
      }
    } else if (sequence[0] == 'O') {
      switch (sequence[1]) {
      case 'H':
        return EditorKey::Home;
      case 'F':
        return EditorKey::End;
      }
    }

    return '\x1b';
  }

  return key;
}

void Editor::display_welcome_message() {
  char message[80];
  int length = snprintf(message, sizeof(message), "Kilo Editor -- Version %s",
                        KILO_VERSION);

  if (length > window->width) {
    length = window->width;
  }

  int padding = (window->width - length) / 2;

  if (padding) {
    screen_buffer->append("~");
    padding--;
  }

  while (padding--) {
    screen_buffer->append(" ");
  }

  screen_buffer->append(message);
}

void Editor::move_cursor(int key) {
  switch (key) {
  case EditorKey::Left:
    if (cursor_position.x != 0) {
      cursor_position.x--;
    }
    break;
  case EditorKey::Right:
    if (cursor_position.x != window->width - 1) {
      cursor_position.x++;
    }
    break;
  case EditorKey::Up:
    if (cursor_position.y != 0) {
      cursor_position.y--;
    }
    break;
  case EditorKey::Down:
    if (cursor_position.y != window->height - 1) {
      cursor_position.y++;
    }
    break;
  }
}

void Editor::open(char* filename) {
  FILE* fp = fopen(filename, "r");
  if (!fp) {
    terminal->terminate("fopen");
  }

  char* line{nullptr};
  size_t line_cap{0};
  ssize_t line_length{0};

  line_length = getline(&line, &line_cap, fp);

  if (line_length != -1) {
    bool is_new_line = line[line_length - 1] == '\n';
    bool is_carriage_return = line[line_length - 1] == '\r';
    while (line_length > 0 && (is_new_line || is_carriage_return)) {
      line_length--;
    }

    this->line.size = line_length;
    this->line.contents = (char*)malloc(line_length + 1);
    std::memcpy(this->line.contents, line, line_length);

    this->line.contents[line_length] = '\0';
    this->line_count = 1;
  }

  free(line);
  fclose(fp);
}
