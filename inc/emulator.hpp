#ifndef _emulator_hpp_
#define _emulator_hpp_

#include <iostream>
#include <vector>
#include <map>
#include <iomanip>
#include <fstream>

#define PC 15
#define SP 14

#define STATUS 0
#define HANDLER 1
#define CAUSE 2

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

struct Instruction {
  OP_CODES M;
  unsigned char A;
  unsigned char B;
  unsigned char C;
  unsigned int D;
};

class Emulator {
public:
  static Emulator& getInstance() {
    static Emulator instance;
    return instance;
  }

  void setInputFile(std::string str);
  void execute();

private:
  Emulator();

  Emulator(const Emulator&) = delete;
  Emulator& operator=(const Emulator&) = delete;

  bool readInputFile();
  void printRegisters();
  void memoryDump();
  void fetchInstruction();
  void executeInstruction();

  int readFourBytes(unsigned int address);
  void writeFourBytes(int data, unsigned int address);

  std::string inputFileStr;

  std::vector<int> gp_regs;
  std::vector<int> cs_regs;
  std::map<unsigned int, unsigned char> memory;

  Instruction nextInstruction;

  bool badInstruction;
  bool halted;
};

#endif