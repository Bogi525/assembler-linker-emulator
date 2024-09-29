#include <iostream>
#include "../inc/assembler.hpp"
#include "../inc/parser.hpp"

using namespace std;

int main(int argc, char* argv[]) {
  const char* option = argv[1];

  if ((string)(argv[1]) != "-o") {
    cout << "Output file does not exist" << endl;
    return -1;
  }

  const char* outfile = argv[2];
  const char* infile = argv[3];
  
  Assembler::getInstance().setOutput(outfile);
  Assembler::getInstance().setInput(infile);

  args = nullptr;
  inputFile = fopen(infile, "r"); // FILE* because it is used in parser.

  if (inputFile == nullptr) {
    std::cout << "File cannot be opened" << std::endl;
    return false;
  }

  yyin = inputFile;

  if (yyparse()) {
    return false;
  }
  fclose(inputFile);

  if (Assembler::getInstance().printableErrors.size() > 0) {
    for (std::map<int, std::string>::iterator it = Assembler::getInstance().printableErrors.begin(); it != Assembler::getInstance().printableErrors.end(); it++) {
      std::cout << it->second;
    }
  }

  Assembler::getInstance().placeLiteralPools();

  Assembler::getInstance().createBinaryFile();
  Assembler::getInstance().createTextFile();

  return true;

  if(Assembler::getInstance().errorsExist()) {
    Assembler::getInstance().printErrors();
    return -1;
  }

  return 0;
}