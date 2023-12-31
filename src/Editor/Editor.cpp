#include "Editor.h"

Editor::Editor(const std::string& filename) {
  try {
    initialize();

    if (filename.length() > 0) {
      open(filename);
    }

    set_status_message("HELP: Ctrl-S = Save | Ctrl-Q = Quit | Ctrl-F = Find | "
                       "Ctrl-N/P = Next/Prev Word");

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
  vertical_scroll_offset = 0;
  horizontal_scroll_offset = 0;
  filename = "";
  status_message.contents[0] = '\0';
  status_message.timestamp = 0;
  edits_count = 0;
  awaiting_user_choice = false;
  current_occurence_index = 0;

  escape_map["show_cursor"] = "\x1b[?25l";
  escape_map["hide_cursor"] = "\x1b[?25h";
  escape_map["cursor_pos"] = "\x1b[%d;%dH";
  escape_map["reset_cursor_pos"] = "\x1b[H";
  escape_map["clear_screen"] = "\x1b[2J";
  escape_map["bottom_right_corner"] = "\x1b[999C\x1b[999B";
  escape_map["device_status"] = "\x1b[6n";
  escape_map["invert_colors"] = "\x1b[7m";
  escape_map["normal_colors"] = "\x1b[m";
  escape_map["clear_line"] = "\x1b[K";

  window->height -= 2;
}

void Editor::open(const std::string& filename) {
  this->filename = filename;

  std::ifstream file{filename};

  if (!file.is_open()) {
    terminal->terminate("open: could not open file");
  }

  for (std::string line; std::getline(file, line);) {
    lines.push_back(std::move(line));
  }

  edits_count = 0;
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
  scroll();

  screen_buffer->append(escape_map["show_cursor"]);
  screen_buffer->append(escape_map["clear_screen"]);
  screen_buffer->append(escape_map["reset_cursor_pos"]);

  draw();
  draw_status_bar();
  draw_message_bar();

  // Move cursor
  char cursor_buffer[32];
  snprintf(cursor_buffer, sizeof(cursor_buffer),
           escape_map["cursor_pos"].c_str(),
           (cursor_position.y - vertical_scroll_offset) + 1,
           (cursor_position.x - horizontal_scroll_offset) + 1);

  screen_buffer->append(cursor_buffer);

  screen_buffer->append(escape_map["hide_cursor"]);

  screen_buffer->flush();
  screen_buffer->clear();
}

void Editor::draw() {
  for (int row = 0; row < window->height; row++) {
    int line_number = row + vertical_scroll_offset;

    // If no lines have been read, display editor startup screen
    if (line_number >= (int)lines.size()) {
      if (lines.size() == 0 && row == window->height / 3) {
        display_welcome_message();
      } else {
        screen_buffer->append("~");
      }
    } else {
      draw_line(line_number);
    }

    screen_buffer->append("\r\n");
  }
}

void Editor::draw_line(int line_number) {
  int line_length = lines[line_number].length() - horizontal_scroll_offset;

  if (line_length < 0) {
    lines[line_number].resize(0);
  }

  if (line_length > window->width) {
    lines[line_number].resize(window->width);
  }

  replace_tabs_with_spaces(line_number);

  screen_buffer->append(lines[line_number].substr(horizontal_scroll_offset));
}

void Editor::process_input() {
  int key = read_key();

  // Handle [Y/N] type choices
  if (awaiting_user_choice) {
    if (key == 121) { // 121 = Y, 110 = N
      terminal->disable_raw_mode();
      terminal->terminate("force quit initiated");
    } else {
      awaiting_user_choice = false;
      set_status_message("");
      return;
    }
  }

  switch (key) {
  case '\r':
    insert_newline();
    break;
  case 0x1f & 'q': // Ctrl-q
    if (edits_count > 0) {
      set_status_message("Warning! There are unsaved changes. Are you sure you "
                         "wish to quit? [Y/N]");
      awaiting_user_choice = true;
    } else {
      terminal->disable_raw_mode();
      terminal->terminate("quit initiated");
    }
    break;
  case 0x1f & 's': // Ctrl-s
    save_file();
    break;
  case EditorKey::Home:
    cursor_position.x = 0;
    break;
  case EditorKey::End:
    if (cursor_position.x < (int)lines.size()) {
      cursor_position.x = lines[cursor_position.y].size();
    }
    break;
  case 0x1f & 'f': // Ctrl-f
    search();
    break;
  case EditorKey::Backspace:
  case EditorKey::Delete:
  case 0x1f & 'h': // Ctrl-h
    delete_character();
    break;
  case EditorKey::PageUp:
  case EditorKey::PageDown: {
    if (key == EditorKey::PageUp) {
      cursor_position.y = vertical_scroll_offset;
    } else {
      cursor_position.y = vertical_scroll_offset + window->height - 1;
      if (cursor_position.y > (int)lines.size()) {
        cursor_position.y = lines.size();
      }
    }

    int times = window->height;
    while (times--) {
      move_cursor(key == EditorKey::PageUp ? EditorKey::Up : EditorKey::Down);
    }
    break;
  }
  case EditorKey::Up:
  case EditorKey::Down:
  case EditorKey::Left:
  case EditorKey::Right:
    move_cursor(key);
    break;
  case 0x1f & 'l': // Ctrl-l
  case '\x1b':
    break;
  case 0x1f & 'n': { // Ctrl-n
    int next_occurence_index =
        (current_occurence_index + 1) % search_occurences.size();
    current_occurence_index = next_occurence_index;
    cursor_position.x = search_occurences[next_occurence_index][0];
    cursor_position.y = search_occurences[next_occurence_index][1];
    break;
  }
  case 0x1f & 'p': { // Ctrl-p
    int prev_occurence_index =
        (current_occurence_index - 1 + search_occurences.size()) %
        search_occurences.size();
    current_occurence_index = prev_occurence_index;
    cursor_position.x = search_occurences[prev_occurence_index][0];
    cursor_position.y = search_occurences[prev_occurence_index][1];
    break;
  }
  default:
    insert_character(key);
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
  std::string line =
      (cursor_position.y >= (int)lines.size()) ? "" : lines[cursor_position.y];

  switch (key) {
  case EditorKey::Left:
    if (cursor_position.x != 0) {
      cursor_position.x--;
    } else if (cursor_position.y > 0) {
      cursor_position.y--;
      cursor_position.x = lines[cursor_position.y].length();
    }
    break;
  case EditorKey::Right:
    if (!line.empty() && cursor_position.x < (int)line.length()) {
      cursor_position.x++;
    } else if (!line.empty() && cursor_position.x == (int)line.length()) {
      cursor_position.y++;
      cursor_position.x = 0;
    }
    break;
  case EditorKey::Up:
    if (cursor_position.y != 0) {
      cursor_position.y--;
    }
    break;
  case EditorKey::Down:
    if (cursor_position.y < (int)lines.size()) {
      cursor_position.y++;
    }
    break;
  }

  line = (cursor_position.y >= (int)lines.size()) ? ""
                                                  : lines.at(cursor_position.y);

  if (cursor_position.x > (int)line.length()) {
    cursor_position.x = line.length();
  }
}

void Editor::scroll() {
  if (cursor_position.y < vertical_scroll_offset) {
    vertical_scroll_offset = cursor_position.y;
  }

  if (cursor_position.y >= vertical_scroll_offset + window->height) {
    vertical_scroll_offset = cursor_position.y - window->height + 1;
  }

  if (cursor_position.x < horizontal_scroll_offset) {
    horizontal_scroll_offset = cursor_position.x;
  }

  if (cursor_position.x >= horizontal_scroll_offset + window->width) {
    horizontal_scroll_offset = cursor_position.x - window->width + 1;
  }
}

void Editor::replace_tabs_with_spaces(int line_number) {
  std::string tab_char{"\t"};
  std::string spaces(KILO_TAB_STOP, ' ');

  auto char_index = lines[line_number].find("\t");

  while (char_index != std::string::npos) {
    lines[line_number].replace(char_index, tab_char.size(), spaces);
    char_index = lines[line_number].find(tab_char);
  }
}

void Editor::draw_status_bar() {
  screen_buffer->append(escape_map["invert_colors"]);

  char left_status[100];
  char right_status[80];

  int status_length =
      snprintf(left_status, sizeof(left_status), "%.20s - %d lines | %s",
               filename.length() > 0 ? filename.c_str() : "[No Name]",
               (int)lines.size(), edits_count > 0 ? "(modified)" : "");

  int right_status_length =
      snprintf(right_status, sizeof(right_status), "%d/%d",
               cursor_position.y + 1, (int)lines.size());

  if (status_length > window->width) {
    status_length = window->width;
  }

  std::string left_status_line{left_status};
  left_status_line.resize(status_length);
  screen_buffer->append(left_status_line);

  while (status_length < window->width) {
    if (window->width - status_length == right_status_length) {
      std::string right_status_line{right_status};
      right_status_line.resize(right_status_length);
      screen_buffer->append(right_status_line);
      break;
    } else {
      screen_buffer->append(" ");
      status_length++;
    }
  }

  screen_buffer->append(escape_map["normal_colors"]);
  screen_buffer->append("\r\n");
}

void Editor::set_status_message(const char* formatted_string, ...) {
  va_list ap;
  va_start(ap, formatted_string);
  vsnprintf(status_message.contents, sizeof(status_message.contents),
            formatted_string, ap);
  va_end(ap);
  status_message.timestamp = time(NULL);
}

void Editor::draw_message_bar() {
  screen_buffer->append(escape_map["clear_line"]);

  int message_length = strlen(status_message.contents);

  if (message_length > window->width) {
    message_length = window->width;
  }

  if (message_length && time(NULL) - status_message.timestamp < 5) {
    screen_buffer->append(status_message.contents);
  }
}

void Editor::insert_character(int character) {
  int column_number = cursor_position.x;
  int line_number = cursor_position.y;

  if (line_number >= (int)lines.size()) {
    lines.push_back("");
  }

  std::string& line = lines.at(line_number);

  if (column_number < 0 || column_number > (int)line.length()) {
    column_number = line.length();
  }

  line.insert(column_number, 1, character);

  draw_line(line_number);

  cursor_position.x++;
  edits_count++;
}

void Editor::save_file() {
  if (filename.length() == 0) {
    filename = prompt("Save file as: %s");

    if (filename.length() == 0) {
      set_status_message("Save operation cancelled");
      return;
    }
  }

  std::ofstream file;

  try {
    file.open(filename, std::ios_base::out);

    for (unsigned int i = 0; i < lines.size(); i++) {
      file << lines[i] << "\n";
    }

    file.close();

    set_status_message("%d bytes to written to disk",
                       lines.size() * sizeof(lines[0]));

    edits_count = 0;

  } catch (const std::exception& e) {
    set_status_message("Could not save file. I/O error: %s", e.what());
    file.close();
  }
}

void Editor::delete_character() {
  int line_number = cursor_position.y;
  int column_number = cursor_position.x;
  std::string& line = lines[line_number];

  if (line_number == (int)lines.size()) {
    return;
  }

  if (column_number > 0) {
    line.erase(column_number - 1, 1);
    cursor_position.x--;
  } else if (column_number == 0) { // First column

    // On first line
    if (line_number == 0) {
      return;
    }

    // On any other line
    if (line.empty()) {
      // If empty, remove line
      move_cursor(EditorKey::Left);
      lines.erase(lines.begin() + line_number);
    } else {
      // If not empty, append current line to previous line
      std::string& previous_line = lines[line_number - 1];

      previous_line.append(line);
      move_cursor(EditorKey::Left);
      cursor_position.x -= line.length();
      lines.erase(lines.begin() + line_number);
    }
  }

  edits_count++;
}

void Editor::insert_newline() {
  if (lines.empty()) {
    lines.push_back("");
    return;
  }

  std::string& current_line = lines[cursor_position.y];

  if (current_line.empty()) {
    lines.push_back("");
    cursor_position.x = 0;
    cursor_position.y++;
  } else {
    if (cursor_position.x == 0) {
      lines.insert(lines.begin() + cursor_position.y, 1, "");
      cursor_position.y++;
    } else {
      std::string text = current_line.substr(cursor_position.x);
      current_line.erase(cursor_position.x);
      lines.insert(lines.begin() + cursor_position.y + 1, text);
      cursor_position.x = 0;
      cursor_position.y++;
    }
  }
}

std::string Editor::prompt(const std::string& message) {
  std::string input{""};

  while (true) {
    set_status_message(message.c_str(), input.c_str());
    refresh_screen();

    int key = read_key();

    if (key == EditorKey::Delete || key == (0x1f & 'h') ||
        key == EditorKey::Backspace) {
      if (input.length() > 0) {
        input.pop_back();
      }
    } else if (key == '\x1b') {
      set_status_message("");
      return "";
    } else if (key == '\r') {
      if (input.length() != 0) {
        set_status_message("");
        return input;
      }
    } else if (!iscntrl(key) && key < 128) {
      input.insert(input.end(), 1, key);
    }
  }
}

void Editor::search() {
  search_occurences.clear();

  std::string query = prompt("Search: %s (Press ESC to cancel)");

  if (query.length() == 0) {
    return;
  }

  unsigned int line_number = 0;
  do {
    const std::string& line = lines[line_number];

    std::size_t result_index = line.find(query);
    if (result_index != std::string::npos) {
      std::vector<unsigned int> occurence{
          static_cast<unsigned int>(result_index), line_number};
      search_occurences.push_back(occurence);
    }

    line_number++;
  } while (line_number != lines.size());

  if (search_occurences.size() > 0) {
    cursor_position.x = search_occurences[0][0];
    cursor_position.y = search_occurences[0][1];
    current_occurence_index = 0;
  }
}
