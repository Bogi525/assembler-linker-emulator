%{
    #include <iostream>
    #include <string>
    #include <vector>
    #include "../inc/assembler.hpp"
    int yylex(void);
    int yyerror(const char*);
    extern int currentLine;

    // u parser.hpp:
    // - #include <iostream>
    // - #include "../inc/helpers.hpp"
%}

%defines "inc/parser.hpp"
%union {
    int number;
    std::string* symbol;
    struct ArgumentNode* arg;
}

%token TOKEN_GLOBAL
%token TOKEN_EXTERN
%token TOKEN_SECTION
%token TOKEN_WORD
%token TOKEN_SKIP
%token TOKEN_ASCII
%token TOKEN_EQU
%token TOKEN_END

%token TOKEN_HALT
%token TOKEN_INT
%token TOKEN_CALL
%token TOKEN_RET
%token TOKEN_IRET
%token TOKEN_JMP
%token TOKEN_BEQ
%token TOKEN_BNE
%token TOKEN_BGT
%token TOKEN_PUSH
%token TOKEN_POP
%token TOKEN_XCHG
%token TOKEN_ADD
%token TOKEN_SUB
%token TOKEN_MUL
%token TOKEN_DIV
%token TOKEN_NOT
%token TOKEN_AND
%token TOKEN_OR
%token TOKEN_XOR
%token TOKEN_SHL
%token TOKEN_SHR
%token TOKEN_LD
%token TOKEN_ST
%token TOKEN_CSRRD
%token TOKEN_CSRWR

%token TOKEN_COMMENT
%token <number> TOKEN_LITERAL
%token <symbol> TOKEN_SYMBOL
%token <symbol> TOKEN_LABEL
%token TOKEN_IMM
%token <symbol> TOKEN_STRING
%token TOKEN_ENDL;
%token TOKEN_COMMA
%token <number> TOKEN_GP_REGISTER
%token <number> TOKEN_CS_REGISTER
%token TOKEN_LEFT_BRACKET
%token TOKEN_RIGHT_BRACKET
%token TOKEN_PLUS

%type <arg> list_of_symbols
%type <arg> list_of_literals_and_syms


%%

inpt:
    line endls | inpt line endls | inpt line
;
endls:
    TOKEN_ENDL {
        currentLine = currentLine + 1;
    }
    | endls TOKEN_ENDL {
        currentLine = currentLine + 1;
    }
;
line:
    label | label content | label content TOKEN_COMMENT
    | content | content TOKEN_COMMENT | TOKEN_COMMENT
;
label:
    TOKEN_LABEL {
        std::string labelName = labelToString($1);
        Assembler::getInstance().handleLabel(labelName, currentLine);
        delete $1;
    }
;
content:
    directive | instruction
;
directive:
    TOKEN_GLOBAL list_of_symbols {
        Assembler::getInstance().handleGlobal(args, currentLine);
        deallocArgs();
    }
    | TOKEN_EXTERN list_of_symbols {
        Assembler::getInstance().handleExtern(args, currentLine);
        deallocArgs();
    }
    | TOKEN_SECTION TOKEN_SYMBOL {
        std::string name = dereferenceStringPointer($2);
        Assembler::getInstance().handleSection(name, currentLine);
        delete $2;
    }
    | TOKEN_WORD list_of_literals_and_syms {
        Assembler::getInstance().handleWord(args, currentLine);
        deallocArgs();
    }
    | TOKEN_SKIP TOKEN_LITERAL {
        Assembler::getInstance().handleSkip($2, currentLine);
    }
    | TOKEN_ASCII {
    }
    | TOKEN_EQU {
    }
    | TOKEN_END {
        Assembler::getInstance().handleEnd(currentLine);
    }

instruction:
    TOKEN_HALT {
        Assembler::getInstance().handleHaltInstruction(INSTR_NAME::HALT1, currentLine);
    } 
    | TOKEN_INT {
        Assembler::getInstance().handleInterruptInstruction(INSTR_NAME::INT1, currentLine);
    }
    | TOKEN_IRET {
        Assembler::getInstance().handleReturnInstruction(INSTR_NAME::IRET1, currentLine);
    }
    | TOKEN_CALL TOKEN_LITERAL {
        Assembler::getInstance().handleCallInstruction(INSTR_NAME::CALL1, ARG_TYPE::NUMBER, $2, "", currentLine);
    }
    | TOKEN_CALL TOKEN_SYMBOL {
        std::string sym = dereferenceStringPointer($2);
        Assembler::getInstance().handleCallInstruction(INSTR_NAME::CALL1, ARG_TYPE::SYMBOL, 0, sym, currentLine);
    }
    | TOKEN_RET {
        Assembler::getInstance().handleReturnInstruction(INSTR_NAME::RET1, currentLine);
    }
    | TOKEN_JMP TOKEN_LITERAL {
        Assembler::getInstance().handleJumpInstruction(INSTR_NAME::JMP1, ARG_TYPE::NUMBER, 0, 0, $2, "", currentLine);
    }
    | TOKEN_JMP TOKEN_SYMBOL {
        std::string sym = dereferenceStringPointer($2);
        Assembler::getInstance().handleJumpInstruction(INSTR_NAME::JMP1, ARG_TYPE::SYMBOL, 0, 0, 0, sym, currentLine);
        delete $2;
    }
    | TOKEN_BEQ TOKEN_GP_REGISTER TOKEN_COMMA TOKEN_GP_REGISTER TOKEN_COMMA TOKEN_LITERAL {
        Assembler::getInstance().handleJumpInstruction(INSTR_NAME::BEQ1, ARG_TYPE::NUMBER, $2, $4, $6, "", currentLine);
    }
    | TOKEN_BEQ TOKEN_GP_REGISTER TOKEN_COMMA TOKEN_GP_REGISTER TOKEN_COMMA TOKEN_SYMBOL {
        std::string sym = dereferenceStringPointer($6);
        Assembler::getInstance().handleJumpInstruction(INSTR_NAME::BEQ1, ARG_TYPE::SYMBOL, $2, $4, 0, sym, currentLine);
        delete $6;
    }
    | TOKEN_BNE TOKEN_GP_REGISTER TOKEN_COMMA TOKEN_GP_REGISTER TOKEN_COMMA TOKEN_LITERAL {
        Assembler::getInstance().handleJumpInstruction(INSTR_NAME::BNE1, ARG_TYPE::NUMBER, $2, $4, $6, "", currentLine);
    }
    | TOKEN_BNE TOKEN_GP_REGISTER TOKEN_COMMA TOKEN_GP_REGISTER TOKEN_COMMA TOKEN_SYMBOL {
        std::string sym = dereferenceStringPointer($6);
        Assembler::getInstance().handleJumpInstruction(INSTR_NAME::BNE1, ARG_TYPE::SYMBOL, $2, $4, 0, sym, currentLine);
        delete $6;
    }
    | TOKEN_BGT TOKEN_GP_REGISTER TOKEN_COMMA TOKEN_GP_REGISTER TOKEN_COMMA TOKEN_LITERAL {
        Assembler::getInstance().handleJumpInstruction(INSTR_NAME::BGT1, ARG_TYPE::NUMBER, $2, $4, $6, "", currentLine);
    }
    | TOKEN_BGT TOKEN_GP_REGISTER TOKEN_COMMA TOKEN_GP_REGISTER TOKEN_COMMA TOKEN_SYMBOL {
        std::string sym = dereferenceStringPointer($6);
        Assembler::getInstance().handleJumpInstruction(INSTR_NAME::BGT1, ARG_TYPE::SYMBOL, $2, $4, 0, sym, currentLine);
        delete $6;
    }
    | TOKEN_PUSH TOKEN_GP_REGISTER {
        Assembler::getInstance().handleStackInstruction(INSTR_NAME::PUSH1, $2, currentLine);
    }
    | TOKEN_POP TOKEN_GP_REGISTER {
        Assembler::getInstance().handleStackInstruction(INSTR_NAME::POP1, $2, currentLine);
    }
    | TOKEN_XCHG TOKEN_GP_REGISTER TOKEN_COMMA TOKEN_GP_REGISTER {
        Assembler::getInstance().handleXCHGInstruction(INSTR_NAME::XCHG1, $2, $4, currentLine);
    }
    | TOKEN_ADD TOKEN_GP_REGISTER TOKEN_COMMA TOKEN_GP_REGISTER {
        Assembler::getInstance().handleArithmeticInstruction(INSTR_NAME::ADD1, $2, $4, currentLine);
    }
    | TOKEN_SUB TOKEN_GP_REGISTER TOKEN_COMMA TOKEN_GP_REGISTER {
        Assembler::getInstance().handleArithmeticInstruction(INSTR_NAME::SUB1, $2, $4, currentLine);
    }
    | TOKEN_MUL TOKEN_GP_REGISTER TOKEN_COMMA TOKEN_GP_REGISTER {
        Assembler::getInstance().handleArithmeticInstruction(INSTR_NAME::MUL1, $2, $4, currentLine);
    }
    | TOKEN_DIV TOKEN_GP_REGISTER TOKEN_COMMA TOKEN_GP_REGISTER {
        Assembler::getInstance().handleArithmeticInstruction(INSTR_NAME::DIV1, $2, $4, currentLine);
    }
    | TOKEN_NOT TOKEN_GP_REGISTER TOKEN_COMMA TOKEN_GP_REGISTER {
        Assembler::getInstance().handleLogicInstruction(INSTR_NAME::NOT1, $2, $4, currentLine);
    }
    | TOKEN_AND TOKEN_GP_REGISTER TOKEN_COMMA TOKEN_GP_REGISTER {
        Assembler::getInstance().handleLogicInstruction(INSTR_NAME::AND1, $2, $4, currentLine);
    }
    | TOKEN_OR TOKEN_GP_REGISTER TOKEN_COMMA TOKEN_GP_REGISTER {
        Assembler::getInstance().handleLogicInstruction(INSTR_NAME::OR1, $2, $4, currentLine);
    }
    | TOKEN_XOR TOKEN_GP_REGISTER TOKEN_COMMA TOKEN_GP_REGISTER {
        Assembler::getInstance().handleLogicInstruction(INSTR_NAME::XOR1, $2, $4, currentLine);
    }
    | TOKEN_SHL TOKEN_GP_REGISTER TOKEN_COMMA TOKEN_GP_REGISTER {
        Assembler::getInstance().handleShiftInstruction(INSTR_NAME::SHL1, $2, $4, currentLine);
    }
    | TOKEN_SHR TOKEN_GP_REGISTER TOKEN_COMMA TOKEN_GP_REGISTER {
        Assembler::getInstance().handleShiftInstruction(INSTR_NAME::SHR1, $2, $4, currentLine);
    }
    | TOKEN_LD TOKEN_IMM TOKEN_LITERAL TOKEN_COMMA TOKEN_GP_REGISTER {
        Assembler::getInstance().handleLoadInstruction(INSTR_NAME::LD1, ADDR_TYPE::IMMED, ARG_TYPE::NUMBER, 0, $5, $3, "", currentLine);
    }
    | TOKEN_LD TOKEN_IMM TOKEN_SYMBOL TOKEN_COMMA TOKEN_GP_REGISTER {
        std::string sym = dereferenceStringPointer($3);
        Assembler::getInstance().handleLoadInstruction(INSTR_NAME::LD1, ADDR_TYPE::IMMED, ARG_TYPE::SYMBOL, 0, $5, 0, sym, currentLine);
        delete $3;
    }
    | TOKEN_LD TOKEN_LITERAL TOKEN_COMMA TOKEN_GP_REGISTER {
        Assembler::getInstance().handleLoadInstruction(INSTR_NAME::LD1, ADDR_TYPE::MEMDIR, ARG_TYPE::NUMBER, 0, $4, $2, "", currentLine);
    }
    | TOKEN_LD TOKEN_SYMBOL TOKEN_COMMA TOKEN_GP_REGISTER {
        std::string sym = dereferenceStringPointer($2);
        Assembler::getInstance().handleLoadInstruction(INSTR_NAME::LD1, ADDR_TYPE::MEMDIR, ARG_TYPE::SYMBOL, 0, $4, 0, sym, currentLine);
        delete $2;
    }
    | TOKEN_LD TOKEN_GP_REGISTER TOKEN_COMMA TOKEN_GP_REGISTER {
        Assembler::getInstance().handleLoadInstruction(INSTR_NAME::LD1, ADDR_TYPE::REGDIR, ARG_TYPE::NUMBER, $2, $4, 0, "", currentLine);
    }
    | TOKEN_LD TOKEN_LEFT_BRACKET TOKEN_GP_REGISTER TOKEN_RIGHT_BRACKET TOKEN_COMMA TOKEN_GP_REGISTER {
        Assembler::getInstance().handleLoadInstruction(INSTR_NAME::LD1, ADDR_TYPE::REGIND, ARG_TYPE::NUMBER, $3, $6, 0, "", currentLine);
    }
    | TOKEN_LD TOKEN_LEFT_BRACKET TOKEN_GP_REGISTER TOKEN_PLUS TOKEN_LITERAL TOKEN_RIGHT_BRACKET TOKEN_COMMA TOKEN_GP_REGISTER {
        Assembler::getInstance().handleLoadInstruction(INSTR_NAME::LD1, ADDR_TYPE::REGINDPOM, ARG_TYPE::NUMBER, $3, $8, $5, "", currentLine);
    }
    | TOKEN_LD TOKEN_LEFT_BRACKET TOKEN_GP_REGISTER TOKEN_PLUS TOKEN_SYMBOL TOKEN_RIGHT_BRACKET TOKEN_COMMA TOKEN_GP_REGISTER {
        std::string sym = dereferenceStringPointer($5);
        Assembler::getInstance().handleLoadInstruction(INSTR_NAME::LD1, ADDR_TYPE::REGINDPOM, ARG_TYPE::SYMBOL, $3, $8, 0, sym, currentLine);
        delete $5;
    }
    | TOKEN_ST TOKEN_GP_REGISTER TOKEN_COMMA TOKEN_IMM TOKEN_LITERAL {
        Assembler::getInstance().handleStoreInstruction(INSTR_NAME::ST1, ADDR_TYPE::IMMED, ARG_TYPE::NUMBER, $2, 0, $5, "", currentLine);
    }
    | TOKEN_ST TOKEN_GP_REGISTER TOKEN_COMMA TOKEN_IMM TOKEN_SYMBOL {
        std::string sym = dereferenceStringPointer($5);
        Assembler::getInstance().handleStoreInstruction(INSTR_NAME::ST1, ADDR_TYPE::IMMED, ARG_TYPE::SYMBOL, $2, 0, 0, sym, currentLine);
        delete $5;
    }
    | TOKEN_ST TOKEN_GP_REGISTER TOKEN_COMMA TOKEN_LITERAL {
        Assembler::getInstance().handleStoreInstruction(INSTR_NAME::ST1, ADDR_TYPE::MEMDIR, ARG_TYPE::NUMBER, $2, 0, $4, "", currentLine);
    }
    | TOKEN_ST TOKEN_GP_REGISTER TOKEN_COMMA TOKEN_SYMBOL {
        std::string sym = dereferenceStringPointer($4);
        Assembler::getInstance().handleStoreInstruction(INSTR_NAME::ST1, ADDR_TYPE::MEMDIR, ARG_TYPE::SYMBOL, $2, 0, 0, sym, currentLine);
        delete $4;
    }
    | TOKEN_ST TOKEN_GP_REGISTER TOKEN_COMMA TOKEN_GP_REGISTER {
        Assembler::getInstance().handleStoreInstruction(INSTR_NAME::ST1, ADDR_TYPE::REGDIR, ARG_TYPE::NUMBER, $2, $4, 0, "", currentLine);
    }
    | TOKEN_ST TOKEN_GP_REGISTER TOKEN_COMMA TOKEN_LEFT_BRACKET TOKEN_GP_REGISTER TOKEN_RIGHT_BRACKET {
        Assembler::getInstance().handleStoreInstruction(INSTR_NAME::ST1, ADDR_TYPE::REGIND, ARG_TYPE::NUMBER, $2, $5, 0, "", currentLine);
    }
    | TOKEN_ST TOKEN_GP_REGISTER TOKEN_COMMA TOKEN_LEFT_BRACKET TOKEN_GP_REGISTER TOKEN_PLUS TOKEN_LITERAL TOKEN_RIGHT_BRACKET {
        Assembler::getInstance().handleStoreInstruction(INSTR_NAME::ST1, ADDR_TYPE::REGINDPOM, ARG_TYPE::NUMBER, $2, $5, $7, "", currentLine);
    }
    | TOKEN_ST TOKEN_GP_REGISTER TOKEN_COMMA TOKEN_LEFT_BRACKET TOKEN_GP_REGISTER TOKEN_PLUS TOKEN_SYMBOL TOKEN_RIGHT_BRACKET {
        std::string sym = dereferenceStringPointer($7);
        Assembler::getInstance().handleStoreInstruction(INSTR_NAME::ST1, ADDR_TYPE::REGINDPOM, ARG_TYPE::SYMBOL, $2, $5, 0, sym, currentLine);
        delete $7;
    }
    | TOKEN_CSRRD TOKEN_CS_REGISTER TOKEN_COMMA TOKEN_GP_REGISTER {
        Assembler::getInstance().handleCSRInstruction(INSTR_NAME::CSRRD1, $4, $2, currentLine);
    }
    | TOKEN_CSRWR TOKEN_GP_REGISTER TOKEN_COMMA TOKEN_CS_REGISTER {
        Assembler::getInstance().handleCSRInstruction(INSTR_NAME::CSRWR1, $4, $2, currentLine);
    }




list_of_literals_and_syms:
    TOKEN_SYMBOL {
        addSymbolToArgs($1);    // u "args" se nalaze argumenti
    }
    | list_of_literals_and_syms TOKEN_COMMA TOKEN_SYMBOL {
        addSymbolToArgs($3);    // u "args" se nalaze argumenti
    }
    | TOKEN_LITERAL {
        addLiteralToArgs($1);   // u "args" se nalaze argumenti
    }
    | list_of_literals_and_syms TOKEN_COMMA TOKEN_LITERAL {
        addLiteralToArgs($3);   // u "args" se nalaze argumenti
    }

list_of_symbols:
    TOKEN_SYMBOL {
        addSymbolToArgs($1);    // u "args" se nalaze argumenti
    }
    | list_of_symbols TOKEN_COMMA TOKEN_SYMBOL {
        addSymbolToArgs($3);    // u "args" se nalaze argumenti
    }