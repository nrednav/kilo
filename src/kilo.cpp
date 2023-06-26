#include "Editor/Editor.h"

int main(int argc, char* argv[]) {
  Editor editor{};

  if (argc >= 2) {
    editor.open(argv[1]);
  }

  return 0;
}
