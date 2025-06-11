# System Software Tools: Assembler, Linker, Emulator

A lightweight project inspired by the ELF format, built for a custom instruction set architecture.
Developed as part of a university course on system software, this project includes an **assembler**, **linker**, and **emulator**, written in C++ with **Flex** and **Bison** for parsing.

## Components:

### Assembler:
- Translates assembly source into object files
- Implements lexical and syntax analysis using **Flex** and **Bison**.
- Generates **symbol tables**, **section tables**, and **relocation entries**
- Supports labels, assembly directives, and instruction encoding
- Produces a object file format inspired by ELF

### Linker:
- Resolves external symbols and merges sections
- Applies relocations based on relocation tables
- Produces an executable binary ready for emulation
- Generates an optional human-readable text file for binary analysis.

### Emulator:
- Interprets and runs the final linked binary
- Simulates registers, memory, and instruction execution
- Provides optional memory dump

## Technologies
- C++
- Flex & Bison
- GNU Make
- Developed in Visual Studio Code on a Linux virtual machine

