#include "../inc/assembler.hpp"

Assembler::Assembler() {
  // Initialization of internal structures and variables
  this->sectionLocationCounter = 0;
  this->currentSection = "";
  this->nextSectionId = 0;
  this->nextSymbolId = 0;
  this->passFinished = false;
}

bool Assembler::handleLabel(std::string labelName, int currentLine) {
  //std::cout << "handling label\n";
  // Check if the label is in a section, if not return error
  if (currentSection == "") {
    printableErrors[currentLine] = "a label must be in a section";
    return false;
  }

  // Find the corresponding pair<string, SymbolTableEntry> if exists in symbolTable
  std::map<std::string, SymbolTableEntry>::iterator it;
  it = symbolTable.find(labelName);

  // If it doesn't exist create a new entry
  if (it == symbolTable.end()) {
    SymbolTableEntry newEntry;
    newEntry.id = getNextSymbolId();
    newEntry.isDefined = true;
    newEntry.isExtern = false;
    newEntry.isGlobal = false;
    newEntry.name = labelName;
    newEntry.section = currentSection;
    newEntry.value = sectionLocationCounter; // offset from the start of the section

    symbolTable[labelName] = newEntry;

  // If it exists and is either defined or extern add errors to print
  } else if (it->second.isDefined) {
    printableErrors[currentLine] = "double definition of a symbol";
    return false;

  } else if (it->second.isExtern) {
    printableErrors[currentLine] = "defining a symbol that is extern";
    return false;

  // Else update the entry
  } else {
    it->second.isDefined = true;
    it->second.value = sectionLocationCounter;
    it->second.section = currentSection;
  }
  
  return true;
}

void Assembler::handleGlobal(ArgumentNode* args, int currentLine) {
  
  ArgumentNode* currArg = args;
  
  while (currArg != nullptr) {
    //std::cout << "GLOBAL >> " << currArg->symbol;
    std::string symbol = currArg->symbol;


    // Find the corresponding pair<string, SymbolTableEntry> if exists in symbolTable
    std::map<std::string, SymbolTableEntry>::iterator it;
    it = symbolTable.find(symbol);

    if (it == symbolTable.end()) {
      SymbolTableEntry newEntry;
      newEntry.id = getNextSymbolId();
      newEntry.isDefined = false;
      newEntry.isExtern = false;
      newEntry.isGlobal = true;
      newEntry.name = symbol;
      newEntry.section = ""; // not defined
      newEntry.value = 0; // not defined

      symbolTable[newEntry.name] = newEntry;
    } else {
      it->second.isGlobal = true;
    }

    // update currArg, and delete already processed argument
    currArg = currArg->next;
  }
}

void Assembler::handleExtern(ArgumentNode* args, int currentLine) {
  ArgumentNode* currArg = args;
  while (currArg != nullptr) {
    std::string symbol = currArg->symbol;

    // Find the corresponding pair<string, SymbolTableEntry> if exists in symbolTable
    std::map<std::string, SymbolTableEntry>::iterator it;
    it = symbolTable.find(symbol);

    if (it == symbolTable.end()) {
      SymbolTableEntry newEntry;
      newEntry.id = getNextSymbolId();
      newEntry.isDefined = false;
      newEntry.isExtern = true;
      newEntry.isGlobal = false;
      newEntry.name = symbol;
      newEntry.section = "UND"; // not defined
      newEntry.value = 0; // not defined

      symbolTable[symbol] = newEntry;
    } else {
      if (it->second.isDefined) {
        printableErrors[currentLine] = "Cannot declare a symbol extern, if it is already defined.";
      }
    }

    // update currArg, and delete already processed argument
    currArg = currArg->next;
  }
}

void Assembler::handleSection(std::string sectionName, int currentLine) {
  //std::cout << "handling section\n";
  std::map<std::string, SectionTableEntry>::iterator it;
  it = sectionTable.find(sectionName);

  if (it != sectionTable.end()) {
    printableErrors[currentLine] = "Section cannot be redeclared";
    return;
  }

  std::map<std::string, SymbolTableEntry>::iterator symbolTableIt;
  symbolTableIt = symbolTable.find(sectionName);

  if (symbolTableIt != symbolTable.end()) {
    if (symbolTableIt->second.isDefined == true) {
      printableErrors[currentLine] = "Symbol with the name of the section is already defined";
      return;
    }
  }

  // Updating data for previous section (if exists)
  if (currentSection != "") {
    //std::cout << "\n cnt = "<< sectionLocationCounter << "\n";
    sectionTable[currentSection].length = sectionLocationCounter;
  }
  // Adding the entry for the new section into the symbolTable
  SymbolTableEntry newSymbol;
  newSymbol.id = getNextSymbolId();
  newSymbol.isDefined = true;
  newSymbol.isExtern = false;
  newSymbol.isGlobal = false;
  newSymbol.name = sectionName;
  newSymbol.section = sectionName;
  newSymbol.value = 0; // offset of the start of the section (obviously 0, because the section starts at the definition)

  symbolTable[sectionName] = newSymbol;

  // Adding the entry for the new section into the sectionTable
  SectionTableEntry newSection;
  newSection.id = nextSectionId++;
  newSection.length = 0;
  newSection.name = sectionName;
  
  sectionLocationCounter = 0;
  sectionTable[sectionName] = newSection;

  currentSection = sectionName;
}

void Assembler::handleSkip(int num, int currentLine) {
  if (currentSection == "") {
    printableErrors[currentLine] = ".skip not in section";
    return;
  }
  
  char c = 0b00000000;
  
  for (int i = 0; i < num; i++) {
    outputString[currentSection].write((char*)&c, sizeof(char));  // .write() is preferred over operator<< because
                                                                  // stringstream << c would just insert '0' while
                                                                  // stringstream.write(char*)&c, sizeof(char)
                                                                  // will insert every bit of c for the length of it
                                                                  // so it will insert 0000 0000 (1 byte)
    sectionLocationCounter++;
  }
  //std::cout << "\n COUNTER = "<< sectionLocationCounter << "\n";
}

void Assembler::handleEnd(int currentLine) {
  if (currentSection == "") {
    printableErrors[currentLine] = "No section was ever created, cannot handle .end";
    return;
  }

  passFinished = true;
  //std::cout << "\n cnt1 = "<< sectionLocationCounter << "\n";
  sectionTable[currentSection].length = sectionLocationCounter;
  return;
}

void Assembler::handleWord(ArgumentNode* args, int currentLine) {
  if (currentSection == "") {
    printableErrors[currentLine] = ".word not in a section.";
    return;
  }

  ArgumentNode* currArg = args;
  //std::cout << "IN WORD: " << std::endl;
  while (currArg != nullptr) {
    if (currArg->type == ARG_TYPE::NUMBER) {
      outputString[currentSection].write((char*)&currArg->number, sizeof(int));
      incrementSectionLocationCounterByFour();
    } else if (currArg->type == ARG_TYPE::SYMBOL) {
      int zero = 0;
      outputString[currentSection].write((char*)&zero, sizeof(int));
      
      RelocationTableEntry rel;
      rel.section = currentSection;
      rel.offset = sectionLocationCounter;
      rel.addend = 0;
      rel.symbol = currArg->symbol;
      rel.type = RELOC_TYPE::ABSOLUTE;

      relocVector.push_back(rel);

      incrementSectionLocationCounterByFour();

    }

    ArgumentNode* prevArg = currArg;
    currArg = currArg->next;
    prevArg->next = nullptr;
    //delete prevArg;
    //std::cout << "IN WORD: " << currArg->symbol << std::endl;
  }


}

void Assembler::printErrors() {
  for (auto it = printableErrors.begin(); it != printableErrors.end(); it++) {
    //std::cout << it->second;
  }
}

void Assembler::handleArithmeticInstruction(INSTR_NAME instruction, int r1, int r2, int currentLine) {
  if (currentSection == "") {
    printableErrors[currentLine] = "Instruction is not a part of a section.";
    return;
  }

  if (instruction == INSTR_NAME::ADD1) {
    insertInstruction(OP_CODES::ADD, r2, r2, r1, 0);
    incrementSectionLocationCounterByFour();
  } else if (instruction == INSTR_NAME::SUB1) {
    insertInstruction(OP_CODES::SUB, r2, r2, r1, 0);
    incrementSectionLocationCounterByFour();
  } else if (instruction == INSTR_NAME::MUL1) {
    insertInstruction(OP_CODES::MUL, r2, r2, r1, 0);
    incrementSectionLocationCounterByFour();
  } else if (instruction == INSTR_NAME::DIV1) {
    insertInstruction(OP_CODES::DIV, r2, r2, r1, 0);
    incrementSectionLocationCounterByFour();
  }
}

void Assembler::handleLogicInstruction(INSTR_NAME instruction, int r1, int r2, int currentLine) {
  if (currentSection == "") {
    printableErrors[currentLine] = "Instruction is not a part of a section.";
    return;
  }

  if (instruction == INSTR_NAME::NOT1) {
    insertInstruction(OP_CODES::NOT, r1, r1, 0, 0);
    incrementSectionLocationCounterByFour();
  } else if (instruction == INSTR_NAME::AND1) {
    insertInstruction(OP_CODES::AND, r2, r2, r1, 0);
    incrementSectionLocationCounterByFour();
  } else if (instruction == INSTR_NAME::OR1) {
    insertInstruction(OP_CODES::OR, r2, r2, r1, 0);
    incrementSectionLocationCounterByFour();
  } else if (instruction == INSTR_NAME::XOR1) {
    insertInstruction(OP_CODES::XOR, r2, r2, r1, 0);
    incrementSectionLocationCounterByFour();
  }
}

void Assembler::handleShiftInstruction(INSTR_NAME instruction, int r1, int r2, int currentLine) {
  if (currentSection == "") {
    printableErrors[currentLine] = "Instruction is not a part of a section.";
    return;
  }

  if (instruction == INSTR_NAME::SHL1) {
    insertInstruction (OP_CODES::SHL, r2, r2, r1, 0);
    incrementSectionLocationCounterByFour();
  } else if (instruction == INSTR_NAME::SHR1) {
    insertInstruction (OP_CODES::SHR, r2, r2, r1, 0);
    incrementSectionLocationCounterByFour();
  }
}

void Assembler::handleHaltInstruction(INSTR_NAME instruction, int currentLine) {
  if (currentSection == "") {
    printableErrors[currentLine] = "Instruction is not a part of a section.";
    return;
  }

  if (instruction == INSTR_NAME::HALT1) {
    insertInstruction(OP_CODES::HALT, 0, 0, 0, 0);
    incrementSectionLocationCounterByFour();
  }
}

void Assembler::handleStackInstruction(INSTR_NAME instruction, int r1, int currentLine) {
  if (currentSection == "") {
    printableErrors[currentLine] = "Instruction is not a part of a section.";
    return;
  }

  if (instruction == INSTR_NAME::PUSH1) {
    insertInstruction(OP_CODES::PUSH, SP, 0, r1, -4);
    incrementSectionLocationCounterByFour();
  } else if (instruction == INSTR_NAME::POP1) {
    insertInstruction(OP_CODES::POP, r1, SP, 0, 4);
    incrementSectionLocationCounterByFour();
  }
}

void Assembler::handleXCHGInstruction(INSTR_NAME instruction, int r1, int r2, int currentLine) {
  if (currentSection == "") {
    printableErrors[currentLine] = "Instruction is not a part of a section.";
    return;
  }

  if (instruction == INSTR_NAME::XCHG1) {
    insertInstruction(OP_CODES::XCHG, 0, r1, r2, 0);
    incrementSectionLocationCounterByFour();
  }
}

void Assembler::handleReturnInstruction(INSTR_NAME instruction, int currentLine) {
  if (currentSection == "") {
    printableErrors[currentLine] = "Instruction is not a part of a section.";
    return;
  }

  if (instruction == INSTR_NAME::RET1) {
    insertInstruction(OP_CODES::POP, PC, SP, 0, 4);
    incrementSectionLocationCounterByFour();
  } else if (instruction == INSTR_NAME::IRET1) {
    insertInstruction(OP_CODES::CSR_WR_MEM_UPDATE, 0, SP, 0, 4);
    incrementSectionLocationCounterByFour();
    insertInstruction(OP_CODES::POP, PC, SP, 0, 4);
    incrementSectionLocationCounterByFour();
  }
}

int Assembler::insertInstruction(OP_CODES code, int a, int b, int c, int d) {
  int opcode = code << 24;
  int aa = (a & 15) << 20;
  int bb = (b & 15) << 16;
  int cc = (c & 15) << 12;
  int dd = (d & 0b111111111111);
  int instruction = opcode | aa | bb | cc | dd;
  //std::cout << std::hex << instruction << " ";
  outputString[currentSection].write((char*)&instruction, sizeof(int));
  return instruction;
}

void Assembler::handleLoadInstruction(INSTR_NAME instruction, ADDR_TYPE addressing, ARG_TYPE type, int gprS, int gprD, int literal, std::string symbol, int currentLine) {
  if (currentSection == "") {
    printableErrors[currentLine] = "Instruction is not a part of a section.";
    return;
  }

  //std::cout << '\n' << "LOAD INTO >>> " << gprD << "\n";
  
  if (instruction == INSTR_NAME::LD1) {
    if (addressing == ADDR_TYPE::IMMED) {
      if (type == ARG_TYPE::NUMBER) {
        if (literal == 0) {
          // then the instruction does not need a placeholder (the literal can be put in 12b)
          insertInstruction(OP_CODES::LD_B_D, gprD, 0, 0, literal);
          incrementSectionLocationCounterByFour();
        } else {
          // see if literal already exists in literal pool
          bool found = checkLiteralIfExistsInLiteralPool(literal);

          // if literal already exists don't add anything to literal pool
          // if it doesn't exist create new entry
          if (found == false) {
            LiteralTableEntry newLiteralEntry;
            newLiteralEntry.isSymbol = false;
            newLiteralEntry.number = literal;
            sectionTable[currentSection].literalPool.push_back(newLiteralEntry);
          }
          
          int instr = insertInstruction(OP_CODES::LD_MEM_B_C_D, gprD, PC, 0, 0);
          
          // either way create a new entry in backpatching array for later processing
          addLiteralToBackpatchingArray(instr, literal);
          incrementSectionLocationCounterByFour();
        }

      } else if (type == ARG_TYPE::SYMBOL) {

        std::map<std::string, SymbolTableEntry>::iterator it = symbolTable.find(symbol);
        // add symbol to symbol table if missing.
        if (it == symbolTable.end()) {
          SymbolTableEntry newEntry;
          newEntry.id = getNextSymbolId();
          newEntry.isDefined = false;
          newEntry.isExtern = false;
          newEntry.isGlobal = false;
          newEntry.name = symbol;
          newEntry.section = currentSection;
          newEntry.value = 0; // only after placing literal pool do we know the offset from the start of section
          symbolTable[symbol] = newEntry;
        }

        // new literal pool entry if needed
        bool found = checkSymbolIfExistsInLiteralPool(symbol);

        if (found == false) {
          LiteralTableEntry newLiteralEntry;
          newLiteralEntry.isSymbol = true;
          newLiteralEntry.symbol = symbol;
          sectionTable[currentSection].literalPool.push_back(newLiteralEntry);
        }

        int instr = insertInstruction(OP_CODES::LD_MEM_B_C_D, gprD, PC, 0, 0);

        // new backpatching entry
        addSymbolToBackpatchingArray(instr, symbol);
        incrementSectionLocationCounterByFour();
      }

    } else if (addressing == ADDR_TYPE::MEMDIR) {
      if (type == ARG_TYPE::NUMBER) {
        bool found = checkLiteralIfExistsInLiteralPool(literal);

        // if literal not in literal pool create new entry
        if (found == false) {
          LiteralTableEntry newLiteralEntry;
          newLiteralEntry.isSymbol = false;
          newLiteralEntry.number = literal;
          sectionTable[currentSection].literalPool.push_back(newLiteralEntry);
        }

        int instr = insertInstruction(OP_CODES::LD_MEM_B_C_D, gprD, PC, 0, 0); // gprD <= literal_from_literalPool (backpatching)

        // new backpatching entry
        addLiteralToBackpatchingArray(instr, literal);
        incrementSectionLocationCounterByFour();

        insertInstruction(OP_CODES::LD_MEM_B_C_D, gprD, gprD, 0, 0); // gprD <= mem[gprD + 0 + 0]
        incrementSectionLocationCounterByFour();

      } else if (type == ARG_TYPE::SYMBOL) {
        
        std::map<std::string, SymbolTableEntry>::iterator it = symbolTable.find(symbol);
        // add symbol to symbol table if missing.
        if (it == symbolTable.end()) {
          SymbolTableEntry newEntry;
          newEntry.id = getNextSymbolId();
          newEntry.isDefined = false;
          newEntry.isExtern = false;
          newEntry.isGlobal = false;
          newEntry.name = symbol;
          newEntry.section = currentSection;
          newEntry.value = 0; // only after placing literal pool do we know the offset from the start of section
          symbolTable[symbol] = newEntry;
        }

        // new literal pool entry if needed
        bool found = checkSymbolIfExistsInLiteralPool(symbol);

        if (found == false) {
          LiteralTableEntry newLiteralEntry;
          newLiteralEntry.isSymbol = true;
          newLiteralEntry.symbol = symbol;
          sectionTable[currentSection].literalPool.push_back(newLiteralEntry);
        }

        int instr = insertInstruction(OP_CODES::LD_MEM_B_C_D, gprD, PC, 0, 0); // gprD <= literal_from_literalPool (backpatching)

        // new backpatching entry
        addSymbolToBackpatchingArray(instr, symbol);
        incrementSectionLocationCounterByFour();

        insertInstruction(OP_CODES::LD_MEM_B_C_D, gprD, gprD, 0, 0); // gprD <= mem[gprD + 0 + 0]
        incrementSectionLocationCounterByFour();
      }

    } else if (addressing == ADDR_TYPE::REGDIR) {
      insertInstruction(OP_CODES::LD_B_D, gprD, gprS, 0, 0);
      incrementSectionLocationCounterByFour();

    } else if (addressing == ADDR_TYPE::REGIND) {
      insertInstruction(OP_CODES::LD_B_D, gprD, gprS, 0, 0);
      incrementSectionLocationCounterByFour();

      insertInstruction(OP_CODES::LD_MEM_B_C_D, gprD, gprD, 0, 0);
      incrementSectionLocationCounterByFour();

    } else if (addressing == ADDR_TYPE::REGINDPOM) {
      if (type == ARG_TYPE::NUMBER) {
        if (literal > 2047 || literal < -2048) {
          printableErrors[currentLine] = "Offset in instruction must fit in 12b.";
          return;
        }

        insertInstruction(OP_CODES::LD_B_D, gprD, gprS, 0, literal);
        incrementSectionLocationCounterByFour();

        insertInstruction(OP_CODES::LD_MEM_B_C_D, gprD, gprD, 0, 0);
        incrementSectionLocationCounterByFour();

      } else if (type == ARG_TYPE::SYMBOL) {
        std::map<std::string, SymbolTableEntry>::iterator it = symbolTable.find(symbol);
        if (it == symbolTable.end() || it->second.isDefined == false) {
          printableErrors[currentLine] = "Symbol is not defined.";
          return;
        }
        if (it->second.value > 2047 || it->second.value < -2048) {
          printableErrors[currentLine] = "Symbol cannot fit in 12b.";
          return;
        }

        insertInstruction(LD_B_D, gprD, gprS, 0, it->second.value);
        incrementSectionLocationCounterByFour();
      }
    }
  } 
}

void Assembler::handleStoreInstruction(INSTR_NAME instruction, ADDR_TYPE addressing, ARG_TYPE type, int gprS, int gprD, int literal, std::string symbol, int currentLine) {
  if (currentSection == "") {
    printableErrors[currentLine] = "Instruction is not a part of a section.";
    return;
  }
  
  if (instruction == INSTR_NAME::ST1) {
    if (addressing == ADDR_TYPE::IMMED) {
      printableErrors[currentLine] = "Instruction cannot have an immediate operand.";
      return;
      
    } else if (addressing == ADDR_TYPE::MEMDIR) {
      if (type == ARG_TYPE::NUMBER) {
        bool found = checkLiteralIfExistsInLiteralPool(literal);

        if (found == false) {
          LiteralTableEntry newLiteralEntry;
          newLiteralEntry.isSymbol = false;
          newLiteralEntry.number = literal;
          sectionTable[currentSection].literalPool.push_back(newLiteralEntry);
        }

        int instr = insertInstruction(OP_CODES::ST_MEM_MEM, PC, 0, gprS, 0);

        addLiteralToBackpatchingArray(instr, literal);
        incrementSectionLocationCounterByFour();

      } else if (type == ARG_TYPE::SYMBOL) {
        std::map<std::string, SymbolTableEntry>::iterator it = symbolTable.find(symbol);
        // add symbol to symbol table if missing.
        if (it == symbolTable.end()) {
          SymbolTableEntry newEntry;
          newEntry.id = getNextSymbolId();
          newEntry.isDefined = false;
          newEntry.isExtern = false;
          newEntry.isGlobal = false;
          newEntry.name = symbol;
          newEntry.section = currentSection;
          newEntry.value = 0; // only after placing literal pool do we know the offset from the start of section
          symbolTable[symbol] = newEntry;
        }

        bool found = checkSymbolIfExistsInLiteralPool(symbol);

        if (found == false) {
          LiteralTableEntry newLiteralEntry;
          newLiteralEntry.isSymbol = true;
          newLiteralEntry.symbol = symbol;
          sectionTable[currentSection].literalPool.push_back(newLiteralEntry);
        }

        int instr = insertInstruction(OP_CODES::ST_MEM_MEM, PC, 0, gprS, 0);

        addSymbolToBackpatchingArray(instr, symbol);
        incrementSectionLocationCounterByFour();
      }

    } else if (addressing == ADDR_TYPE::REGDIR) {
      // TODO, i can do the same as how i did it with LD r1, r2, just switch the registers
      insertInstruction(OP_CODES::LD_B_D, gprD, gprS, 0, 0);
      incrementSectionLocationCounterByFour();

    } else if (addressing == ADDR_TYPE::REGIND) {
      insertInstruction(OP_CODES::ST_MEM, gprD, 0, gprS, 0);
      incrementSectionLocationCounterByFour();

    } else if (addressing == ADDR_TYPE::REGINDPOM) {
      if (type == ARG_TYPE::NUMBER) {
        if (literal > 2047 || literal < -2048) {
          printableErrors[currentLine] = "Literal for offset in instruction must fit in 12b.";
          return;
        }

        insertInstruction(OP_CODES::ST_MEM, gprD, 0, gprS, literal);
        incrementSectionLocationCounterByFour();

      } else if (type == ARG_TYPE::SYMBOL) {
        std::map<std::string, SymbolTableEntry>::iterator it = symbolTable.find(symbol);
        if (it == symbolTable.end() || it->second.isDefined == false) {
          printableErrors[currentLine] = "Symbol is not defined.";
          return;
        }
        if (it->second.value > 2047 || it->second.value < -2048) {
          printableErrors[currentLine] = "Symbol cannot fit in 12b.";
          return;
        }

        insertInstruction(ST_MEM, gprD, gprS, 0, it->second.value);
        incrementSectionLocationCounterByFour();
      }
    }
  }
}

void Assembler::handleJumpInstruction(INSTR_NAME instruction, ARG_TYPE type, int r1, int r2, int literal, std::string symbol, int currentLine) {
  if (currentSection == "") {
    printableErrors[currentLine] = "Instruction is not a part of a section.";
    return;
  }
  
  if (type == ARG_TYPE::NUMBER) {
    //std::cout << "JUMP LITERAL\n";
    bool found = checkLiteralIfExistsInLiteralPool(literal);

    // if literal not in literal pool create new entry
    if (found == false) {
      LiteralTableEntry newLiteralEntry;
      newLiteralEntry.isSymbol = false;
      newLiteralEntry.number = literal;
      sectionTable[currentSection].literalPool.push_back(newLiteralEntry);
    }
    if (instruction == INSTR_NAME::JMP1) {

      int instr = insertInstruction(OP_CODES::JMP_MEM_A_D, PC, 0, 0, 0);

      addLiteralToBackpatchingArray(instr, literal);
      incrementSectionLocationCounterByFour();

    } else if (instruction == INSTR_NAME::BEQ1) {

      int instr = insertInstruction(OP_CODES::BEQ_MEM_A_D, PC, r1, r2, 0);

      addLiteralToBackpatchingArray(instr, literal);
      incrementSectionLocationCounterByFour();

    } else if (instruction == INSTR_NAME::BNE1) {

      int instr = insertInstruction(OP_CODES::BNE_MEM_A_D, PC, r1, r2, 0);

      addLiteralToBackpatchingArray(instr, literal);
      incrementSectionLocationCounterByFour();

    } else if (instruction == INSTR_NAME::BGT1) {

      int instr = insertInstruction(OP_CODES::BGT_MEM_A_D, PC, r1, r2, 0);

      addLiteralToBackpatchingArray(instr, literal);
      incrementSectionLocationCounterByFour();
    }
    
  } else if (type == ARG_TYPE::SYMBOL) {
    std::map<std::string, SymbolTableEntry>::iterator it = symbolTable.find(symbol);
    // add symbol to symbol table if missing.
    if (it == symbolTable.end()) {
      SymbolTableEntry newEntry;
      newEntry.id = getNextSymbolId();
      newEntry.isDefined = false;
      newEntry.isExtern = false;
      newEntry.isGlobal = false;
      newEntry.name = symbol;
      newEntry.section = currentSection;
      newEntry.value = 0; // only after placing literal pool do we know the offset from the start of section
      symbolTable[symbol] = newEntry;
    }

    bool found = checkSymbolIfExistsInLiteralPool(symbol);

    if (found == false) {
      LiteralTableEntry newLiteralEntry;
      newLiteralEntry.isSymbol = true;
      newLiteralEntry.symbol = symbol;
      sectionTable[currentSection].literalPool.push_back(newLiteralEntry);
    }

    if (instruction == INSTR_NAME::JMP1) {

      int instr = insertInstruction(OP_CODES::JMP_MEM_A_D, PC, 0, 0, 0);

      addSymbolToBackpatchingArray(instr, symbol);
      incrementSectionLocationCounterByFour();

    } else if (instruction == INSTR_NAME::BEQ1) {

      int instr = insertInstruction(OP_CODES::BEQ_MEM_A_D, PC, r1, r2, 0);

      addSymbolToBackpatchingArray(instr, symbol);
      incrementSectionLocationCounterByFour();

    } else if (instruction == INSTR_NAME::BNE1) {

      int instr = insertInstruction(OP_CODES::BNE_MEM_A_D, PC, r1, r2, 0);

      addSymbolToBackpatchingArray(instr, symbol);
      incrementSectionLocationCounterByFour();

    } else if (instruction == INSTR_NAME::BGT1) {

      int instr = insertInstruction(OP_CODES::BGT_MEM_A_D, PC, r1, r2, 0);

      addSymbolToBackpatchingArray(instr, symbol);
      incrementSectionLocationCounterByFour();
    }
  }
}

void Assembler::handleCallInstruction(INSTR_NAME instruction, ARG_TYPE type, int literal, std::string symbol, int currentLine) {
  if (currentSection == "") {
    printableErrors[currentLine] = "Instruction is not a part of a section.";
    return;
  }
  
  if (instruction == INSTR_NAME::CALL1) {
    if (type == ARG_TYPE::NUMBER) {

      bool found = checkLiteralIfExistsInLiteralPool(literal);

      // if literal not in literal pool create new entry
      if (found == false) {
        LiteralTableEntry newLiteralEntry;
        newLiteralEntry.isSymbol = false;
        newLiteralEntry.number = literal;
        sectionTable[currentSection].literalPool.push_back(newLiteralEntry);
      }

      int instr = insertInstruction(OP_CODES::CALL_MEM_A_B_D, PC, 0, 0, 0);

      addLiteralToBackpatchingArray(instr, literal);
      incrementSectionLocationCounterByFour();

    } else if (type == ARG_TYPE::SYMBOL) {
      std::map<std::string, SymbolTableEntry>::iterator it = symbolTable.find(symbol);
      // add symbol to symbol table if missing.
      if (it == symbolTable.end()) {
        SymbolTableEntry newEntry;
        newEntry.id = getNextSymbolId();
        newEntry.isDefined = false;
        newEntry.isExtern = false;
        newEntry.isGlobal = false;
        newEntry.name = symbol;
        newEntry.section = currentSection; // nepotrebno i netacno, .section treba da bude "UND", al me mrzi da menjam
        newEntry.value = 0; // only after placing literal pool do we know the offset from the start of section
        symbolTable[symbol] = newEntry;
      }

      bool found = checkSymbolIfExistsInLiteralPool(symbol);

      if (found == false) {
        LiteralTableEntry newLiteralEntry;
        newLiteralEntry.isSymbol = true;
        newLiteralEntry.symbol = symbol;
        sectionTable[currentSection].literalPool.push_back(newLiteralEntry);
      }

      int instr = insertInstruction(OP_CODES::CALL_MEM_A_B_D, PC, 0, 0, 0);

      addSymbolToBackpatchingArray(instr, symbol);
      incrementSectionLocationCounterByFour();
    }
  }
}

void Assembler::handleCSRInstruction(INSTR_NAME instruction, int gpr, int csr, int currentLine) {
  if (currentSection == "") {
    printableErrors[currentLine] = "Instruction is not a part of a section.";
    return;
  }

  if (instruction == INSTR_NAME::CSRRD1) {
    insertInstruction(OP_CODES::CSRRD, gpr, csr, 0, 0);
    incrementSectionLocationCounterByFour();

  } else if (instruction == INSTR_NAME::CSRWR1) {
    insertInstruction(OP_CODES::CSRWR, csr, gpr, 0, 0);
    incrementSectionLocationCounterByFour();
  }
}

void Assembler::handleInterruptInstruction(INSTR_NAME instruction, int currentLine) {
  if (currentSection == "") {
    printableErrors[currentLine] = "Instruction is not a part of a section.";
    return;
  }

  if (instruction == INSTR_NAME::INT1) {
    insertInstruction(OP_CODES::INT, 0, 0, 0, 0);
    incrementSectionLocationCounterByFour();
  }
}

int Assembler::getNextSymbolId() {
  return nextSymbolId++;
}

int Assembler::incrementSectionLocationCounterByFour() {
  //std ::cout << sectionLocationCounter + 4 << " - " << currentSection << '\n';
  return sectionLocationCounter += 4;
}

void Assembler::addLiteralToBackpatchingArray(int instr, int literal) {
  Backpatching newBackpatching;
  newBackpatching.instructionLocation = sectionLocationCounter;
  newBackpatching.currentPlaceholder = instr;
  newBackpatching.isSymbol = false;
  newBackpatching.literal = literal;
  sectionTable[currentSection].backpatchingArray.push_back(newBackpatching);
}
void Assembler::addSymbolToBackpatchingArray(int instr, std::string symbol) {
  Backpatching newBackpatching;
  newBackpatching.instructionLocation = sectionLocationCounter;
  newBackpatching.currentPlaceholder = instr;
  newBackpatching.isSymbol = true;
  newBackpatching.symbol = symbol;
  sectionTable[currentSection].backpatchingArray.push_back(newBackpatching);
}

bool Assembler::checkLiteralIfExistsInLiteralPool(int literal) {
  for (int i = 0; i < sectionTable[currentSection].literalPool.size(); i++) {
    if (sectionTable[currentSection].literalPool.at(i).isSymbol == false &&
        sectionTable[currentSection].literalPool.at(i).number == literal) {
      return true;
    }
  }
  return false;
}

bool Assembler::checkSymbolIfExistsInLiteralPool(std::string symbol) {
  for (int i = 0; i < sectionTable[currentSection].literalPool.size(); i++) {
    if (sectionTable[currentSection].literalPool.at(i).isSymbol == true &&
        sectionTable[currentSection].literalPool.at(i).symbol == symbol) {
      return true;
    }
  }

  return false;
}

void Assembler::setInput(const char* in) {
  infileStr = in;
}

void Assembler::setOutput(const char* out) {
  outfileStr = out;
}

bool Assembler::errorsExist() {
  if (printableErrors.empty()) {
    return false;
  }
  return true;
}

void Assembler::placeLiteralPools() {
  std::map<std::string, SectionTableEntry>::iterator it;
  for (it = sectionTable.begin(); it != sectionTable.end(); it++) {
    currentSection = it->second.name;
    sectionLocationCounter = it->second.length; // IMPORTANT, sets the sectionLocationCounter to point at the end of the section
    //std::cout << "POCETAK BAZENA LITERALA: " << sectionLocationCounter << "\n";
    //std::cout << "DUZINA: " << outputString[currentSection].str().length() << " - " << currentSection << ".\n";
    //std::cout << "literal pool for (" << it->second.name << ") section [" << it->second.literalPool.size() << "]\n";
    for (int i = 0; i < it->second.literalPool.size(); i++) {
      //std::cout << "LITERAL : " << it->second.literalPool.at(i).number << "\n";
        LiteralTableEntry literalEntry = it->second.literalPool.at(i);
      for (int j = 0; j < it->second.backpatchingArray.size(); j++) {
        Backpatching backpatchingEntry = it->second.backpatchingArray.at(j);
        if (literalEntry.isSymbol == backpatchingEntry.isSymbol && literalEntry.symbol == backpatchingEntry.symbol
            && literalEntry.number == backpatchingEntry.literal) {
          //std::cout << "BACKPATCH : " << it->second.backpatchingArray.at(j).instructionLocation << "---" << sectionLocationCounter << "\n";
          unsigned int data, offset;

          outputString[currentSection].seekp(backpatchingEntry.instructionLocation);
          data = backpatchingEntry.currentPlaceholder;
          offset = sectionLocationCounter - (backpatchingEntry.instructionLocation + 4);

          //std::cout << "Offset = " << offset << "\n";
          data = (~0b111111111111 & data) | (0b111111111111 & offset);
          outputString[currentSection].write((char*)&data, sizeof(int));

          outputString[currentSection].seekp(0, std::ios::end);
        }
      }
      if (literalEntry.isSymbol == false) {
          int entry = literalEntry.number;
          outputString[currentSection].write((char*)&entry, sizeof(int));

        } else if (literalEntry.isSymbol == true) {
          //std::cout << "   - symbol: " << literalEntry.symbol << "\n";
          int zero = 0;
          outputString[currentSection].write((char*)&zero, sizeof(int));
          // add relocation entry
          RelocationTableEntry rel;
          rel.section = currentSection;
          rel.offset = sectionLocationCounter;
          rel.addend = 0;
          rel.symbol = literalEntry.symbol;
          rel.type = RELOC_TYPE::ABSOLUTE;

          relocVector.push_back(rel);
        }
        //std::cout << "i am here\n";
        incrementSectionLocationCounterByFour();
    }
    //std::cout << "DUZINA: " << outputString[currentSection].str().length() << " - " << currentSection << ".\n";
    //std::cout << "new counter = " << sectionLocationCounter << "\n";
    it->second.length = sectionLocationCounter;
  }
}

void Assembler::createBinaryFile() {
  // open output file
  std::ofstream outputFile(outfileStr, std::ios::out | std::ios::binary);

  // symbol table
  int numOfEntries = symbolTable.size();
  //std::cout << "CREATING OUTPUT... \n";
  //std::cout << "\n" << numOfEntries << "\n";
  outputFile.write((char*)&numOfEntries, sizeof(int));

  for (std::map<std::string, SymbolTableEntry>::iterator it = symbolTable.begin(); it != symbolTable.end(); it++) {
    int len = it->second.name.length();
    outputFile.write((char*)&len, sizeof(int));
    outputFile.write((char*)it->second.name.c_str(), len);

    outputFile.write((char*)&it->second.isDefined, sizeof(it->second.isDefined));
    outputFile.write((char*)&it->second.isGlobal, sizeof(it->second.isGlobal));
    outputFile.write((char*)&it->second.isExtern, sizeof(it->second.isExtern));
    outputFile.write((char*)&it->second.value, sizeof(it->second.value));

    len = it->second.section.length();
    outputFile.write((char*)&len, sizeof(int));
    outputFile.write((char*)it->second.section.c_str(), len);
  }

  // section table
  numOfEntries = sectionTable.size();
  //std::cout << "\n" << numOfEntries << "\n";
  outputFile.write((char*)&numOfEntries, sizeof(int));

  for (std::map<std::string, SectionTableEntry>::iterator it = sectionTable.begin(); it != sectionTable.end(); it++) {

    // section name
    int len = it->second.name.length();
    outputFile.write((char*)&len, sizeof(int));
    outputFile.write((char*)it->second.name.c_str(), len);

    // id and length
    outputFile.write((char*)&it->second.id, sizeof(it->second.id));
    outputFile.write((char*)&it->second.length, sizeof(it->second.length));
  }

  // reloc table
  numOfEntries = relocVector.size();
  //std::cout << "\n" << numOfEntries << "\n";
  outputFile.write((char*)&numOfEntries, sizeof(int));

  for (int i = 0; i < relocVector.size(); i++) {
    
    // section of symbol to be relocated
    int len = relocVector.at(i).section.length();
    outputFile.write((char*)&len, sizeof(int));
    outputFile.write((char*)relocVector.at(i).section.c_str(), len);

    // offset and type of relocation
    outputFile.write((char*)&relocVector.at(i).offset, sizeof(relocVector.at(i).offset));
    outputFile.write((char*)&relocVector.at(i).type, sizeof(relocVector.at(i).type));

    // 
    len = relocVector.at(i).symbol.length();
    outputFile.write((char*)&len, sizeof(int));
    outputFile.write((char*)relocVector.at(i).symbol.c_str(), len);

    outputFile.write((char*)&relocVector.at(i).addend, sizeof(relocVector.at(i).addend));
  }

  // sections
  numOfEntries = outputString.size();
  outputFile.write((char*)&numOfEntries, sizeof(int));

  for (std::map<std::string, std::stringstream>::iterator it = outputString.begin(); it != outputString.end(); it++) {

    // section name
    int len;
    len = it->first.length();
    outputFile.write((char*)&len, sizeof(int));
    outputFile.write((char*)it->first.c_str(), len);

    // section data
    it->second.seekg(0);
    len = it->second.str().length();
    outputFile.write((char*)&len, sizeof(int));
    outputFile.write((char*)it->second.str().c_str(), len);
  }
  outputFile.close();
}

void Assembler::createTextFile() {
  std::string textoutfileStr = outfileStr;
  textoutfileStr.pop_back();
  textoutfileStr.pop_back();
  textoutfileStr += ".txt";

  std::ofstream outputFile(textoutfileStr, std::ios::out);

  // symbol table
  outputFile << "\n\n##### TABELA SIMBOLA #####\n";
  int i = 0;
  outputFile << std::setw(5) << "index";
  outputFile << std::setw(20) << "symbol_name";
  outputFile << std::setw(10) << "isDefined";
  outputFile << std::setw(10) << "isGlobal";
  outputFile << std::setw(10) << "isExtern";
  outputFile << std::setw(15) << "value(offset)";
  outputFile << std::setw(20) << "section";
  outputFile << "\n\n";
  for (auto it = symbolTable.begin(); it != symbolTable.end(); it++) {
    //outputFile << i++ << ".\t" << it->second.name << '\t' << it->second.isDefined << '\t' << it->second.isGlobal
    //            << '\t' << it->second.isExtern << '\t' << it->second.value << '\t' << it->second.section << std::endl;
    int counter = 20;
    counter = i % 10 - 1;
    outputFile << std::setw(5) << i++;
    outputFile << std::setw(20) << it->second.name;
    outputFile << std::setw(10) << it->second.isDefined;
    outputFile << std::setw(10) << it->second.isGlobal;
    outputFile << std::setw(10) << it->second.isExtern;
    outputFile << std::setw(15) << it->second.value;
    outputFile << std::setw(20) << it->second.section;
    outputFile << "\n";
  }

  // section table
  outputFile << "\n\n##### TABELA SEKCIJA #####\n";
  i  = 0;
  int currOffset = 0;
  outputFile << std::setw(5) << "index";
  outputFile << std::setw(20) << "section_name";
  outputFile << std::setw(15) << "offset";
  outputFile << std::setw(15) << "length";
  outputFile << "\n";
  for (std::map<std::string, SectionTableEntry>::iterator it = sectionTable.begin(); it != sectionTable.end(); it++) {
    outputFile << std::setw(5) << i++;
    outputFile << std::setw(20) << it->second.name;
    outputFile << std::setw(15) << currOffset;
    outputFile << std::setw(15) << it->second.length ;
    outputFile << "\n";
    currOffset += it->second.length;
  }

  // reloc table
  outputFile << "\n\n##### TABELA RELOKACIJA #####\n";
  outputFile << std::setw(5) << "index";
  outputFile << std::setw(20) << "section";
  outputFile << std::setw(10) << "offset";
  outputFile << std::setw(20) << "symbol";
  outputFile << std::setw(10) << "type";
  outputFile << "\n";
  for (int i = 0; i < relocVector.size(); i++) {
    RelocationTableEntry currReloc = relocVector.at(i);
    outputFile << std::setw(5) << i;
    outputFile << std::setw(20) << currReloc.section;
    outputFile << std::setw(10) << currReloc.offset;
    outputFile << std::setw(20) << currReloc.symbol;
    switch (currReloc.type) {
    case RELOC_TYPE::ABSOLUTE:
      outputFile << std::setw(10) << "/ABS/";
      break;
    case RELOC_TYPE::RELATIVE:
      outputFile << std::setw(10)  << "/REL/";
      break;
    }
    outputFile << std::endl;
  }
  
  // section data
  int locationCounter = 0;
  
  outputFile << "\n\n##### SECTION DATA #####\n";
  
  for (std::map<std::string, SectionTableEntry>::iterator it = sectionTable.begin(); it != sectionTable.end(); it++) {
    bool first = true;
    outputFile << "#." << it->second.name << "\n";
    //std::cout << "DUZINA: " << outputString[it->second.name].str().length() << " - " << it->second.name << ".\n";
    for (int i = 0; i < outputString[it->second.name].str().length(); i++) {
      if (locationCounter % 4 == 0) {
        if (!first) outputFile << '\n';
        first = false;
        outputFile << std::setfill('0') << std::setw(8) << std::hex << locationCounter;
        outputFile << ": ";
      }
      locationCounter++;
      unsigned char c = outputString[it->second.name].str().c_str()[i];
      outputFile << std::setfill('0') << std::setw(2) << std::hex << (unsigned int)c;
      outputFile << " ";
    }
    outputFile << "\n";
  }
}