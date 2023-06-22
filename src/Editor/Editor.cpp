#include "Editor.h"

Editor::Editor() {
  try {
    this->initialize();

    while (true) {
      this->refresh_screen();
      this->process_input();
    }
  } catch (std::exception const& e) {
    std::cout << e.what() << std::endl;
  }
}

Editor::~Editor() {
  this->terminal = nullptr;
  this->window = nullptr;
  this->screen_buffer = nullptr;
}

void Editor::initialize() {
  this->terminal = std::make_unique<Terminal>();
  this->window = this->create_window();
  this->screen_buffer = std::make_unique<AppendBuffer>();
  this->cursor_position = CursorPosition{0, 0};

  this->escape_map["show_cursor"] = "\x1b[?25l";
  this->escape_map["hide_cursor"] = "\x1b[?25h";
  this->escape_map["cursor_pos"] = "\x1b[%d;%dH";
  this->escape_map["reset_cursor_pos"] = "\x1b[H";
  this->escape_map["clear_screen"] = "\x1b[2J";
  this->escape_map["bottom_right_corner"] = "\x1b[999C\x1b[999B";
  this->escape_map["device_status"] = "\x1b[6n";
}

Window* Editor::create_window() {
  static Window window{};

  struct winsize windowSize;
  bool gotWindowSize = ioctl(STDOUT_FILENO, TIOCGWINSZ, &windowSize) != -1;

  if (!gotWindowSize || windowSize.ws_col == 0) {
    if (write(STDOUT_FILENO, this->escape_map["bottom_right_corner"].c_str(),
              12) != 12) {
      throw std::runtime_error{
          "create_window: could not reposition cursor to bottom-right edge"};
    }

    CursorPosition cursorPosition = this->get_cursor_position();
    window.width = cursorPosition.x;
    window.height = cursorPosition.y;
  }

  window.width = windowSize.ws_col;
  window.height = windowSize.ws_row;

  return &window;
}

CursorPosition Editor::get_cursor_position() {
  CursorPosition cursorPosition{};

  char buffer[32];
  unsigned int i = 0;

  if (write(STDOUT_FILENO, this->escape_map["device_status"].c_str(), 4) != 4) {
    throw std::runtime_error{
        "get_cursor_position: could not retrieve device status report"};
  }

  while (i < sizeof(buffer) - 1) {
    if (read(STDIN_FILENO, &buffer[i], 1) != 1)
      break;
    if (buffer[i] == 'R')
      break;
    i++;
  }

  buffer[i] = '\0';

  if (buffer[0] != '\x1b' || buffer[1] != '[')
    throw std::runtime_error{
        "get_cursor_position: could not detect escape sequence"};

  if (sscanf(&buffer[2], "%d;%d", &cursorPosition.x, &cursorPosition.y) != 2)
    throw std::runtime_error{
        "get_cursor_position: could not extract x or y coordinates"};

  return cursorPosition;
}

void Editor::refresh_screen() {
  this->screen_buffer->append(this->escape_map["show_cursor"]);
  this->screen_buffer->append(this->escape_map["clear_screen"]);
  this->screen_buffer->append(this->escape_map["reset_cursor_pos"]);

  this->draw();

  // Move cursor
  char cursor_buffer[32];
  snprintf(cursor_buffer, sizeof(cursor_buffer),
           this->escape_map["cursor_pos"].c_str(), this->cursor_position.y + 1,
           this->cursor_position.x + 1);
  this->screen_buffer->append(cursor_buffer);

  this->screen_buffer->append(this->escape_map["hide_cursor"]);

  this->screen_buffer->flush();
  this->screen_buffer->clear();
}

void Editor::draw() {
  for (int row = 0; row < this->window->height; row++) {
    if (row == this->window->height / 3) {
      this->display_welcome_message();
    } else {
      this->screen_buffer->append("~");
    }

    if (row < this->window->height - 1) {
      this->screen_buffer->append("\r\n");
    }
  }
}

void Editor::process_input() {
  int key = this->read_key();

  switch (key) {
  case 0x1f & 'q': // Ctrl-q
    this->terminal->disable_raw_mode();
    this->terminal->terminate("quit initiated");
    break;

  case EditorKey::Home:
    this->cursor_position.x = 0;
    break;
  case EditorKey::End:
    this->cursor_position.x = this->window->width - 1;
    break;

  case EditorKey::PageUp:
  case EditorKey::PageDown: {
    int times = this->window->height;
    while (times--) {
      this->move_cursor(key == EditorKey::PageUp ? EditorKey::Up
                                                 : EditorKey::Down);
    }
  }

  case EditorKey::Left:
  case EditorKey::Right:
  case EditorKey::Up:
  case EditorKey::Down:
    this->move_cursor(key);
  }
}

int Editor::read_key() {
  int numBytesRead = 0;
  char key;

  while ((read(STDIN_FILENO, &key, 1)) != 1) {
    if (numBytesRead == -1 && errno != EAGAIN)
      this->terminal->terminate("read_key");
  }

  if (key == '\x1b') {
    char sequence[3];

    if (read(STDIN_FILENO, &sequence[0], 1) != 1)
      return '\x1b';

    if (read(STDIN_FILENO, &sequence[1], 1) != 1)
      return '\x1b';

    if (sequence[0] == '[') {
      if (sequence[1] >= '0' && sequence[1] <= '9') {
        if (read(STDIN_FILENO, &sequence[2], 1) != 1)
          return '\x1b';
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
  char welcome[80];
  int welcome_length = snprintf(welcome, sizeof(welcome),
                                "Kilo Editor -- Version %s", KILO_VERSION);

  if (welcome_length > this->window->width)
    welcome_length = this->window->width;

  int padding = (this->window->width - welcome_length) / 2;
  if (padding) {
    this->screen_buffer->append("~");
    padding--;
  }
  while (padding--)
    this->screen_buffer->append(" ");

  this->screen_buffer->append(welcome);
}

void Editor::move_cursor(int key) {
  switch (key) {
  case EditorKey::Left:
    if (this->cursor_position.x != 0) {
      this->cursor_position.x--;
    }
    break;
  case EditorKey::Right:
    if (this->cursor_position.x != this->window->width - 1) {
      this->cursor_position.x++;
    }
    break;
  case EditorKey::Up:
    if (this->cursor_position.y != 0) {
      this->cursor_position.y--;
    }
    break;
  case EditorKey::Down:
    if (this->cursor_position.y != this->window->height - 1) {
      this->cursor_position.y++;
    }
    break;
  }
}
