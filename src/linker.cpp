#include "../inc/linker.hpp"

Linker::Linker() {
  this->outfileStr = "";
  this->isHex = false;
}

void Linker::analizeInputFiles() {
  for (int i = 0; i < inputFiles.size(); i++) {
    //std::cout << "FILE : " << inputFiles.at(i) << "\n";
    std::string infileStr = inputFiles.at(i);
    std::ifstream file(infileStr, std::ios::in | std::ios::binary);

    // symbol table
    int numOfEntries = 0;
    file.read((char*)&numOfEntries, sizeof(int));
    for (int j = 0; j < numOfEntries; j++) {
      SymbolTableEntry entry;
      entry.fileId = i;

      int len;
      file.read((char*)&len, sizeof(int));
      entry.name.resize(len);
      file.read((char*)entry.name.c_str(), len);

      file.read((char*)&entry.isDefined, sizeof(entry.isDefined));
      file.read((char*)&entry.isGlobal, sizeof(entry.isGlobal));
      file.read((char*)&entry.isExtern, sizeof(entry.isExtern));
      file.read((char*)&entry.value, sizeof(entry.value));

      file.read((char*)&len, sizeof(int));
      entry.section.resize(len);
      
      file.read((char*)entry.section.c_str(), len);

        // if symbol is local, just add it to symbolTable for corresponding file
        // else if symbol is global, add it to corresponding symbolTable, but also add it to globalSymbolTable
      if (!entry.isGlobal && !entry.isExtern && entry.isDefined) {
        symbolTablesForEachFile[infileStr].push_back(entry);
      } else if (entry.isDefined) {
        globalSymbolTable.push_back(entry);
        auto it = symbolTimesDefined.find(entry.name);
        if (it != symbolTimesDefined.end()) {
          symbolTimesDefined[entry.name]++;
        } else {
          symbolTimesDefined[entry.name] = 1;
        }
      }
      // If symbol not defined in a file, we discard the symbol entry.
    }
    // section table
    file.read((char*)&numOfEntries, sizeof(int));
    for (int j = 0; j < numOfEntries; j++) {
      SectionTableEntry entry;

      entry.offset = 0;

      int len;
      file.read((char*)&len, sizeof(int));
      entry.name.resize(len);
      file.read((char*)entry.name.c_str(), len);
      
      file.read((char*)&entry.id, sizeof(entry.id));
      file.read((char*)&entry.length, sizeof(entry.length));

      

      sectionTableForEachFile[infileStr].push_back(entry);
    }
    // reloc table
    file.read((char*)&numOfEntries, sizeof(int));
    for (int j = 0; j < numOfEntries; j++) {
      RelocationTableEntry entry;
      int len;

      file.read((char*)&len, sizeof(int));
      entry.section.resize(len);
      file.read((char*)entry.section.c_str(), len);

      file.read((char*)&entry.offset, sizeof(entry.offset));
      file.read((char*)&entry.type, sizeof(entry.type));

      file.read((char*)&len, sizeof(int));
      entry.symbol.resize(len);
      file.read((char*)entry.symbol.c_str(), len);

      file.read((char*)&entry.addend, sizeof(entry.addend));


      relocTableForEachFile[infileStr].push_back(entry);
    }
    // sections data
    file.read((char*)&numOfEntries, sizeof(int));
    
    for (int j = 0; j < numOfEntries; j++) {
      int len;
      std::string section;
      std::string data;
      file.read((char*)&len, sizeof(int));
      section.resize(len);
      file.read((char*)section.c_str(), len);

      file.read((char*)&len, sizeof(int));
      data.resize(len);
      file.read((char*)data.c_str(), len);

      // append data to section
      stringstreamPerSectionPerFile[infileStr][section].write((char*)data.c_str(), len);
    }
    file.close();
  }
}

// If two files have a section with same name, the section of the second
// file will have an offset of sectionFromFirstFile.length
void Linker::determineSectionOffsetsFromFirstInstanceOfSection() {
  // No need to change offsets from first file, because they will always be zero.
  for (int i = 0; i < inputFiles.size(); i++) {
    
    std::string currFile = inputFiles.at(i);

    for (int j = 0; j < sectionTableForEachFile[currFile].size(); j++) {
      std::string currSectionName = sectionTableForEachFile[currFile].at(j).name;
      int currOffset = 0;
      bool found = false;
      for (int k = i - 1; k >= 0; k--) {
        std::string fileToCheck = inputFiles.at(k);

        for (int p = 0; p < sectionTableForEachFile[fileToCheck].size(); p++) {
          if (sectionTableForEachFile[fileToCheck].at(p).name == currSectionName) {
            sectionTableForEachFile[currFile].at(j).offset = sectionTableForEachFile[fileToCheck].at(p).length
                                                            + sectionTableForEachFile[fileToCheck].at(p).offset;
            found = true;
            sectionTableForEachFile[fileToCheck].at(p).hasFollowUp = true; // the internal structure that tells if
                                                          // the section has a follow up section from another file.
            break;
          } 
        }
        if (found == true) break;
      }
    }
  }
}

void Linker::updateOffsetsOfTables() {

  // no need to change tables from first file, because offsets of sections will always be zero.
  for (int i = 1; i < inputFiles.size(); i++) {
    std::string currFile = inputFiles.at(i);

    // updateSymbolTables(), because value (or offset from start of section) may now be obsolete and should be updated
    for (int j = 0; j < symbolTablesForEachFile[currFile].size(); j++) {
      //std::cout << "> " << symbolTablesForEachFile[currFile].at(j).name << " " << symbolTablesForEachFile[currFile].at(j).value << '\n';

      for (int k = 0; k < sectionTableForEachFile[currFile].size(); k++) {
        if (sectionTableForEachFile[currFile].at(k).name == symbolTablesForEachFile[currFile].at(j).section) {
          symbolTablesForEachFile[currFile].at(j).value = symbolTablesForEachFile[currFile].at(j).value + sectionTableForEachFile[currFile].at(k).offset;
          break;
        }
      }
      //std::cout << ">! " << symbolTablesForEachFile[currFile].at(j).name << " " << symbolTablesForEachFile[currFile].at(j).value << '\n';

    }
  }
    // updateGlobalSymbolTable()
  for (int j = 0; j < globalSymbolTable.size(); j++) {
    std::string currFile = inputFiles.at(globalSymbolTable.at(j).fileId);
    //std::cout << "< " << globalSymbolTable.at(j).name << " " << globalSymbolTable.at(j).value << '\n';

    for (int k = 0; k < sectionTableForEachFile[currFile].size(); k++) {
      if (sectionTableForEachFile[currFile].at(k).name == globalSymbolTable.at(j).section) {
        globalSymbolTable.at(j).value = globalSymbolTable.at(j).value + sectionTableForEachFile[currFile].at(k).offset;
      }
    }
    //std::cout << "<! " << globalSymbolTable.at(j).name << " " << globalSymbolTable.at(j).value << '\n';
  }
  // updateRelocTables(), because offset (from start of section) may now be obsolete and should be updated
  for (int i = 0; i < inputFiles.size(); i++) {
    std::string currFile = inputFiles.at(i);
    for (int j = 0; j < relocTableForEachFile[currFile].size(); j++) {
      
      for (int k = 0; k < sectionTableForEachFile[currFile].size(); k++) {
        if (sectionTableForEachFile[currFile].at(k).name == relocTableForEachFile[currFile].at(j).section) {
          relocTableForEachFile[currFile].at(j).offset = relocTableForEachFile[currFile].at(j).offset + sectionTableForEachFile[currFile].at(k).offset;
        }
      }
    }
  }
}

void Linker::mergeSectionStringstreams() {

  for (int i = 0; i < inputFiles.size(); i++) {
    std::string currFile = inputFiles.at(i);

    for (int j = 0; j < sectionTableForEachFile[currFile].size(); j++) {
      std::string currSection = sectionTableForEachFile[currFile].at(j).name;
      std::string currString = stringstreamPerSectionPerFile[currFile][currSection].str();
      int len = currString.length();
      stringstreamPerMergedSection[currSection].write((char*)currString.c_str(), len);
    }
  }
}

void Linker::determineSectionLengths() {

  for (int i = 0; i < inputFiles.size(); i++) {
    std::string currFile = inputFiles.at(i);

    for (int j = 0; j < sectionTableForEachFile[currFile].size(); j++) {
      //std::cout << " >>  " << sectionTableForEachFile[currFile].at(j).name << " " << sectionTableForEachFile[currFile].at(j).offset <<" " << sectionTableForEachFile[currFile].at(j).length << "\n";

      if (sectionTableForEachFile[currFile].at(j).hasFollowUp == true) {
        for (int k = i + 1; k < inputFiles.size(); k++) {
          std::string nextFile = inputFiles.at(k);
          bool hasAnotherFollowUp = false;

          int deletedSections = 0;
          for (int p = 0; p < sectionTableForEachFile[nextFile].size(); p++) {

            if (sectionTableForEachFile[nextFile].at(p).name == sectionTableForEachFile[currFile].at(j).name) {
              sectionTableForEachFile[currFile].at(j).length = sectionTableForEachFile[currFile].at(j).length + sectionTableForEachFile[nextFile].at(p).length;
              hasAnotherFollowUp = sectionTableForEachFile[nextFile].at(p).hasFollowUp;
              // i could also, when i update structures from currSection, delete nextSection because the sectionTableEntry for
              // the nextSection is no longer needed
              sectionTableForEachFile[nextFile].erase(sectionTableForEachFile[nextFile].begin() + p - deletedSections);
              deletedSections++; // because we shrinked sectionTable for 1 element (balances out the p++ in for loop)
              break;
            }
          }
          if (hasAnotherFollowUp == false) break;
        }
      }
    }
  }
}

void Linker::determineSectionOffsetsFromStartOfProgram() {
  int currMaxOffset = 0;
  //std::cout << '\n' << sectionsWithPlaceOption.size() << '\n';
  for (int i = 0; i < sectionsWithPlaceOption.size(); i++) {
    std::string placedSectionName = sectionsWithPlaceOption.at(i).first;
    int place = sectionsWithPlaceOption.at(i).second;
    //std::cout << placedSectionName << ":" << place << "\n";
    for (int j = 0; j < inputFiles.size(); j++) {
      std::string currFile = inputFiles.at(j);
      bool found = false;

      for (int k = 0; k < sectionTableForEachFile[currFile].size(); k++) {
        //std::cout << " >>  " << sectionTableForEachFile[currFile].at(k).name << "\n";
        if (sectionTableForEachFile[currFile].at(k).name == placedSectionName) {
          sectionTableForEachFile[currFile].at(k).offset = place;
          sectionTableForEachFile[currFile].at(k).hasExplicitPlace = true;
          int possibleMaxOffset = place + sectionTableForEachFile[currFile].at(k).length;
          if (possibleMaxOffset > currMaxOffset) currMaxOffset = possibleMaxOffset; 
          found = true;
          break;
        }
      }
      if (found == true) break;
    }
  }

  for (int i = 0; i < inputFiles.size(); i++) {
    std::string currFile = inputFiles.at(i);

    for (int j = 0; j < sectionTableForEachFile[currFile].size(); j++) {
      // if section is explicitly placed. skip this section;
      if (sectionTableForEachFile[currFile].at(j).hasExplicitPlace == true) continue;
      sectionTableForEachFile[currFile].at(j).offset = currMaxOffset; // IMPORTANT - NOW offsets determine offset from START OF PROGRAM!!!
      currMaxOffset += sectionTableForEachFile[currFile].at(j).length;
      //std::cout << " >>  " << sectionTableForEachFile[currFile].at(j).name << " " << sectionTableForEachFile[currFile].at(j).offset <<" " << sectionTableForEachFile[currFile].at(j).length << "\n";
    }
  }
}

void Linker::determineSymbolAndRelocOffsetsFromStartOfFile() {
  for (int i = 0; i < inputFiles.size(); i++) {
    std::string currFileName = inputFiles.at(i);

    for (int j = 0; j < sectionTableForEachFile[currFileName].size(); j++) {
      
      // symbol tables
      for (int k = 0; k < symbolTablesForEachFile[currFileName].size(); k++) {
        if (symbolTablesForEachFile[currFileName].at(k).section == sectionTableForEachFile[currFileName].at(j).name) {
          //std::cout << "> " << symbolTablesForEachFile[currFileName].at(k).name << " " << symbolTablesForEachFile[currFileName].at(k).value << '\n';
          symbolTablesForEachFile[currFileName].at(k).value += sectionTableForEachFile[currFileName].at(j).offset;
          //std::cout << "> " << symbolTablesForEachFile[currFileName].at(k).name << " " << symbolTablesForEachFile[currFileName].at(k).value << "\n\n";

        }
      }
      
      // global symbols table
      for (int k = 0; k < globalSymbolTable.size(); k++) {
        if (globalSymbolTable.at(k).section == sectionTableForEachFile[currFileName].at(j).name) {
          //std::cout << ">- " << globalSymbolTable.at(k).name << " " << globalSymbolTable.at(k).value << '\n';
          globalSymbolTable.at(k).value += sectionTableForEachFile[currFileName].at(j).offset;
        }
      }
    }
  }
}

void Linker::relocateSymbolInstances() {
  for (int i = 0; i < inputFiles.size(); i++) {
    std::string currFileName = inputFiles.at(i);

    for (int j = 0; j < relocTableForEachFile[currFileName].size(); j++) {


      bool isLocal = false;
      for (int k = 0; k < symbolTablesForEachFile[currFileName].size(); k++) {
        if (symbolTablesForEachFile[currFileName].at(k).name == relocTableForEachFile[currFileName].at(j).symbol) {
          //std::cout << "RELOCATING..." << symbolTablesForEachFile[currFileName].at(k).name << "|" << symbolTablesForEachFile[currFileName].at(k).value << "\n";
          stringstreamPerMergedSection[relocTableForEachFile[currFileName].at(j).section].seekp(relocTableForEachFile[currFileName].at(j).offset);
          stringstreamPerMergedSection[relocTableForEachFile[currFileName].at(j).section].write((char*)&symbolTablesForEachFile[currFileName].at(k).value, sizeof(int));
          stringstreamPerMergedSection[relocTableForEachFile[currFileName].at(j).section].seekp(0, std::ios::end);
          isLocal = true;
        }
      }
      if (!isLocal) {
        for (int k = 0; k < globalSymbolTable.size(); k++) {
          if (globalSymbolTable.at(k).name == relocTableForEachFile[currFileName].at(j).symbol) {
            //std::cout << "RELOCATING(global)..." << globalSymbolTable.at(k).name << "(" << globalSymbolTable.at(k).value << ") ->" << relocTableForEachFile[currFileName].at(j).section << "(" << relocTableForEachFile[currFileName].at(j).offset <<")\n";
            stringstreamPerMergedSection[relocTableForEachFile[currFileName].at(j).section].seekp(relocTableForEachFile[currFileName].at(j).offset);
            stringstreamPerMergedSection[relocTableForEachFile[currFileName].at(j).section].write((char*)&globalSymbolTable.at(k).value, sizeof(int));
            stringstreamPerMergedSection[relocTableForEachFile[currFileName].at(j).section].seekp(0, std::ios::end);
          }
        }
      }
    }
  }
}

bool Linker::checkForOverlappedPlaceSections() {
  bool overlapExists = false;
  for (int i = 0; i < sectionsWithPlaceOption.size(); i++) {
    auto pair1 = sectionsWithPlaceOption.at(i);
    int len1;
    for (int j  = 0; j < inputFiles.size(); j++) {
      bool found = false;
      std::string currFile = inputFiles.at(j);
      for (int k = 0; k < sectionTableForEachFile[currFile].size(); k++) {
        if (sectionTableForEachFile[currFile].at(k).name == pair1.first) {
          len1 = sectionTableForEachFile[currFile].at(k).length;
          found = true;
          break;
        }
      }
      if (found == true) break;
    }
    for (int j = 0; j < sectionsWithPlaceOption.size(); j++) {
      auto pair2 = sectionsWithPlaceOption.at(j);
      if (pair1.first == pair2.first) continue;
      int len2;
      for (int k = 0; k < inputFiles.size(); k++) {
        bool found = false;
        std::string currFile = inputFiles.at(k);
        for (int q = 0; q < sectionTableForEachFile[currFile].size(); q++) {
          if (sectionTableForEachFile[currFile].at(q).name == pair2.first) {
            len2 = sectionTableForEachFile[currFile].at(q).length;

            if ((pair1.second < pair2.second && pair1.second + len1 >= pair2.second)
                  || (pair2.second < pair1.second && pair2.second + len2 >= pair1.second)) {
              std::cout << "sections used in -place option are overlapping\n";
              overlapExists = true;
            }

            found = true;
            break;
          }
        }
      }
    }
  }
  return overlapExists;
}

void Linker::mergeSections() {
  determineSectionOffsetsFromFirstInstanceOfSection();
  updateOffsetsOfTables(); // to represent offset from start of section it is a part of
  mergeSectionStringstreams(); // std::maps of sections from each file will now be a part of one std::map of sections
  
  determineSectionLengths(); // updates section lengths to sections that existed in more than one file.
                             // also, if a section existed in more than one file, it deletes all but first instances of
                             // the section in section table entries

  bool overlapExists = checkForOverlappedPlaceSections();
  //std::cout << "\n\n#####";
  determineSectionOffsetsFromStartOfProgram();
  //std::cout << "#####\n\n";
  determineSymbolAndRelocOffsetsFromStartOfFile();
  
  relocateSymbolInstances();
}

void Linker::putAndSortSectionsIntoOneVector() {
  for (int i = 0; i < inputFiles.size(); i++) {
    for (int j = 0; j < sectionTableForEachFile[inputFiles.at(i)].size(); j++) {
      bool found = false;
      for (auto it = mergedSections.begin(); it != mergedSections.end(); it++) {
        if (it->offset > sectionTableForEachFile[inputFiles.at(i)].at(j).offset) {
          mergedSections.insert(it, sectionTableForEachFile[inputFiles.at(i)].at(j));
          found = true;
          break;
        }
      }
      if (!found) mergedSections.push_back(sectionTableForEachFile[inputFiles.at(i)].at(j));
    }
  }
}

void Linker::createBinaryFile() {
  std::string filename = outfileStr;
  filename.pop_back();
  filename.pop_back();
  filename.pop_back();
  filename.pop_back();

  filename += ".lnk";
  
  std::ofstream outputFile(filename, std::ios::out | std::ios::binary);

  int numOfEntries = 0;
  numOfEntries = mergedSections.size();
  outputFile.write((char*)&numOfEntries, sizeof(int));

  for (int i = 0; i < numOfEntries; i++) {
    SectionTableEntry currSection = mergedSections.at(i);
    std::string currString = stringstreamPerMergedSection[currSection.name].str();
    int address = currSection.offset;
    int len = currString.length();
    outputFile.write((char*)&address, sizeof(int));
    outputFile.write((char*)&len, sizeof(int));
    outputFile.write((char*)currString.c_str(), len);
  }
  outputFile.close();
}


void Linker::createTextFile() {
  std::ofstream outputFile(outfileStr, std::ios::out);
  bool firstLine = true;


  for (int i = 0; i < mergedSections.size(); i++) {
    std::string str = stringstreamPerMergedSection[mergedSections.at(i).name].str();
    int address = mergedSections.at(i).offset;
    int counter = 0;

    for (int k = 0; k < str.length(); k++) {
      if (counter % 8 == 0) {
        if (firstLine == false) outputFile << "\n";
        firstLine = false;
        counter = 0;
        outputFile << std::setfill('0') << std::setw(8) << std::hex << address;
        outputFile << ": ";
      }
      unsigned char c = str.c_str()[k];
      outputFile << std::setfill('0') << std::setw(2) << std::hex << (unsigned int)c;
      outputFile << " ";

      counter++;
      address++;
    }
  }
  outputFile.close();
}

void Linker::setOutput(std::string str) {
  outfileStr = str;
}

void Linker::addInputFile(std::string str) {
  inputFiles.push_back(str);
}

void Linker::addPlacedSection(std::string w, int loc) {
  printOutputFileName();
  //std::cout << "PLACE - " << w << loc << '\n';
  std::pair<std::string, int> placedSection(w, loc);
  sectionsWithPlaceOption.push_back(placedSection);
}

void Linker::setIsHex(bool boolean) {
  isHex = boolean;
}

void Linker::printOutputFileName() {
  //std::cout << "output (" << outfileStr << ")\n\n";
}

bool Linker::checkAndPrintMultipleDefinitions() {
  bool multipleDefinitionsExist = false;
  for (auto it = symbolTimesDefined.begin(); it != symbolTimesDefined.end(); it++) {
    if (it->second >= 2) {
      std::cout << "Multiple definitions of symbol '" << it->first << "' exist.\n";
      multipleDefinitionsExist = true;
    }
  }
  return multipleDefinitionsExist;
}

bool Linker::link() {
  std::cout << "linking... \n";
  analizeInputFiles();

  std::cout << "checking for multiple definitions of global symbols...\n";
  bool multipleDefinitionsExist = checkAndPrintMultipleDefinitions();

  if (multipleDefinitionsExist) return false;

  std::cout << "merging... \n";
  mergeSections();

  putAndSortSectionsIntoOneVector();

  if (isHex == true) {
    createTextFile();
    createBinaryFile();
  }

  return true;
}
