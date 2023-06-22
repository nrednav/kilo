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
  this->escape_map["reset_cursor_pos"] = "\x1b[H";
  this->escape_map["clear_screen"] = "\x1b[2J";
}

Window* Editor::create_window() {
  static Window window{};

  struct winsize windowSize;
  bool gotWindowSize = ioctl(STDOUT_FILENO, TIOCGWINSZ, &windowSize) != -1;

  if (!gotWindowSize || windowSize.ws_col == 0) {
    if (write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12) != 12)
      throw std::runtime_error{
          "create_window: could not reposition cursor to bottom-right edge"};

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

  if (write(STDOUT_FILENO, "\x1b[6n", 4) != 4)
    throw std::runtime_error{
        "get_cursor_position: could not retrieve device status report"};

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

  char buffer[32];
  snprintf(buffer, sizeof(buffer), "\x1b[%d;%dH", this->cursor_position.y + 1,
           this->cursor_position.x + 1);
  this->screen_buffer->append(buffer);

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

  case ArrowKey::Left:
  case ArrowKey::Right:
  case ArrowKey::Up:
  case ArrowKey::Down:
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
      switch (sequence[1]) {
      case 'A':
        return ArrowKey::Up;
      case 'B':
        return ArrowKey::Down;
      case 'C':
        return ArrowKey::Right;
      case 'D':
        return ArrowKey::Left;
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
  case ArrowKey::Left:
    this->cursor_position.x--;
    break;
  case ArrowKey::Right:
    this->cursor_position.x++;
    break;
  case ArrowKey::Up:
    this->cursor_position.y--;
    break;
  case ArrowKey::Down:
    this->cursor_position.y++;
    break;
  }
}
