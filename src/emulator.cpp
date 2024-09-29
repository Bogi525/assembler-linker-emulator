#include "../inc/emulator.hpp"

Emulator::Emulator() {
  inputFileStr = "";
  nextInstruction = {(OP_CODES)0,0,0,0,0};
  gp_regs = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0x40000000};
  cs_regs = {0,0,0};
  badInstruction = false;
  halted = false;
}

bool Emulator::readInputFile() {
  std::ifstream inputFile(inputFileStr, std::ios::in | std::ios::binary);
  if (!inputFile) return false;
  
  int numOfEntries;
  inputFile.read((char*)&numOfEntries, sizeof(int));

  for (int i = 0; i < numOfEntries; i++) {
    int address;
    int len;
    std::string str;

    inputFile.read((char*)&address, sizeof(int));
    inputFile.read((char*)&len, sizeof(int));
    str.resize(len);
    inputFile.read((char*)str.c_str(), len);
    
    for (int j = 0; j < str.size(); j++) {
      char c = str.at(j);
      memory[address++] = c;
    }
  }
  
  inputFile.close();
  return true;
}

void Emulator::printRegisters() {
  std::cout << "-----------------------------------------------------------------\n";
  std::cout << "Emulated processor state:\n";
  int counter = 0;

  for (int i = 0; i < gp_regs.size(); i++) {
    std::cout << "r" << std::dec << i << "=0x" << std::setw(8) << std::setfill('0') << std::hex << gp_regs.at(i);
    counter++;

    if (counter % 4 == 0) {
      counter = 0;
      std::cout << "\n";
    } else {
      std::cout << " ";
    }
  }
}

void Emulator::fetchInstruction() {
  unsigned char first, second, third, fourth;

  unsigned int pc = gp_regs[PC]++;
  auto it = memory.find(pc);
  if (it == memory.end()) fourth = 0x00;
  else fourth = it->second;

  pc = gp_regs[PC]++;
  it = memory.find(pc);
  if (it == memory.end()) third = 0x00;
  else third = it->second;

  pc = gp_regs[PC]++;
  it = memory.find(pc);
  if (it == memory.end()) second = 0x00;
  else second = it->second;

  pc = gp_regs[PC]++;
  it = memory.find(pc);
  if (it == memory.end()) first = 0x00;
  else first = it->second;


  nextInstruction.M = (OP_CODES)first;
	nextInstruction.A = (second >> 4 ) & 15;
	nextInstruction.B = second & 15;
	nextInstruction.C = (third >> 4) & 15;
	nextInstruction.D = ((third & 15) << 8) | fourth;

}

void Emulator::executeInstruction() {
  if (nextInstruction.M == OP_CODES::HALT) {
    halted = true;
  } else if (nextInstruction.M == OP_CODES::INT) {
    gp_regs[SP] -= 4;
    writeFourBytes(gp_regs[PC], gp_regs[SP]);
    gp_regs[SP] -= 4;
    writeFourBytes(cs_regs[STATUS], gp_regs[SP]);
    cs_regs[CAUSE] = 0x4;
    cs_regs[STATUS] = cs_regs[STATUS] & (~0x1);
    gp_regs[PC] = cs_regs[HANDLER];
  } else if (nextInstruction.M == OP_CODES::CALL_MEM_A_B_D) {
    gp_regs[SP] -= 4;
    writeFourBytes(gp_regs[PC], gp_regs[SP]);
    gp_regs[PC] = readFourBytes(gp_regs[nextInstruction.A] + gp_regs[nextInstruction.B] + nextInstruction.D);
  } else if (nextInstruction.M == OP_CODES::XCHG) {
    int temp = gp_regs[nextInstruction.B];
    gp_regs[nextInstruction.B] = gp_regs[nextInstruction.C];
    gp_regs[nextInstruction.C] = temp;
  } else if (nextInstruction.M == OP_CODES::PUSH) {
    if (nextInstruction.D > 255) {
      // drugi komplement i oduzimaj
      nextInstruction.D = nextInstruction.D | 0xfffff000;
      nextInstruction.D = ~nextInstruction.D + 1;
      gp_regs[nextInstruction.A] = gp_regs[nextInstruction.A] - nextInstruction.D;
    } else {
      gp_regs[nextInstruction.A] = gp_regs[nextInstruction.A] + nextInstruction.D;
    }
    writeFourBytes(gp_regs[nextInstruction.C], gp_regs[nextInstruction.A]);
  } else if (nextInstruction.M == OP_CODES::POP) {
    gp_regs[nextInstruction.A] = readFourBytes(gp_regs[nextInstruction.B]);
    gp_regs[nextInstruction.B] = gp_regs[nextInstruction.B] + nextInstruction.D;
  } else if (nextInstruction.M == OP_CODES::CSR_WR_MEM_UPDATE) {
    cs_regs[nextInstruction.A] = readFourBytes(gp_regs[nextInstruction.B]);
    gp_regs[nextInstruction.B] = gp_regs[nextInstruction.B] + nextInstruction.D;
  } else if (nextInstruction.M == OP_CODES::ADD) {
    if (nextInstruction.A != 0) gp_regs[nextInstruction.A] = gp_regs[nextInstruction.B] + gp_regs[nextInstruction.C];
  } else if (nextInstruction.M == OP_CODES::SUB) {
    if (nextInstruction.A != 0) gp_regs[nextInstruction.A] = gp_regs[nextInstruction.B] - gp_regs[nextInstruction.C];
  } else if (nextInstruction.M == OP_CODES::MUL) {
    if (nextInstruction.A != 0) gp_regs[nextInstruction.A] = gp_regs[nextInstruction.B] * gp_regs[nextInstruction.C];
  } else if (nextInstruction.M == OP_CODES::DIV) {
    if (nextInstruction.A != 0) gp_regs[nextInstruction.A] = gp_regs[nextInstruction.B] / gp_regs[nextInstruction.C];
  } else if (nextInstruction.M == OP_CODES::NOT) {
    if (nextInstruction.A != 0) gp_regs[nextInstruction.A] = ~gp_regs[nextInstruction.B];
  } else if (nextInstruction.M == OP_CODES::AND) {
    if (nextInstruction.A != 0) gp_regs[nextInstruction.A] = gp_regs[nextInstruction.B] & gp_regs[nextInstruction.C];
  } else if (nextInstruction.M == OP_CODES::OR) {
    if (nextInstruction.A != 0) gp_regs[nextInstruction.A] = gp_regs[nextInstruction.B] | gp_regs[nextInstruction.C];
  } else if (nextInstruction.M == OP_CODES::XOR) {
    if (nextInstruction.A != 0) gp_regs[nextInstruction.A] = gp_regs[nextInstruction.B] ^ gp_regs[nextInstruction.C];
  } else if (nextInstruction.M == OP_CODES::SHL) {
    if (nextInstruction.A != 0) gp_regs[nextInstruction.A] = gp_regs[nextInstruction.B] << gp_regs[nextInstruction.C];
  } else if (nextInstruction.M == OP_CODES::SHR) {
    if (nextInstruction.A != 0) gp_regs[nextInstruction.A] = gp_regs[nextInstruction.B] >> gp_regs[nextInstruction.C];
  } else if (nextInstruction.M == OP_CODES::CSRRD) {
    if (nextInstruction.A != 0) gp_regs[nextInstruction.A] = cs_regs[nextInstruction.B];
  } else if (nextInstruction.M == OP_CODES::CSRWR) {
    cs_regs[nextInstruction.A] = gp_regs[nextInstruction.B];
  } else if (nextInstruction.M == OP_CODES::LD_B_D) {
     if (nextInstruction.A != 0) gp_regs[nextInstruction.A] = gp_regs[nextInstruction.B] + nextInstruction.D;
  } else if (nextInstruction.M == OP_CODES::LD_MEM_B_C_D) {
    
    if (nextInstruction.A != 0) {
      if (memory.find(gp_regs[nextInstruction.B] + gp_regs[nextInstruction.C] + nextInstruction.D) != memory.end()) {
        int data = readFourBytes(gp_regs[nextInstruction.B] + gp_regs[nextInstruction.C] + nextInstruction.D);
        gp_regs[nextInstruction.A] = data;
        
      }
      else {
        gp_regs[nextInstruction.A] = 0;
      }
    }
  } else if (nextInstruction.M == OP_CODES::ST_MEM) {
    writeFourBytes(gp_regs[nextInstruction.C], gp_regs[nextInstruction.A] + gp_regs[nextInstruction.B] + nextInstruction.D);
  } else if (nextInstruction.M == OP_CODES::ST_MEM_MEM) {
    
    int address = readFourBytes(gp_regs[nextInstruction.A] + gp_regs[nextInstruction.B] + nextInstruction.D);
    writeFourBytes(gp_regs[nextInstruction.C], address);
  } else if (nextInstruction.M == OP_CODES::JMP_MEM_A_D) {
    int data = readFourBytes(gp_regs[nextInstruction.A] + nextInstruction.D);
    gp_regs[PC] = data;
  } else if (nextInstruction.M == OP_CODES::BEQ_MEM_A_D) {
    if (gp_regs[nextInstruction.B] == gp_regs[nextInstruction.C]) {
      int data = readFourBytes(gp_regs[nextInstruction.A] + nextInstruction.D);
      gp_regs[PC] = data;
    }
  } else if (nextInstruction.M == OP_CODES::BNE_MEM_A_D) {
    if (gp_regs[nextInstruction.B] != gp_regs[nextInstruction.C]) {
      int data = readFourBytes(gp_regs[nextInstruction.A] + nextInstruction.D);
      gp_regs[PC] = data;
    }
  } else if (nextInstruction.M == OP_CODES::BGT_MEM_A_D) {
    if ((signed int)gp_regs[nextInstruction.B] > (signed int)gp_regs[nextInstruction.C]) {
      int data = readFourBytes(gp_regs[nextInstruction.A] + nextInstruction.D);
      gp_regs[PC] = data;
    }
  } else {
    badInstruction = true;
  }
}

int Emulator::readFourBytes(unsigned int address) {
  int data = 0;

  if (memory.find(address) != memory.end()) {
    data = memory[address] & 0b11111111;
  }
  address++;

  if (memory.find(address) != memory.end()) {
    data = data | ((memory[address] & 0b11111111) << 8);
  }
  address++;

  if (memory.find(address) != memory.end()) {
    data = data | ((memory[address] & 0b11111111) << 16);
  }
  address++;

  if (memory.find(address) != memory.end()) {
    data = data | ((memory[address] & 0b11111111) << 24);
  }

  return data;
}

void Emulator::writeFourBytes(int data, unsigned int address) {
  char first, second, third, fourth;

  first = data & 0b11111111;
  second = (data >> 8) & 0b11111111;
  third = (data >> 16) & 0b11111111;
  fourth = (data >> 24) & 0b11111111;

  memory[address++] = first;
  memory[address++] = second;
  memory[address++] = third;
  memory[address] = fourth;
}

void Emulator::setInputFile(std::string str) {
  inputFileStr = str;
}

void Emulator::memoryDump() {
  std::ofstream outputFile("mem_content.hex", std::ios::out);
  bool firstLine = true;
  int prevAddr = 0;
  int counter = 0;

  for (auto it = memory.begin(); it != memory.end(); it++) {
    int currAddr = it->first;
    unsigned char value = it->second;

    if (counter % 8 == 0 || currAddr != prevAddr + 1) {
      if (!firstLine) outputFile << '\n';
      firstLine = false;
      outputFile << std::setfill('0') << std::setw(8) << std::hex << currAddr << ": ";
      firstLine = false;
      counter = 0;
    }

    outputFile << std::setfill('0') << std::setw(2) << std::hex << (unsigned int)value;
    outputFile << " ";

    counter++;
    prevAddr = currAddr;
  }
}

void Emulator::execute() {
  if (readInputFile() == false) {
    std::cout << "input file '" << inputFileStr << "' does not exist.\n";
    return;
  }

  int counter = 0;
  
  while (halted != true && counter != 20) {
    fetchInstruction();
    executeInstruction();
    if (badInstruction) {
      gp_regs[SP] -= 4;
      writeFourBytes(cs_regs[STATUS], gp_regs[SP]);
      gp_regs[SP] -= 4;
      writeFourBytes(gp_regs[PC], gp_regs[SP]);
      cs_regs[CAUSE] = 0x1;
      cs_regs[STATUS] = cs_regs[STATUS] & (~0x1);
      gp_regs[PC] = cs_regs[HANDLER];
    }
    if (halted) break;
  }

  printRegisters();
  memoryDump();
}