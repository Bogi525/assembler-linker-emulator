#include <iostream>
#include "../inc/emulator.hpp"
#include <vector>

int main(int argc, const char* argv[]) {
  if (argc != 2) {
    std::cout << "there must be 1 argument (input file).\n";
    return -1;
  }
  std::vector<std::string> arguments;
  arguments.push_back(std::string(argv[1]));
  std::string str = arguments.at(0);
  Emulator::getInstance().setInputFile(str);
  Emulator::getInstance().execute();

  return 0;
}