#include "../inc/helpers.hpp"

#define HEX_NUM 16

struct ArgumentNode* args;
FILE* inputFile;

int stringLiteralToInt(const char* str) {
  return atoi(str);
}

int stringHexToInt(const char* str) {
  return strtol(str, NULL, HEX_NUM);
}

void addSymbolToArgs(std::string* str) {
  std::string argument = *str;
  if (args == nullptr) {
    ArgumentNode* newArg = new ArgumentNode();
    newArg->type = ARG_TYPE::SYMBOL;
    newArg->symbol = argument;
    newArg->number = 0;
    newArg->next = nullptr;
    args = newArg;
  } else {
    ArgumentNode* newArg = new ArgumentNode();
    newArg->type = ARG_TYPE::SYMBOL;
    newArg->symbol = argument;
    newArg->number = 0;
    newArg->next = nullptr;
    ArgumentNode* lastArg = args;
    while (lastArg->next != nullptr) {
      lastArg = lastArg->next;
    }
    lastArg->next = newArg;
  }
}

void addLiteralToArgs(int num) {
  if (args == nullptr) {
    ArgumentNode* newArg = new ArgumentNode();
    newArg->type = ARG_TYPE::NUMBER;
    newArg->symbol = "";
    newArg->number = num;
    newArg->next = nullptr;
    args = newArg;
  } else {
    ArgumentNode* newArg = new ArgumentNode();
    newArg->type = ARG_TYPE::NUMBER;
    newArg->symbol = "";
    newArg->number = num;
    newArg->next = nullptr;
    ArgumentNode* lastArg = args;
    while (lastArg->next != nullptr) {
      lastArg = lastArg->next;
    }
    lastArg->next = newArg;
  }
}

std::string dereferenceStringPointer(std::string* str) {
  return *str;
}

std::string labelToString(std::string* str) {
  std::string res = *str;
  res.pop_back();
  return res;
}

void deallocArgs() {
  ArgumentNode* currArg = args;
  ArgumentNode* prevArg = args;
  while (currArg != nullptr) {
    prevArg = currArg;
    currArg = currArg->next;
    delete prevArg;
  }
  args = nullptr;
}