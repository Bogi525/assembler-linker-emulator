/*
  Helper file containing functions/classes/structures used in lexer.l and parser.y
*/
// lexerparser.hpp:
#include <vector>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>

enum ARG_TYPE {
  SYMBOL,
  NUMBER
};

enum ADDR_TYPE {
  IMMED,
  MEMDIR,
  REGDIR,
  REGIND,
  REGINDPOM
};

enum INSTR_NAME {
  HALT1,
  INT1,
  IRET1,
  CALL1,
  RET1,
  JMP1,
  BEQ1,
  BNE1,
  BGT1,
  PUSH1,
  POP1,
  XCHG1,
  ADD1,
  SUB1,
  MUL1,
  DIV1,
  NOT1,
  AND1,
  OR1,
  XOR1,
  SHL1,
  SHR1,
  LD1,
  ST1,
  CSRRD1,
  CSRWR1
};

struct ArgumentNode {
  ARG_TYPE type;
  std::string symbol;
  int number;
  ArgumentNode* next;
};

extern struct ArgumentNode* args;
extern FILE* inputFile;

int stringLiteralToInt(const char* str);
int stringHexToInt(const char* str);

void addSymbolToArgs(std::string* str);
void addLiteralToArgs(int num);
std::string dereferenceStringPointer(std::string* str);
std::string labelToString(std::string* str);
void deallocArgs();