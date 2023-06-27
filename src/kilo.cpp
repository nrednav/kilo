#include "Editor/Editor.h"

int main(int argc, char* argv[]) {

  if (argc >= 2) {
    std::string filename{argv[1]};
    Editor editor{filename};
  } else {
    Editor editor{};
  }

  return 0;
}
