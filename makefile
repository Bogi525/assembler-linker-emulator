OBJS_ASS  = src/helpers.o src/assembler.o src/parser.o src/lexer.o src/main_assembler.o
OBJS_LNK = src/linker.o src/main_linker.o
OBJS_EMU = src/emulator.o src/main_emulator.o

###

all: asembler linker emulator

###

asembler: $(OBJS_ASS)
		g++ -o $@ $(OBJS_ASS)

linker: $(OBJS_LNK)
		g++ -o $@ $(OBJS_LNK)

emulator: $(OBJS_EMU)
		g++ -o $@ $(OBJS_EMU)

###

src/main_assembler.o: src/main_assembler.cpp
		g++ -c -o $@ $<

src/main_linker.o: src/main_linker.cpp
		g++ -c -o $@ $<

src/main_emulator.o: src/main_emulator.cpp
		g++ -c -o $@ $<

###

src/helpers.o: src/helpers.cpp inc/helpers.hpp
		g++ -c -o $@ $<

src/assembler.o: src/assembler.cpp inc/assembler.hpp
		g++ -c -o $@ $<

src/linker.o: src/linker.cpp inc/linker.hpp
		g++ -c -o $@ $<

src/emulator.o: src/emulator.cpp inc/emulator.hpp

###

src/lexer.cpp: misc/lexer.l
		flex --outfile=$@ $<

src/parser.cpp: misc/parser.y
		bison -v --defines=inc/parser.hpp --output=$@ $<

src/lexer.o: src/lexer.cpp
		g++ -c -o $@ $<

src/parser.o: src/parser.cpp
		g++ -c -Iinc -o $@ $<

###

clean:
		rm -f assembler asembler linker emulator src/*.o src/lexer.cpp src/parser.cpp inc/parser.hpp src/parser.output assout.txt *.o