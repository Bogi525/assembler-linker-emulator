#ifndef _linker_hpp_
#define _linker_hpp_

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>
#include <iomanip>

struct SectionTableEntry {
  int id;
  std::string name;
  int offset;
  int length;
  bool hasFollowUp = false;
  int fileId;
  bool hasExplicitPlace = false;
};

struct SymbolTableEntry {
  int id;
  std::string name;
  int value;
  bool isGlobal;
  bool isExtern;
  bool isDefined;
  std::string section;
  int fileId; // index of file which it belongs to
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

class Linker {  
public:
  static Linker& getInstance() {
    static Linker instance;
    return instance;
  }

  void analizeInputFiles();
  void mergeSections();
  void createBinaryFile();
  void createTextFile();
  bool link();
  
  void setOutput(std::string str);
  void addInputFile(std::string str);
  void addPlacedSection(std::string w, int loc);
  void setIsHex(bool boolean);
  void printOutputFileName();

private:
  Linker();

  Linker(const Linker&) = delete;
  Linker& operator=(const Linker&) = delete;

  void determineSectionOffsetsFromFirstInstanceOfSection();
  void updateOffsetsOfTables();
  void mergeSectionStringstreams();
  void determineSectionLengths();
  void determineSectionOffsetsFromStartOfProgram();
  void determineSymbolAndRelocOffsetsFromStartOfFile();
  void relocateSymbolInstances();
  bool checkForOverlappedPlaceSections();
  bool checkAndPrintMultipleDefinitions();
  void putAndSortSectionsIntoOneVector();


  std::vector<std::string> inputFiles;

  std::map<std::string, std::vector<SymbolTableEntry>> symbolTablesForEachFile; // key == INPUTFILE NAME
  std::vector<SymbolTableEntry> globalSymbolTable;

  std::map<std::string, std::vector<SectionTableEntry>> sectionTableForEachFile;

  std::map<std::string, std::vector<RelocationTableEntry>> relocTableForEachFile;

  std::map<std::string, std::map<std::string, std::stringstream>> stringstreamPerSectionPerFile;
  std::map<std::string, std::stringstream> stringstreamPerMergedSection;

  std::vector<std::pair<std::string, int>> sectionsWithPlaceOption;

  std::map<std::string, int> symbolTimesDefined;

  std::vector<SectionTableEntry> mergedSections;

  std::string outfileStr;
  bool isHex;
};

#endif