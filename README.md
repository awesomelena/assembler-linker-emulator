# Assembler, Linker and Emulator for an Abstract 32-bit Computer System

A complete toolchain written from scratch in C++17: a **single-pass assembler**, an
**architecture-independent linker**, and an **interpretive emulator** for a 32-bit
abstract computer system with a terminal and a timer peripheral.

This was built as a university project for the *System Software* course
(Faculty of Electrical Engineering, University of Belgrade). The source code
identifiers and comments are in Serbian; this README maps everything to English
so the code can be read by anyone.

---

## What the toolchain does

```
   program.s ──▶ [ assembler ] ──▶ program.o ──▶ [ linker ] ──▶ program.hex ──▶ [ emulator ]
   assembly        translates        object        merges,        memory           executes
   source          to machine        file          relocates      image            instructions
                   code                                                            until halt
```

**Assembler** translates one assembly source file into an *object file* containing
machine code plus metadata (symbol table, relocation records). It is **single-pass**:
it cannot look ahead, so forward references are recorded as relocations and resolved
later rather than by a second scan.

**Linker** merges one or more object files: it concatenates same-named sections,
builds a global symbol table, assigns final memory addresses, and resolves all
relocation records. It can emit either a memory image (`-hex`) or a merged,
still-relocatable object file (`-relocatable`) for further linking.

**Emulator** loads the memory image and runs a fetch–decode–execute loop, modelling
16 general-purpose registers, control/status registers, memory, an interrupt
mechanism, a terminal (keyboard + display) and a periodic timer.

---

## Target architecture (summary)

- 32-bit, von Neumann, byte-addressable, **little-endian**
- 2³² byte address space; execution starts at `0x40000000` after reset
- 16 general-purpose registers `r0`–`r15`, where `r0` is hardwired to zero,
  `r14` = stack pointer, `r15` = program counter
- Control/status registers: `status`, `handler`, `cause`
- **Fixed 4-byte instructions** with the layout:

```
  byte 0      byte 1        byte 2         byte 3
 [OC][MOD]  [RegA][RegB]  [RegC][Disp]  [Disp][Disp]
  4    4     4     4       4     4          8      bits
```

- `Disp` is a **12-bit signed displacement**. Because 32-bit constants and addresses
  do not fit in 12 bits, the assembler uses a **literal pool** placed at the end of
  each section, and instructions reach it PC-relatively.
- Memory-mapped peripheral registers: `term_out` at `0xFFFFFF00`,
  `term_in` at `0xFFFFFF04`, `tim_cfg` at `0xFFFFFF10`

---

## Build and run

Requires a Linux environment (or WSL) with `g++` supporting C++17, and `make`.

```bash
make                 # builds asembler, assembler, linker, emulator
```

Note: `assembler` is an identical copy of `asembler` — the course specification uses
the Serbian name, while the official test scripts invoke the English one.

Example: assemble two files, link them to fixed addresses, and run.

```bash
./asembler -o main.o main.s
./asembler -o math.o math.s
./linker -hex -place=code@0x40000000 -place=math@0xF0000000 \
         -o program.hex main.o math.o
./emulator program.hex
```

Sample programs demonstrating each feature live in `tests/`
(not included in this repository, which contains only `inc/` and `src/`).

---

## Source file guide

Serbian identifiers are used throughout the code. The tables below give the English
meaning of each filename and describe what the module does.

### Shared foundations

| File | Name means | Purpose |
|------|-----------|---------|
| `inc/tipovi.hpp` | *types* | Fixed-width integer aliases (`u8`, `u16`, `u32`, `i32`). Exact widths matter because instructions are 4 bytes and addresses are 32 bits. |
| `inc/instrukcije.hpp` | *instructions* | Instruction format: opcode constants, `sklopiInstrukciju` (*assemble instruction* — packs fields into a 32-bit word) and `dekodirajInstrukciju` (*decode instruction* — unpacks, with sign-extension of the 12-bit displacement). Also holds status-register bit masks and interrupt cause codes. |
| `inc/tabela_simbola.hpp`, `src/tabela_simbola.cpp` | *symbol table* | The `Simbol` record (name, section index, value, `jeDefinisan`/`jeGlobalan`/`jeEksteran` = is-defined/is-global/is-external) and the table itself, with lookup by name and stable ordinal indices used by relocations. |
| `inc/sekcija.hpp` | *section* | A named block of bytes. Its `lokacija()` (*location counter*) is simply the current content size, so appending a byte advances it automatically. Also holds the section's relocations, its literal pool, and the patch list. |
| `inc/relokacija.hpp` | *relocation* | A relocation record: *where* to patch (offset), *how* (type), *whose* value to use (symbol index), and the addend. A deferred addition, executed later by the linker. |
| `inc/registri.hpp`, `src/registri.cpp` | *registers* | Maps register names to indices — `r0`–`r15`, the aliases `sp` and `pc`, and the control registers `status`/`handler`/`cause`. |

### Assembler

| File | Name means | Purpose |
|------|-----------|---------|
| `src/asembler.cpp` | *assembler* | `main`: command-line handling (`-o`), reading the source file, invoking assembly, writing the object file. |
| `inc/asembler.hpp` | *assembler* | The `Asembler` class declaration — the state of one assembly run: symbol table, sections, current section, deferred `.equ` definitions. |
| `inc/lekser.hpp`, `src/lekser.cpp` | *lexer* | Splits one source line into tokens (identifiers, numbers, registers, punctuation, strings). A directive such as `.word` is lexed as a single identifier beginning with a dot. The `-` character is a numeric sign or a subtraction operator depending on the preceding token. |
| `src/parser.cpp` | *parser* | The main loop: reads lines, recognises labels, directives and instructions, and fills the symbol table and sections. Implements `.global`, `.extern`, `.section`, `.word`, `.skip`, `.ascii`, `.equ`, `.end`. Also contains `izracunajEquDefinicije` (*evaluate .equ definitions*) — see "Design notes" below. |
| `inc/operand.hpp`, `src/operandi.cpp` | *operand(s)* | Parses all eight addressing modes and generates code for `ld`, `st` and the jump/branch family. Modes needing a full 32-bit value go through the literal pool. |
| `src/naredbe.cpp` | *instructions/statements* | Code generation for instructions without complex operands: arithmetic, logic, shifts, `xchg`, stack operations (`push`/`pop`), `ret`, `iret`, and control-register access (`csrrd`/`csrwr`). |
| `src/bazen.cpp` | *pool* | The literal pool and relocation machinery: `dodajLiteralUBazen`/`dodajSimbolUBazen` (*add literal/symbol to pool*), `isprazniBazen` (*flush the pool* — writes entries after the code, patches PC-relative displacements, creates relocations), and `finalizujRelokacije` (*finalise relocations*). |
| `inc/izlaz.hpp`, `src/izlaz.cpp` | *output* | Writes the object file in a human-readable text format: symbol table, section contents as hex bytes, and relocation records. Readable output makes debugging the linker far easier. |

### Linker

| File | Name means | Purpose |
|------|-----------|---------|
| `src/linker.cpp` | *linker* | `main`: parses `-o`, `-hex`, `-relocatable` and `-place=section@address`, loads the input object files, and drives merge → placement → relocation → output. |
| `inc/linker.hpp` | *linker* | Declarations for the `Linker` class, merged sections (`SpojenaSekcija`), global symbols (`GlobalniSimbol`, including *absolute* symbols originating from `.equ`), and the per-file section contribution key. |
| `inc/predmetni_program.hpp`, `src/citac.cpp` | *object program*, *reader* | Loads an object file back into memory — the mirror image of `izlaz.cpp`. Implemented as a small state machine over the `#tabela_simbola` / `#sekcija` / `#relokacije` blocks. |
| `src/jezgro.cpp` | *core* | The heart of the linker: `spojiSekcije` (*merge sections*, tracking each file's base offset within a merged section), `izgradiGlobalnuTabelu` (*build the global symbol table*, detecting multiple definitions), `rasporedi` (*place* sections, honouring `-place` and detecting overlaps), `razresiRelokacije` (*resolve relocations*), and `generisiRelocatable` (emit a merged, still-relocatable object file). |
| `inc/hex_izlaz.hpp`, `src/hex_izlaz.cpp` | *hex output* | Emits the memory image as `address: bb bb bb ...` lines. Each section is written as its own block, so widely separated sections do not produce gigabytes of padding. |

### Emulator

| File | Name means | Purpose |
|------|-----------|---------|
| `src/emulator.cpp` | *emulator* | `main`: loads the hex image, runs the emulation, prints the final processor state. |
| `inc/procesor.hpp` | *processor* | Processor state: general-purpose and control registers, with `r0` reads forced to zero and writes to it ignored. |
| `inc/memorija.hpp` | *memory* | Sparse memory model (`address → byte` map) with little-endian 32-bit word access. Uninitialised memory reads as zero, like real RAM after reset. |
| `inc/ucitavanje.hpp`, `src/ucitavanje.cpp` | *loading* | Parses the linker's hex output into memory. |
| `inc/emulacija.hpp`, `src/emulacija.cpp` | *emulation* | The fetch–decode–execute loop and the implementation of every instruction. Also `udjiUPrekid` (*enter interrupt*) and the interrupt checks performed at instruction boundaries. |
| `inc/terminal.hpp`, `src/terminal.cpp` | *terminal* | The terminal peripheral. Output: writes to `term_out` print a character. Input: the terminal is switched to raw mode via `termios` (no line buffering, no echo) and polled non-blockingly, so a keypress raises an interrupt without stalling execution. Original terminal settings are always restored, including on error. |
| `inc/tajmer.hpp`, `src/tajmer.cpp` | *timer* | The timer peripheral. Generates a periodic interrupt whose period is selected by `tim_cfg` (500 ms … 60 s). Uses `steady_clock` — a monotonic clock — because we are measuring an elapsed interval, not wall-clock time. |

---

## Design notes

A few decisions worth explaining, since they shape the code.

**The literal pool.** The 12-bit displacement cannot hold a 32-bit constant or
address. Instead of embedding the value in the instruction, the assembler appends it
to a pool at the end of the section and has the instruction load it PC-relatively.
Because the pool's position is unknown while code is still being generated, each such
instruction is recorded in a *patch list* and its displacement is filled in when the
section ends.

**Relocations: symbol vs. section.** A relocation targeting a *global or external*
symbol points at the symbol itself. A relocation targeting a *local* symbol instead
points at the symbol's **section**, with the symbol's offset as the addend. Local
symbols need not survive into the final symbol table, whereas sections always do —
which is also why sections have their own entries in the symbol table.

**Deferring decisions — the single-pass theme.** Twice the same pattern appears:
when information is not yet available, record the question and answer it at the end.
A relocation stores only the symbol *name*, and whether it is local or global is
decided in `finalizujRelokacije` once every symbol is known. Likewise `.equ` stores
its expression tokens and evaluates them in `izracunajEquDefinicije`, which is what
makes `.equ len, msg_end - msg_start` work even though both labels appear further
down the file.

**Constant expressions in `.equ`.** The difference of two symbols in the same section
is a constant: if the linker moves the section, both shift equally. To check this, the
evaluator tracks a per-section *weight* (occurrences with `+` minus occurrences with
`-`) alongside the numeric value. All weights must cancel to zero, so `end - start`
is accepted while `end + start` is rejected.

**Interrupts.** Instructions execute atomically; interrupt requests are only serviced
at instruction boundaries. On entry the processor pushes `status` then `pc`, so `pc`
sits on top of the stack. `iret` must therefore read `status` from `sp+4` *before*
restoring `pc` — restoring `pc` first would transfer control and the second
instruction would never run.

---

## Feature coverage

**Assembler directives:** `.global`, `.extern`, `.section`, `.word`, `.skip`,
`.ascii`, `.equ` (with `+`/`-` expressions over literals and symbols), `.end`

**Instructions:** `halt`, `int`, `iret`, `call`, `ret`, `jmp`, `beq`, `bne`, `bgt`,
`push`, `pop`, `xchg`, `add`, `sub`, `mul`, `div`, `not`, `and`, `or`, `xor`, `shl`,
`shr`, `ld`, `st`, `csrrd`, `csrwr`

**Addressing modes:** `$literal`, `$symbol`, `literal`, `symbol`, `%reg`, `[%reg]`,
`[%reg + literal]`, `[%reg + symbol]`

**Linker:** `-hex`, `-relocatable`, `-place=section@address`, section merging,
and diagnostics for multiple definitions, unresolved symbols and overlapping sections

**Emulator:** full instruction set, interrupt mechanism, terminal (input and output),
timer

---

## Repository contents

This repository contains only the `inc/` and `src/` directories. The build file and
the sample assembly programs from the original project layout are not included here.
