#ifndef _assembler_hpp_
#define _assembler_hpp_

#include <iostream>
#include <sstream>
#include <map>
#include "helpers.hpp"
#include <fstream>
#include <iomanip>

#define PC 15
#define SP 14

extern FILE *yyin;

enum OP_CODES {
  HALT = 0b00000000,
  INT = 0b00010000,
  CALL_MEM_A_B_D = 0b00100001,
  XCHG = 0b01000000,
  PUSH = 0b10000001,
  POP = 0b10010011,
  CSR_WR_MEM_UPDATE = 0b10010111, // za "pop status" kod "iret" instrukcije
  ADD = 0b01010000,
  SUB = 0b01010001,
  MUL = 0b01010010,
  DIV = 0b01010011,
  NOT = 0b01100000,
  AND = 0b01100001,
  OR = 0b01100010,
  XOR = 0b01100011,
  SHL = 0b01110000,
  SHR = 0b01110001,

  CSRRD = 0b10010000,
  CSRWR = 0b10010100,

  LD_B_D = 0b10010001,
  LD_MEM_B_C_D = 0b10010010,

  ST_MEM = 0b10000000,
  ST_MEM_MEM = 0b10000010,

  JMP_MEM_A_D = 0b00111000,
  BEQ_MEM_A_D = 0b00111001,
  BNE_MEM_A_D = 0b00111010,
  BGT_MEM_A_D = 0b00111011,
};

struct LiteralTableEntry {
  int number = 0;
  std::string symbol = "";
  bool isSymbol = false;
};

struct Backpatching {
  int instructionLocation;
  int currentPlaceholder;
  int literal = 0;
  std::string symbol = "";
  bool isSymbol = false;
};


struct SectionTableEntry {
  int id;
  std::string name;
  int offset;
  int length;
  std::vector<LiteralTableEntry> literalPool;
  std::vector<Backpatching> backpatchingArray;
};

struct SymbolTableEntry {
  int id;
  std::string name;
  int value;
  bool isGlobal;
  bool isExtern;
  bool isDefined;
  std::string section;
};

enum RELOC_TYPE {
  ABSOLUTE,
  RELATIVE
};

struct RelocationTableEntry {
  std::string section;
  int offset;
  RELOC_TYPE type;
  std::string symbol;
  int addend;
};

class Assembler {
public:
  static Assembler& getInstance() {
    static Assembler instance;
    return instance;
  }

  //konstruktor Asemblera:
  // label:
  bool handleLabel(std::string labelName, int location);

  // direktive:
  void handleGlobal(ArgumentNode* args, int currentLine);
  void handleExtern(ArgumentNode* args, int currentLine);
  void handleSection(std::string name, int currentLine);
  void handleWord(ArgumentNode* args, int currentLine);
  void handleSkip(int num, int currentLine);
  void handleEnd(int currentLine);

  // instrukcije:
  void handleArithmeticInstruction(INSTR_NAME instruction,int r1, int r2, int currentLine);
  void handleLogicInstruction(INSTR_NAME instruction, int r1, int r2, int currentLine);
  void handleShiftInstruction(INSTR_NAME instruction, int r1, int r2, int currentLine);
  void handleHaltInstruction(INSTR_NAME instruction, int currentLine);
  void handleStackInstruction(INSTR_NAME instruction, int r1, int currentLine);
  void handleLoadInstruction(INSTR_NAME instruction, ADDR_TYPE addressing, ARG_TYPE type, int gprS, int gprD, int literal, std::string symbol, int currentLine);
  void handleStoreInstruction(INSTR_NAME instruction, ADDR_TYPE addressing, ARG_TYPE type, int gprS, int gprD, int literal, std::string symbol, int currentLine);
  void handleCSRInstruction(INSTR_NAME instruction, int gpr, int csr, int currentLine);
  void handleXCHGInstruction(INSTR_NAME instruction, int r1, int r2, int currentLine);
  void handleJumpInstruction(INSTR_NAME instruction, ARG_TYPE type, int r1, int r2, int literal, std::string symbol, int currentLine);
  void handleCallInstruction(INSTR_NAME instruction, ARG_TYPE type, int literal, std::string symbol, int currentLine);
  void handleReturnInstruction(INSTR_NAME instruction, int currentLine);
  void handleInterruptInstruction(INSTR_NAME instruction, int currentLine);

  int insertInstruction(OP_CODES code, int a, int b, int c, int d);
  void placeLiteralPools();

  void setInput(const char* in);
  void setOutput(const char* out);

  void printErrors();
  bool errorsExist();

  void createBinaryFile();
  void createTextFile();

  std::map<int, std::string> printableErrors;
private:
  Assembler();

  Assembler(const Assembler&) = delete;
  Assembler& operator=(const Assembler&) = delete;

  
  
  int getNextSymbolId();
  int incrementSectionLocationCounterByFour();

  void addLiteralToBackpatchingArray(int instr, int literal);
  void addSymbolToBackpatchingArray(int instr, std::string symbol);

  bool checkLiteralIfExistsInLiteralPool(int literal);
  bool checkSymbolIfExistsInLiteralPool(std::string symbol);

  std::map<std::string, SectionTableEntry> sectionTable;
  int nextSectionId;

  std::map<std::string, SymbolTableEntry> symbolTable;
  int nextSymbolId;

  std::vector<RelocationTableEntry> relocVector;

  std::string currentSection;
  int sectionLocationCounter;

  
  std::map<std::string, std::stringstream> outputString;  // we work with std::stringstream instead of std::string, 
                                                          //because it works with integers easier than std::string
  bool passFinished;

  const char* infileStr;
  const char* outfileStr;
};


#endif