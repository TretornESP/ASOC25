# ASOC-V Simulator + Assembler

This workspace contains a simple simulator (`simulador.c`), a terminal bridge (`terminal.c`), and a Python assembler (`assembler.py`) for the ASOC-V architecture described in `descripcion.txt`.

## Files
- `simulador.c` — CPU + devices (GPU/keyboard) simulation using shared memory.
- `terminal.c` — Host-side terminal that bridges stdin/stdout to the shared memory ring buffers.
- `assembler.py` — Assembler that converts `.asoc` files to `rom.bin` loadable by the simulator.
- `programa.asoc` — Sample program that adds two memory values and stores the result.
- `Makefile` — Builds the C programs and assembles `programa.asoc` to `rom.bin`.

## Assembly format
- Instruction encoding: `[ opcode:8 | reg:4 | addr_mode:4 | operand:16 ]`.
- Registers: `X=0`, `ACC=1`.
- Addressing modes: `#imm` (Immediate), `label` or `[addr]` (Direct), `@label` (Indirect), `label(X)` (Indexed by X).
- Directives:
  - `ORG <addr>` — Set the current output address.
  - `WORD <value>` — Emit a raw 32-bit word at the current address.
- Comments start with `;` or `//`.

Example:
```
ORG 0x0000
LD  ACC, [DATA1]
ADD ACC, [DATA2]
ST  ACC, [RESULT]
HALT

ORG 0x0100
DATA1: WORD 5
DATA2: WORD 7
RESULT: WORD 0
```

## Build and assemble
Run:

```bash
make
```

This produces:
- `./simulador` and `./terminal` binaries
- `rom.bin` assembled from `programa.asoc` (skipped if `python3` is not found)

## Run
Open two terminals:

1) Terminal A (simulator):
```bash
./simulador
```

2) Terminal B (host bridge):
```bash
./terminal
```

The sample program computes 5 + 7, stores the result at `RESULT` (0x0102) and halts. You can modify `programa.asoc` and re-run `./build.sh` to reassemble.

## Notes
- The simulator loads `rom.bin` (32-bit words). Uninitialized memory defaults to zero.
- Terminal I/O uses POSIX shared memory segment `/asoc_shm` with two ring buffers (VM→Host and Host→VM).
- Clock speed is set to 0.05 seconds per tick (`VELOCIDAD_RELOJ_US` in `simulador.c`).
