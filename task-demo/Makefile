CC_SRC = main.c GameOfLife.c
CXX_SRC = simSFML.cc
SRC = $(CC_SRC) $(CXX_SRC)
OBJ = $(CC_SRC:.c=.o) $(CXX_SRC:.cc=.o)
PROG = game-of-life
LIBS =-lsfml-graphics -lsfml-window -lsfml-system
CXXFLAGS +=-Wall -Wextra -ggdb -std=c++17
CFLAGS +=-Wall -Wextra -ggdb

$(PROG): $(OBJ)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $(PROG) $(OBJ) $(LIBS)

.PHONY: clean clang-format emit-llvm
clean:
	rm -f $(OBJ) depend.* *.ll

clang-format:
	clang-format --style=LLVM -i $(SRC)

emit-llvm: $(CC_SRC:.c=.ll)

%.ll: %.c
	clang -S -emit-llvm $(CFLAGS) $<

.PHONY: depend
depend: depend.CC depend.CXX

depend.%:
	$($*) -MM $($*_SRC) > $@

-include depend.CC depend.CXX
