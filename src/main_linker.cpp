#include <iostream>
#include "../inc/linker.hpp"

using namespace std;


int main(int argc, const char* argv[]) {
  bool nextOneIsOutputFile = false;
  std::vector<std::string> arguments;
  for (int i = 0; i < argc; i++) {
    arguments.push_back(std::string(argv[i]));
  }
  
  for (int i = 1; i < arguments.size(); i++) {
    std::string str;
    str = arguments.at(i);
    if (str == "-o") {
      nextOneIsOutputFile = true;

    } else if (nextOneIsOutputFile) {
      nextOneIsOutputFile = false;
      Linker::getInstance().setOutput(str);
      

    } else if (str.size() >= 7 && str[0] == '-' && str[1] == 'p' && str[2] == 'l' && str[3] == 'a' && str[4] == 'c' && str[5] == 'e' && str[6] == '=') {
      Linker::getInstance().printOutputFileName();
      int sectionIndex = 6;
      int locationIndex = str.find('@');
      std::string s1;
      for (int j = sectionIndex + 1; j < locationIndex; j++) {
        s1 += str[j];
      }
      std::string s2;
      for (int j = locationIndex + 3; j < str.length(); j++) { // @0x_____
        s2 += str[j];
      }
      int location = stoul(s2, 0, 16);
      Linker::getInstance().addPlacedSection(s1, location);

    } else if (str == "-hex") {
      Linker::getInstance().setIsHex(true);
    } else {
      Linker::getInstance().addInputFile(str);
    }
  }

  bool status = Linker::getInstance().link();

  if (status == false) return -1;

  return 0;
}