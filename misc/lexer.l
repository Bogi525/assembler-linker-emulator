/* this will be a part of lexer.hpp file */
%{
  #include "../inc/helpers.hpp"
  #include "../inc/parser.hpp"
  #include <iostream>
  #include <string>
  int currentLine = 1;
%}


%option noyywrap

GP_REGISTER %(r[0-9]+|sp|pc)
CS_REGISTER %(status|handler|cause)
LEFT_BRACKET "["
RIGHT_BRACKET "]"
PLUS "+"
GLOBAL \.global
EXTERN \.extern
SECTION \.section
WORD \.word
SKIP \.skip
ASCII \.ascii
EQU \.equ
END \.end
SPACE [ \r\t]*
COMMENT #.*
LITERAL -?[0-9]+
HEX_LITERAL 0[xX][0-9A-Fa-f]+
SYMBOL [a-zA-Z_][a-zA-Z0-9_]*
LABEL [a-zA-Z_][a-zA-Z0-9_]*:
IMMEDIATE \$
STRING \"[^"]*\"
ENDL \n
COMMA ","

%%

{GLOBAL} {
  return TOKEN_GLOBAL;
  }
{EXTERN} {
  return TOKEN_EXTERN;
  }
{SECTION} {
  return TOKEN_SECTION;
  }
{WORD} {
  return TOKEN_WORD;
  }
{SKIP} {
  return TOKEN_SKIP;
  }
{ASCII} {
  return TOKEN_ASCII;
  }
{EQU} {
  return TOKEN_EQU;
  }
{END} {
  return TOKEN_END;
  }


halt {
  return TOKEN_HALT;
  }
int {
  return TOKEN_INT;
  }
iret {
  return TOKEN_IRET;
  }
call {
  return TOKEN_CALL;
}
ret {
  return TOKEN_RET;
}
jmp {
  return TOKEN_JMP;
}
beq {
  return TOKEN_BEQ;
}
bne {
  return TOKEN_BNE;
}
bgt {
  return TOKEN_BGT;
}
push {
  return TOKEN_PUSH;
  }
pop {
  return TOKEN_POP;
  }
xchg {
  return TOKEN_XCHG;
  }
add {
  return TOKEN_ADD;
  }
sub {
  return TOKEN_SUB;
  }
mul {
  return TOKEN_MUL;
  }
div {
  return TOKEN_DIV;
  }
not {
  return TOKEN_NOT;
  }
and {
  return TOKEN_AND;
  }
or {
  return TOKEN_OR;
  }
xor {
  return TOKEN_XOR;
  }
shl {
  return TOKEN_SHL;
  }
shr {
  return TOKEN_SHR;
  }
ld {
  return TOKEN_LD;
  }
st {
  return TOKEN_ST;
  }
csrrd {
  return TOKEN_CSRRD;
  }
csrwr {
  return TOKEN_CSRWR;
  }


{SPACE} {}
{COMMENT} {return TOKEN_COMMENT;}

{LITERAL} {
  yylval.number = stringLiteralToInt(yytext);
  return TOKEN_LITERAL;
  }

{HEX_LITERAL} {
  yylval.number = stringHexToInt(yytext);
  return TOKEN_LITERAL;
  }

{SYMBOL} {
  yylval.symbol = new std::string(yytext);
  return TOKEN_SYMBOL;
  }

{LABEL} {
  yylval.symbol = new std::string(yytext);
  return TOKEN_LABEL;
  }


{IMMEDIATE} {
  return TOKEN_IMM;
  }

{STRING} {
  yylval.symbol = new std::string(yytext);
  return TOKEN_STRING;
  }

{ENDL} {return TOKEN_ENDL;}
{COMMA} {return TOKEN_COMMA;}

{GP_REGISTER} {
  std::string str = yytext;
  if (str == "%pc") {
    yylval.number = 15;
    return TOKEN_GP_REGISTER;
  } else if (str == "%sp") {
    yylval.number = 14;
    return TOKEN_GP_REGISTER;
  } else {
    std::string currStr = yytext;
    if (currStr.length() > 3) {
			yylval.number = ((yytext)[2] - '0') * 10 + ((yytext)[3] - '0');
		}
		else {
			yylval.number = (yytext)[2] - '0';
		}
    return TOKEN_GP_REGISTER;
  }
}

{CS_REGISTER} {
  std::string str = yytext;
  if (str == "%status") {
    yylval.number = 0;
    return TOKEN_CS_REGISTER;
  } else if (str == "%handler") {
    yylval.number = 1;
    return TOKEN_CS_REGISTER;
  } else if (str == "%cause") {
    //std::cout << str << "\n";
    yylval.number = 2;
    return TOKEN_CS_REGISTER;
  }
}

{LEFT_BRACKET} {
  return TOKEN_LEFT_BRACKET;
}

{RIGHT_BRACKET} {
  return TOKEN_RIGHT_BRACKET;
}

{PLUS} {
  return TOKEN_PLUS;
}

%%

int yyerror(const char* msg) {
    printf("lexer error: %s\n", msg);
    printf("line : %d", currentLine);
    return 0;
}
