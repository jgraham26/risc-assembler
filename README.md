RISC Assembler & Disassembler
=================================

Small assembler and disassembler toolset for a MIPS-like ISA.

Features
- Preprocessor with simple macros (e.g. `DIV` macro expansion)
- Two-pass assembler producing 32-bit little-endian binaries
- Simple disassembler for produced binaries

Requirements
- GCC (or compatible C compiler)
- `make`

Build
-----
From the project root run:

```
make
```

This produces two tools in the repo root:
- `riscasm` — the assembler/preprocessor
- `riscdisasm` — the disassembler

Common targets
- `make` or `make default` — build both tools
- `make clean` — remove object files and `riscasm`

Usage
-----

Assemble (three modes):

- Preprocess only (text assembly output):

```
./riscasm -E input.asm output.pre
```

- Assemble only (skip preprocessing):

```
./riscasm -S input_preprocessed.asm output.bin
```

- Full pipeline (preprocess then assemble) — default:

```
./riscasm input.asm output.bin
```

Disassemble
-----------

To disassemble a produced binary (4-byte little-endian instructions):

```
./riscdisasm output.bin
```

Repository layout
- [Makefile](Makefile)
- [src/assemble.c](src/assemble.c) — assembler + preprocessor
- [src/disassemble.c](src/disassemble.c) — disassembler
- `test.asm` — example test assembly used by `make run`
- `test_programs/` — extra sample assembly programs

Examples
--------
Build and run the provided test pipeline:

```
make
./riscasm test.asm test.bin
./riscdisasm test.bin
```

Notes
- The assembler supports `.ORG`, `.DB`, `.DW`, `.DD` directives and a limited instruction set (R-type, I-type, memory, branch and jump).
- Preprocessor preserves comments and expands a basic `DIV` macro into a sequence of instructions.

License
- MIT License
