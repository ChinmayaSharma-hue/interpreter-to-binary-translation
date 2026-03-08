import json
import sys
from itertools import zip_longest

CONTEXT = 20

# =========================
# Addressing Modes
# =========================

IMPLIED = "Implied"
ACCUMULATOR = "Accumulator"
IMMEDIATE = "Immediate"
ZERO_PAGE = "Zero Page"
ZERO_PAGE_X = "Zero Page,X"
ZERO_PAGE_Y = "Zero Page,Y"
ABSOLUTE = "Absolute"
ABSOLUTE_X = "Absolute,X"
ABSOLUTE_Y = "Absolute,Y"
INDIRECT = "Indirect"
INDEXED_INDIRECT = "(Indirect,X)"
INDIRECT_INDEXED = "(Indirect),Y"
RELATIVE = "Relative"

# =========================
# Opcode Table
# OPCODE → (Mnemonic, Mode, Description)
# =========================

OPCODE_TABLE = {

    # --- System / Control ---
    "00": ("BRK", IMPLIED, "Force Interrupt"),
    "EA": ("NOP", IMPLIED, "No Operation"),
    "40": ("RTI", IMPLIED, "Return from Interrupt"),
    "60": ("RTS", IMPLIED, "Return from Subroutine"),
    "20": ("JSR", ABSOLUTE, "Jump to Subroutine"),
    "4C": ("JMP", ABSOLUTE, "Jump Absolute"),
    "6C": ("JMP", INDIRECT, "Jump Indirect"),

    # --- Loads ---
    "A9": ("LDA", IMMEDIATE, "Load Accumulator"),
    "A5": ("LDA", ZERO_PAGE, "Load Accumulator"),
    "B5": ("LDA", ZERO_PAGE_X, "Load Accumulator"),
    "AD": ("LDA", ABSOLUTE, "Load Accumulator"),
    "BD": ("LDA", ABSOLUTE_X, "Load Accumulator"),
    "B9": ("LDA", ABSOLUTE_Y, "Load Accumulator"),
    "A1": ("LDA", INDEXED_INDIRECT, "Load Accumulator"),
    "B1": ("LDA", INDIRECT_INDEXED, "Load Accumulator"),

    "A2": ("LDX", IMMEDIATE, "Load X Register"),
    "A6": ("LDX", ZERO_PAGE, "Load X Register"),
    "B6": ("LDX", ZERO_PAGE_Y, "Load X Register"),
    "AE": ("LDX", ABSOLUTE, "Load X Register"),
    "BE": ("LDX", ABSOLUTE_Y, "Load X Register"),

    "A0": ("LDY", IMMEDIATE, "Load Y Register"),
    "A4": ("LDY", ZERO_PAGE, "Load Y Register"),
    "B4": ("LDY", ZERO_PAGE_X, "Load Y Register"),
    "AC": ("LDY", ABSOLUTE, "Load Y Register"),
    "BC": ("LDY", ABSOLUTE_X, "Load Y Register"),

    # --- Stores ---
    "8D": ("STA", ABSOLUTE, "Store Accumulator"),
    "85": ("STA", ZERO_PAGE, "Store Accumulator"),
    "95": ("STA", ZERO_PAGE_X, "Store Accumulator"),
    "9D": ("STA", ABSOLUTE_X, "Store Accumulator"),
    "99": ("STA", ABSOLUTE_Y, "Store Accumulator"),
    "81": ("STA", INDEXED_INDIRECT, "Store Accumulator"),
    "91": ("STA", INDIRECT_INDEXED, "Store Accumulator"),

    "8E": ("STX", ABSOLUTE, "Store X Register"),
    "86": ("STX", ZERO_PAGE, "Store X Register"),
    "96": ("STX", ZERO_PAGE_Y, "Store X Register"),

    "8C": ("STY", ABSOLUTE, "Store Y Register"),
    "84": ("STY", ZERO_PAGE, "Store Y Register"),
    "94": ("STY", ZERO_PAGE_X, "Store Y Register"),

    # --- Transfers ---
    "AA": ("TAX", IMPLIED, "Transfer Accumulator → X"),
    "A8": ("TAY", IMPLIED, "Transfer Accumulator → Y"),
    "8A": ("TXA", IMPLIED, "Transfer X → Accumulator"),
    "98": ("TYA", IMPLIED, "Transfer Y → Accumulator"),
    "BA": ("TSX", IMPLIED, "Transfer Stack Pointer → X"),
    "9A": ("TXS", IMPLIED, "Transfer X → Stack Pointer"),

    # --- Stack ---
    "48": ("PHA", IMPLIED, "Push Accumulator"),
    "68": ("PLA", IMPLIED, "Pull Accumulator"),
    "08": ("PHP", IMPLIED, "Push Processor Status"),
    "28": ("PLP", IMPLIED, "Pull Processor Status"),

    # --- Arithmetic ---
    "69": ("ADC", IMMEDIATE, "Add with Carry"),
    "65": ("ADC", ZERO_PAGE, "Add with Carry"),
    "75": ("ADC", ZERO_PAGE_X, "Add with Carry"),
    "6D": ("ADC", ABSOLUTE, "Add with Carry"),
    "7D": ("ADC", ABSOLUTE_X, "Add with Carry"),
    "79": ("ADC", ABSOLUTE_Y, "Add with Carry"),
    "61": ("ADC", INDEXED_INDIRECT, "Add with Carry"),
    "71": ("ADC", INDIRECT_INDEXED, "Add with Carry"),

    "E9": ("SBC", IMMEDIATE, "Subtract with Carry"),
    "E5": ("SBC", ZERO_PAGE, "Subtract with Carry"),
    "F5": ("SBC", ZERO_PAGE_X, "Subtract with Carry"),
    "ED": ("SBC", ABSOLUTE, "Subtract with Carry"),
    "FD": ("SBC", ABSOLUTE_X, "Subtract with Carry"),
    "F9": ("SBC", ABSOLUTE_Y, "Subtract with Carry"),
    "E1": ("SBC", INDEXED_INDIRECT, "Subtract with Carry"),
    "F1": ("SBC", INDIRECT_INDEXED, "Subtract with Carry"),

    # --- Comparisons ---
    "C9": ("CMP", IMMEDIATE, "Compare Accumulator"),
    "E0": ("CPX", IMMEDIATE, "Compare X Register"),
    "C0": ("CPY", IMMEDIATE, "Compare Y Register"),

    # --- Bitwise ---
    "29": ("AND", IMMEDIATE, "Logical AND"),
    "49": ("EOR", IMMEDIATE, "Exclusive OR"),
    "09": ("ORA", IMMEDIATE, "Logical OR"),

    "24": ("BIT", ZERO_PAGE, "Bit Test"),
    "2C": ("BIT", ABSOLUTE, "Bit Test"),

    # --- Shifts / Rotates ---
    "0A": ("ASL", ACCUMULATOR, "Arithmetic Shift Left"),
    "06": ("ASL", ZERO_PAGE, "Arithmetic Shift Left"),
    "0E": ("ASL", ABSOLUTE, "Arithmetic Shift Left"),

    "4A": ("LSR", ACCUMULATOR, "Logical Shift Right"),
    "46": ("LSR", ZERO_PAGE, "Logical Shift Right"),

    "2A": ("ROL", ACCUMULATOR, "Rotate Left"),
    "6A": ("ROR", ACCUMULATOR, "Rotate Right"),

    # --- INC / DEC ---
    "E6": ("INC", ZERO_PAGE, "Increment Memory"),
    "EE": ("INC", ABSOLUTE, "Increment Memory"),

    "C6": ("DEC", ZERO_PAGE, "Decrement Memory"),
    "CE": ("DEC", ABSOLUTE, "Decrement Memory"),

    # --- Register INC / DEC ---
    "E8": ("INX", IMPLIED, "Increment X Register"),
    "C8": ("INY", IMPLIED, "Increment Y Register"),
    "CA": ("DEX", IMPLIED, "Decrement X Register"),
    "88": ("DEY", IMPLIED, "Decrement Y Register"),

    # --- Branches ---
    "10": ("BPL", RELATIVE, "Branch if Positive (N=0)"),
    "30": ("BMI", RELATIVE, "Branch if Minus (N=1)"),
    "50": ("BVC", RELATIVE, "Branch if Overflow Clear"),
    "70": ("BVS", RELATIVE, "Branch if Overflow Set"),
    "90": ("BCC", RELATIVE, "Branch if Carry Clear"),
    "B0": ("BCS", RELATIVE, "Branch if Carry Set"),
    "D0": ("BNE", RELATIVE, "Branch if Not Equal"),
    "F0": ("BEQ", RELATIVE, "Branch if Equal"),

    # --- Flags ---
    "18": ("CLC", IMPLIED, "Clear Carry Flag"),
    "38": ("SEC", IMPLIED, "Set Carry Flag"),
    "58": ("CLI", IMPLIED, "Clear Interrupt Disable"),
    "78": ("SEI", IMPLIED, "Set Interrupt Disable"),
    "B8": ("CLV", IMPLIED, "Clear Overflow Flag"),
    "D8": ("CLD", IMPLIED, "Clear Decimal Mode"),
    "F8": ("SED", IMPLIED, "Set Decimal Mode"),
}

# =========================
# Decoder
# =========================

def decode_opcode(opcode):
    opcode = opcode.upper()
    if opcode in OPCODE_TABLE:
        mnemonic, mode, description = OPCODE_TABLE[opcode]
        return {
            "mnemonic": mnemonic,
            "addressing_mode": mode,
            "operation_description": description
        }
    return {
        "mnemonic": "UNKNOWN",
        "addressing_mode": "Unknown",
        "operation_description": "Unmapped Opcode"
    }


def _read_u8(program_image, address):
    if program_image is None:
        return None
    if address < 0 or address >= len(program_image):
        return None
    return program_image[address]


def decode_operand(addressing_mode, program_counter, program_image):
    if program_counter is None or program_image is None:
        return None

    op1 = _read_u8(program_image, (program_counter + 1) & 0xFFFF)
    op2 = _read_u8(program_image, (program_counter + 2) & 0xFFFF)

    if addressing_mode == IMPLIED:
        return None
    if addressing_mode == ACCUMULATOR:
        return "A"
    if op1 is None:
        return "Unavailable"
    if addressing_mode == IMMEDIATE:
        return f"#$%02X" % op1
    if addressing_mode == ZERO_PAGE:
        return f"$%02X" % op1
    if addressing_mode == ZERO_PAGE_X:
        return f"$%02X,X" % op1
    if addressing_mode == ZERO_PAGE_Y:
        return f"$%02X,Y" % op1
    if addressing_mode == INDEXED_INDIRECT:
        return f"($%02X,X)" % op1
    if addressing_mode == INDIRECT_INDEXED:
        return f"($%02X),Y" % op1
    if addressing_mode == RELATIVE:
        rel = op1 if op1 < 0x80 else op1 - 0x100
        target = (program_counter + 2 + rel) & 0xFFFF
        return f"$%02X -> $%04X" % (op1, target)

    if op2 is None:
        return "Unavailable"
    value = op1 | (op2 << 8)
    if addressing_mode == ABSOLUTE:
        return f"$%04X" % value
    if addressing_mode == ABSOLUTE_X:
        return f"$%04X,X" % value
    if addressing_mode == ABSOLUTE_Y:
        return f"$%04X,Y" % value
    if addressing_mode == INDIRECT:
        return f"($%04X)" % value

    return None

# =========================
# Trace Parsing
# =========================

def parse_line(line, program_image=None):
    parts = line.strip().split()

    if len(parts) < 7:
        return {
            "raw_trace": line.strip(),
            "error": "Malformed trace line"
        }

    opcode = parts[6]
    program_counter = int(parts[0], 16)
    decoded = decode_opcode(opcode)
    decoded["operand"] = decode_operand(decoded["addressing_mode"], program_counter, program_image)

    return {
        "program_counter": parts[0],
        "accumulator": parts[1],
        "x_register": parts[2],
        "y_register": parts[3],
        "stack_pointer": parts[4],
        "status_register": parts[5],
        "opcode": opcode,
        "decoded_instruction": decoded,
        "raw_trace": line.strip()
    }

def load_file(path):
    with open(path, "r") as f:
        return f.readlines()


def load_program_image(path):
    with open(path, "rb") as f:
        return f.read()

# =========================
# Diff Engine
# =========================

def main(mine_path, ref_path, program_path):
    mine_lines = load_file(mine_path)
    ref_lines = load_file(ref_path)
    program_image = load_program_image(program_path)

    divergence = None

    for idx, (mine, ref) in enumerate(zip_longest(mine_lines, ref_lines)):
        if mine != ref:
            divergence = idx
            break

    if divergence is None:
        print(json.dumps({"status": "IDENTICAL_EXECUTION"}, indent=4))
        return

    start = max(0, divergence - CONTEXT)
    end = divergence + CONTEXT + 1

    result = {
        "status": "DIVERGENCE_DETECTED",

        "divergence_visible_at": {
            "line_number": divergence + 1,
            "mine": parse_line(mine_lines[divergence], program_image),
            "reference": parse_line(ref_lines[divergence], program_image)
        },

        "root_cause_instruction": None,

        "execution_context": []
    }

    if divergence > 0:
        result["root_cause_instruction"] = {
            "line_number": divergence,
            "mine": parse_line(mine_lines[divergence - 1], program_image),
            "reference": parse_line(ref_lines[divergence - 1], program_image),
            "note": "Instruction causing incorrect state transition"
        }

    for i in range(start, min(end, len(mine_lines), len(ref_lines))):
        result["execution_context"].append({
            "line_number": i + 1,
            "mine": parse_line(mine_lines[i], program_image),
            "reference": parse_line(ref_lines[i], program_image),
            "states_match": mine_lines[i] == ref_lines[i]
        })

    print(json.dumps(result, indent=4))

# =========================
# Entry Point
# =========================

if __name__ == "__main__":
    default_program_path = "/home/chinmay/Work/codebases/mos6502/tests/functional/6502_functional_test.bin"
    if len(sys.argv) not in (3, 4):
        print("Usage: python diff.py <mine.txt> <py65.txt> [program.bin]")
        sys.exit(1)

    program_path = sys.argv[3] if len(sys.argv) == 4 else default_program_path
    main(sys.argv[1], sys.argv[2], program_path)
