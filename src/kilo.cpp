#include "Editor/Editor.h"

int main(int argc, char* argv[]) {
  if (argc >= 2) {
    Editor editor{argv[1]};
  } else {
    Editor editor{};
  }

  return 0;
}
