CC = gcc

CFLAGS = -Wall -Wextra -O2

C_SRC = src/assemble.c
DISASM_SRC = src/disassemble.c
OBJ = $(C_SRC:.c=.o) $(DISASM_SRC:.c=.o)

.PHONY: default clean run


default: riscasm riscdisasm

riscasm: $(OBJ)
	$(CC) $(CFLAGS) -o riscasm src/assemble.o

riscdisasm: $(OBJ)
	$(CC) $(CFLAGS) -o riscdisasm src/disassemble.o

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) riscasm

run: riscasm
	./riscasm test.asm test.bin