# Copying and distribution of this file, with or without modification,
# are permitted in any medium without royalty provided the copyright
# notice and this notice are preserved.  This file is offered as-is,
# without any warranty.

PREFIX ?= /usr
BIN ?= /bin
BINDIR ?= $(PREFIX)$(BIN)
DATA ?= /share
DATADIR ?= $(PREFIX)$(DATA)
LICENSEDIR ?= $(DATADIR)/licenses

PKGNAME = total-lockdown
COMMAND = total-lockdown

OPTIMISE = -Og -g

WARN = -Wall -Wextra -pedantic -Wdouble-promotion -Wformat=2 -Winit-self -Wmissing-include-dirs  \
       -Wtrampolines -Wfloat-equal -Wshadow -Wmissing-prototypes -Wmissing-declarations          \
       -Wredundant-decls -Wnested-externs -Winline -Wno-variadic-macros -Wswitch-default         \
       -Wsync-nand -Wunsafe-loop-optimizations -Wcast-align -Wstrict-overflow -Wundef            \
       -Wbad-function-cast -Wcast-qual -Wpacked -Wlogical-op -Wstrict-prototypes -Wconversion    \
       -Wold-style-definition -Wvector-operation-performance -Wunsuffixed-float-constants        \
       -Wsuggest-attribute=const -Wsuggest-attribute=noreturn -Wsuggest-attribute=pure           \
       -Wsuggest-attribute=format -Wnormalized=nfkc -Wdeclaration-after-statement -ftree-vrp     \
       -fstrict-aliasing -fipa-pure-const -fstack-usage -fstrict-overflow -funsafe-loop-optimizations

STD = gnu99

FLAGS = $(OPTIMISE) -std=$(STD) $(WARN) $(CFLAGS) $(CPPFLAGS) $(LDFLAGS)



.PHONY: all
all: bin/total-lockdown

bin/total-lockdown: obj/program.o obj/keyboard.o obj/kbddriver.o
	@mkdir -p bin
	$(CC) $(FLAGS) -o $@ $^

obj/program.o: src/program.c src/layout.c src/*.h
	@mkdir -p obj
	$(CC) $(FLAGS) -c -o $@ $<

obj/%.o: src/%.c src/*.h
	@mkdir -p obj
	$(CC) $(FLAGS) -c -o $@ $<


.PHONY: clean
clean:
	-rm -r bin obj
