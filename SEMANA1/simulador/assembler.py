#!/usr/bin/env python3
import argparse
import re
import struct
import sys
from typing import Dict, List, Tuple, Optional
from dataclasses import dataclass

# ASOC-V instruction encoding (32-bit):
# [ opcode:8 | reg:4 | addr_mode:4 | operand:16 ]
# Registers: 0=X, 1=ACC
# Addressing modes: 0=Immediate, 1=Direct, 2=Indirect, 3=Indexed (with X)

OPCODES: Dict[str, int] = {
    'ST': 0,
    'LD': 1,
    'LDI': 2,
    'ADD': 3,
    'SUB': 4,
    'MUL': 5,
    'DIV': 6,
    'MOD': 7,
    'AND': 8,
    'OR': 9,
    'XOR': 10,
    'NOT': 11,
    'JMP': 12,
    'JZ': 13,
    'JN': 14,
    'CLR': 15,
    'NOP': 16,
    'DEC': 17,
    'INC': 18,
    'HALT': 19,
}

REGS: Dict[str, int] = {
    'X': 0,
    'ACC': 1,
}

ADDR_MODES = {
    'IMM': 0,
    'DIR': 1,
    'IND': 2,
    'IDX': 3,
}

comment_re = re.compile(r"(;.*$|//.*$)")
label_re = re.compile(r"^([A-Za-z_][A-Za-z0-9_]*):\s*(.*)$")
org_re = re.compile(r"^ORG\s+(.+)$", re.IGNORECASE)
word_re = re.compile(r"^WORD\s+(.+)$", re.IGNORECASE)

number_re = re.compile(r"^([-+]?)(0x[0-9A-Fa-f]+|\d+)$")
indexed_re = re.compile(r"^(.+)\(X\)$", re.IGNORECASE)

class AsmError(Exception):
    pass

@dataclass
class Item:
    kind: str  # 'instr' or 'word'
    addr: int
    text: str
    fields: Optional[Tuple] = None


def parse_number(token: str) -> int:
    m = number_re.match(token)
    if not m:
        raise AsmError(f"Invalid number: {token}")
    sign, body = m.groups()
    if body.startswith('0x') or body.startswith('0X'):
        val = int(body, 16)
    else:
        val = int(body, 10)
    if sign == '-':
        val = -val
    return val


def parse_operand(token: str, symbols: Dict[str, int]) -> Tuple[int, int, List[str]]:
    # returns (addr_mode, value, unresolved_symbols)
    token = token.strip()
    unresolved: List[str] = []

    # Immediate: #value
    if token.startswith('#'):
        val_tok = token[1:].strip()
        val = None
        if number_re.match(val_tok):
            val = parse_number(val_tok)
        else:
            # immediate label constant (resolve to address value)
            if val_tok in symbols:
                val = symbols[val_tok]
            else:
                unresolved.append(val_tok)
                val = 0
        if val is not None and not (-32768 <= val <= 65535):
            raise AsmError(f"Immediate out of range (16-bit): {val}")
        return (ADDR_MODES['IMM'], val & 0xFFFF, unresolved)

    # Indexed: label(X) or 123(X)
    mi = indexed_re.match(token)
    if mi:
        inner = mi.group(1).strip()
        if number_re.match(inner):
            val = parse_number(inner)
        else:
            if inner in symbols:
                val = symbols[inner]
            else:
                unresolved.append(inner)
                val = 0
        return (ADDR_MODES['IDX'], val & 0xFFFF, unresolved)

    # Indirect: @label or @123
    if token.startswith('@'):
        inner = token[1:].strip()
        if number_re.match(inner):
            val = parse_number(inner)
        else:
            if inner in symbols:
                val = symbols[inner]
            else:
                unresolved.append(inner)
                val = 0
        return (ADDR_MODES['IND'], val & 0xFFFF, unresolved)

    # Direct: label or 123 or [label]
    if token.startswith('[') and token.endswith(']'):
        token = token[1:-1].strip()
    if number_re.match(token):
        val = parse_number(token)
        return (ADDR_MODES['DIR'], val & 0xFFFF, unresolved)
    else:
        if token in symbols:
            return (ADDR_MODES['DIR'], symbols[token] & 0xFFFF, unresolved)
        unresolved.append(token)
        return (ADDR_MODES['DIR'], 0, unresolved)


def encode(op: int, reg: int, am: int, operand: int) -> int:
    return ((op & 0xFF) << 24) | ((reg & 0x0F) << 20) | ((am & 0x0F) << 16) | (operand & 0xFFFF)


def assemble(lines: List[str]) -> Tuple[List[int], Dict[str, int]]:
    # Pass 1: collect symbols and items
    loc = 0
    symbols: Dict[str, int] = {}
    items: List[Item] = []

    def clean(line: str) -> str:
        line = comment_re.sub('', line)
        return line.strip()

    pending_lines: List[Tuple[int, str]] = []  # (address, raw line)

    for raw in lines:
        line = clean(raw)
        if not line:
            continue
        # Handle label at start
        m = label_re.match(line)
        if m:
            label, rest = m.groups()
            if label in symbols:
                raise AsmError(f"Duplicate label: {label}")
            symbols[label] = loc
            line = rest.strip()
            if not line:
                continue
        # Directives
        mo = org_re.match(line)
        if mo:
            loc = parse_number(mo.group(1)) & 0xFFFF
            continue
        mw = word_re.match(line)
        if mw:
            items.append(Item(kind='word', addr=loc, text=clean(mw.group(1))))
            loc += 1
            continue
        # Instruction
        items.append(Item(kind='instr', addr=loc, text=line))
        loc += 1

    # Pass 2: resolve and emit
    # Determine ROM size
    max_addr = 0
    for it in items:
        if it.addr > max_addr:
            max_addr = it.addr
    rom: List[int] = [0] * (max_addr + 1 if items else 0)

    unresolved_refs: List[Tuple[int, str]] = []  # (index in rom, symbol)

    for it in items:
        if it.kind == 'word':
            token = it.text
            val = 0
            if number_re.match(token):
                val = parse_number(token)
            else:
                if token in symbols:
                    val = symbols[token]
                else:
                    unresolved_refs.append((it.addr, token))
                    val = 0
            rom[it.addr] = val & 0xFFFFFFFF
            continue

        # Parse instruction
        text = it.text
        # Split mnemonic and operands
        parts = [p.strip() for p in text.split(None, 1)]
        if not parts:
            continue
        mnemonic = parts[0].upper()
        operands = parts[1] if len(parts) > 1 else ''

        if mnemonic not in OPCODES:
            raise AsmError(f"Unknown instruction: {mnemonic}")
        op = OPCODES[mnemonic]

        reg = 0
        am = 0
        operand = 0

        # Define formats
        def split_operands(ops: str) -> List[str]:
            # split by comma, but ignore commas in brackets
            out = []
            cur = ''
            depth = 0
            for ch in ops:
                if ch == '[':
                    depth += 1
                elif ch == ']':
                    depth = max(0, depth - 1)
                if ch == ',' and depth == 0:
                    out.append(cur.strip())
                    cur = ''
                else:
                    cur += ch
            if cur.strip():
                out.append(cur.strip())
            return out

        ops = [o for o in split_operands(operands) if o]

        def parse_reg(tok: str) -> int:
            t = tok.upper()
            if t not in REGS:
                raise AsmError(f"Unknown register: {tok}")
            return REGS[t]

        if mnemonic in ('ST', 'LD', 'ADD', 'SUB', 'MUL', 'DIV', 'MOD', 'AND', 'OR', 'XOR'):
            if len(ops) != 2:
                raise AsmError(f"{mnemonic} expects: REG, operand")
            reg = parse_reg(ops[0])
            am, operand, unresolved = parse_operand(ops[1], symbols)
            for sym in unresolved:
                unresolved_refs.append((it.addr, sym))
        elif mnemonic == 'LDI':
            if len(ops) != 2:
                raise AsmError("LDI expects: REG, #imm")
            reg = parse_reg(ops[0])
            am, operand, unresolved = parse_operand(ops[1], symbols)
            if am != ADDR_MODES['IMM']:
                raise AsmError("LDI requires immediate operand prefixed with #")
            for sym in unresolved:
                unresolved_refs.append((it.addr, sym))
        elif mnemonic in ('NOT', 'CLR', 'DEC', 'INC'):
            if len(ops) != 1:
                raise AsmError(f"{mnemonic} expects: REG")
            reg = parse_reg(ops[0])
            am, operand = ADDR_MODES['IMM'], 0
        elif mnemonic in ('JMP', 'JZ', 'JN'):
            if len(ops) != 1:
                raise AsmError(f"{mnemonic} expects: operand")
            reg = 0  # ignored
            am, operand, unresolved = parse_operand(ops[0], symbols)
            for sym in unresolved:
                unresolved_refs.append((it.addr, sym))
        elif mnemonic in ('NOP', 'HALT'):
            if len(ops) != 0:
                raise AsmError(f"{mnemonic} takes no operands")
            reg = 0
            am, operand = ADDR_MODES['IMM'], 0
        else:
            raise AsmError(f"Unhandled mnemonic: {mnemonic}")

        rom[it.addr] = encode(op, reg, am, operand)

    # Resolve unresolved symbol refs for WORD or operand values
    if unresolved_refs:
        # If any unresolved remain, error with list
        missing = sorted(set(sym for _, sym in unresolved_refs))
        raise AsmError("Unresolved symbols: " + ", ".join(missing))

    return rom, symbols


def write_rom(path: str, rom: List[int]) -> None:
    # Write as native 32-bit little-endian values
    with open(path, 'wb') as f:
        for word in rom:
            f.write(struct.pack('<I', word & 0xFFFFFFFF))


def main(argv: List[str]) -> int:
    ap = argparse.ArgumentParser(description='ASOC-V assembler (.asoc -> rom.bin)')
    ap.add_argument('input', help='Input .asoc file')
    ap.add_argument('-o', '--output', default='rom.bin', help='Output ROM binary (default: rom.bin)')
    args = ap.parse_args(argv)

    try:
        with open(args.input, 'r', encoding='utf-8') as fh:
            lines = fh.readlines()
        rom, symbols = assemble(lines)
        write_rom(args.output, rom)
        print(f"Assembled {args.input} -> {args.output} ({len(rom)} words)")
        # Optional: list symbols
        if symbols:
            print('Symbols:')
            for k in sorted(symbols.keys()):
                print(f"  {k}: 0x{symbols[k]:04X}")
        return 0
    except AsmError as e:
        print(f"Assembly error: {e}", file=sys.stderr)
        return 2
    except FileNotFoundError as e:
        print(str(e), file=sys.stderr)
        return 1

if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
