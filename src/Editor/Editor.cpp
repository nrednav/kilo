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
}

void Editor::initialize() {
  static Terminal terminal{};
  this->terminal = &terminal;
  this->window = this->create_window();
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
  write(STDOUT_FILENO, "\x1b[2J", 4);
  write(STDOUT_FILENO, "\x1b[H", 3);

  this->draw();

  write(STDOUT_FILENO, "\x1b[H", 3);
}

void Editor::draw() {
  for (int y = 0; y < this->window->height; y++) {
    write(STDOUT_FILENO, "~", 3);

    if (y < this->window->height - 1) {
      write(STDOUT_FILENO, "\r\n", 2);
    }
  }
}

void Editor::process_input() {
  char key = this->read_key();

  switch (key) {
  case 0x1f & 'q': // Ctrl-q
    write(STDOUT_FILENO, "\x1b[2J", 4);
    write(STDOUT_FILENO, "\x1b[H", 3);
    exit(0);
    break;
  }
}

char Editor::read_key() {
  int numBytesRead = 0;
  char key;

  while ((read(STDIN_FILENO, &key, 1)) != 1) {
    if (numBytesRead == -1 && errno != EAGAIN)
      this->terminal->terminate("read_key");
  }

  return key;
}
