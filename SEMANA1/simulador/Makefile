# GNU Makefile for ASOC-V Simulator + Terminal + Assembler

# Tools
CC ?= cc
CFLAGS ?= -g -O2 -Wall -Wextra
LDFLAGS_SIMULADOR ?= -pthread
LDFLAGS_TERMINAL ?=

# Targets
BINARIES := simulador terminal
ROM := rom.bin
ASM := assembler.py
ASM_SRC := programa.asoc

# Default target
all: $(BINARIES) $(ROM)
	@echo "Binaries built: ./simulador, ./terminal"
	@echo "Using shared memory for I/O (no FIFOs needed)"
	@echo
	@echo "Build complete."
	@echo
	@echo "Next steps:"
	@echo "- In one terminal:   ./simulador"
	@echo "- In another:         ./terminal"
	@echo "Type in the terminal window to send keystrokes to the VM. Output from the VM appears here."

# Build binaries
simulador: simulador.c
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS_SIMULADOR)

terminal: terminal.c
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS_TERMINAL)

# Assemble ROM (best-effort: skip if python3 is missing)
$(ROM): $(ASM) $(ASM_SRC)
	@set -e; \
	if command -v python3 >/dev/null 2>&1; then \
		echo "Assembling $(ASM_SRC) -> $(ROM)"; \
		python3 $(ASM) $(ASM_SRC) -o $(ROM); \
	else \
		echo "python3 not found; skipping assembly" 1>&2; \
	fi

# Convenience targets
assemble: $(ROM)

run-simulador: simulador
	./simulador

run-terminal: terminal
	./terminal

clean:
	rm -f $(BINARIES) $(ROM)

.PHONY: all clean assemble run-simulador run-terminal
