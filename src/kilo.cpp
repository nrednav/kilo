#include "Editor/Editor.h"

int main(int argc, char* argv[]) {

  std::string filename{argc >= 2 ? argv[1] : ""};
  Editor editor{filename};

  return 0;
}
