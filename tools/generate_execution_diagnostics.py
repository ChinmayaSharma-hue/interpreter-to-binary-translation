#!/usr/bin/env python3

import argparse
import json
import logging
import os
import struct
import subprocess
import sys
import time
from pathlib import Path

from py65.devices.mpu6502 import MPU

CONTEXT = 20
EMULATION_MODES = ["BLOCK"]
LOGGER = logging.getLogger("diagnostics")

ANSI_RESET = "\033[0m"
ANSI_DIM = "\033[2m"
ANSI_CYAN = "\033[36m"
ANSI_YELLOW = "\033[33m"
ANSI_RED = "\033[31m"
ANSI_BOLD_RED = "\033[1;31m"

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


class ColorLogFormatter(logging.Formatter):
    LEVEL_COLORS = {
        logging.DEBUG: ANSI_CYAN,
        logging.INFO: ANSI_CYAN,
        logging.WARNING: ANSI_YELLOW,
        logging.ERROR: ANSI_RED,
        logging.CRITICAL: ANSI_BOLD_RED,
    }

    def __init__(self, use_color):
        super().__init__(datefmt="%Y-%m-%d %H:%M:%S")
        self.use_color = use_color

    def format(self, record):
        timestamp = f"{self.formatTime(record)},{int(record.msecs):03d}"
        level_name = record.levelname
        message = record.getMessage()

        if self.use_color:
            level_color = self.LEVEL_COLORS.get(record.levelno, ANSI_CYAN)
            prefix = (
                f"{ANSI_DIM}{timestamp}{ANSI_RESET} "
                f"{ANSI_DIM}|{ANSI_RESET} "
                f"{level_color}{level_name}{ANSI_RESET} "
                f"{ANSI_DIM}|{ANSI_RESET}"
            )
        else:
            prefix = f"{timestamp} | {level_name} |"

        formatted = f"{prefix} {message}"
        if record.exc_info:
            formatted = f"{formatted}\n{self.formatException(record.exc_info)}"
        return formatted


def configure_logging(log_level):
    handler = logging.StreamHandler(sys.stderr)
    handler.setFormatter(ColorLogFormatter(use_color=sys.stderr.isatty()))

    root_logger = logging.getLogger()
    root_logger.handlers.clear()
    root_logger.setLevel(log_level)
    root_logger.addHandler(handler)


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

TRACE_RECORD_FORMAT = ">HBBBBBB"  # PC (uint16), A, X, Y, SP, SR, opcode
TRACE_RECORD_SIZE = struct.calcsize(TRACE_RECORD_FORMAT)


def write_reference_trace_binary(path, trace_lines):
    with open(path, "wb") as f:
        for line in trace_lines:
            parts = line.strip().split()
            f.write(struct.pack(TRACE_RECORD_FORMAT,
                int(parts[0], 16), int(parts[1], 16), int(parts[2], 16),
                int(parts[3], 16), int(parts[4], 16), int(parts[5], 16),
                int(parts[6], 16)))


def read_reference_trace_binary(path):
    with open(path, "rb") as f:
        data = f.read()
    return [
        "%04X %02X %02X %02X %02X %02X %02X\n" % struct.unpack_from(TRACE_RECORD_FORMAT, data, i)
        for i in range(0, len(data), TRACE_RECORD_SIZE)
    ]


def load_program_image(path):
    with open(path, "rb") as f:
        return f.read()


def build_performance_report(elapsed_seconds, instruction_count=None):
    instructions_per_second = None
    if elapsed_seconds > 0:
        if instruction_count is not None:
            instructions_per_second = instruction_count / elapsed_seconds

    return {
        "elapsed_seconds": elapsed_seconds,
        "elapsed_milliseconds": elapsed_seconds * 1000,
        "instruction_count": instruction_count,
        "instructions_per_second": instructions_per_second,
    }

def run_local_implementation(binary_path, emulation_mode, trace=False, instruction_count=None):
    LOGGER.info("Starting local run for mode=%s trace=%s", emulation_mode, trace)
    env = dict(os.environ)
    env["EMULATION_MODE"] = emulation_mode
    if trace:
        env["EMULATION_TRACE"] = "1"
    else:
        env.pop("EMULATION_TRACE", None)
    start = time.perf_counter()
    completed = subprocess.run(
        [str(binary_path)],
        check=False,
        capture_output=True,
        text=True,
        env=env,
    )
    elapsed_seconds = time.perf_counter() - start
    if completed.returncode != 0:
        LOGGER.error(
            "Local run failed for mode=%s (exit_code=%d, elapsed_ms=%.3f)",
            emulation_mode,
            completed.returncode,
            elapsed_seconds * 1000,
        )
        raise RuntimeError(
            "Local implementation failed "
            f"(mode={emulation_mode}, exit_code={completed.returncode}):\n{completed.stderr.strip()}"
        )
    trace_lines = completed.stdout.splitlines(keepends=True)
    performance = build_performance_report(
        elapsed_seconds,
        instruction_count=instruction_count,
    )
    LOGGER.info(
        "Completed local run for mode=%s trace=%s (trace_lines=%d, elapsed_ms=%.3f)",
        emulation_mode,
        trace,
        len(trace_lines),
        performance["elapsed_milliseconds"],
    )
    return trace_lines, performance


def generate_py65_reference_trace(program_path):
    LOGGER.info("Generating py65 reference trace from program=%s", program_path)
    start = time.perf_counter()
    mpu = MPU()

    with open(program_path, "rb") as f:
        data = f.read()

    for i, byte in enumerate(data):
        mpu.memory[i] = byte

    mpu.pc = 0x0400

    lines = []

    while True:
        pc = mpu.pc
        opcode = mpu.memory[pc]
        lines.append(f"{pc:04X} {mpu.a:02X} {mpu.x:02X} {mpu.y:02X} {mpu.sp:02X} {mpu.p:02X} {opcode:02X}\n")

        if opcode == 0x4C:
            lo = mpu.memory[pc + 1]
            hi = mpu.memory[pc + 2]
            target = (hi << 8) | lo
            if target == pc:
                break

        mpu.step()

    elapsed_seconds = time.perf_counter() - start
    performance = build_performance_report(
        elapsed_seconds,
        instruction_count=len(lines),
    )
    LOGGER.info(
        "Completed py65 reference generation (trace_lines=%d, elapsed_ms=%.3f)",
        len(lines),
        performance["elapsed_milliseconds"],
    )
    return lines, performance


def benchmark_py65_reference(program_path):
    LOGGER.info("Benchmarking py65 reference without trace from program=%s", program_path)
    start = time.perf_counter()
    mpu = MPU()

    with open(program_path, "rb") as f:
        data = f.read()

    for i, byte in enumerate(data):
        mpu.memory[i] = byte

    mpu.pc = 0x0400
    instruction_count = 0

    while True:
        pc = mpu.pc
        opcode = mpu.memory[pc]
        instruction_count += 1

        if opcode == 0x4C:
            lo = mpu.memory[pc + 1]
            hi = mpu.memory[pc + 2]
            target = (hi << 8) | lo
            if target == pc:
                break

        mpu.step()

    elapsed_seconds = time.perf_counter() - start
    performance = build_performance_report(
        elapsed_seconds,
        instruction_count=instruction_count,
    )
    LOGGER.info(
        "Completed py65 reference benchmark without trace (instructions=%d, elapsed_ms=%.3f)",
        instruction_count,
        performance["elapsed_milliseconds"],
    )
    return performance


def compare_execution_traces(local_trace_lines, reference_trace_lines, program_path):
    LOGGER.info(
        "Comparing traces (local_lines=%d, reference_lines=%d)",
        len(local_trace_lines),
        len(reference_trace_lines),
    )
    mine_lines = local_trace_lines
    ref_lines = reference_trace_lines
    program_image = load_program_image(program_path)

    divergence = None

    for idx, (mine, ref) in enumerate(zip(mine_lines, ref_lines)):
        if mine != ref:
            divergence = idx
            break

    if divergence is None:
        LOGGER.info("Comparison complete: IDENTICAL_EXECUTION")
        return {"status": "IDENTICAL_EXECUTION"}

    start = max(0, divergence - CONTEXT)
    end = divergence + CONTEXT + 1

    result = {
        "status": "DIVERGENCE_DETECTED",

        "divergence_visible_at": {
            "line_number": divergence + 1,
            "implementation_trace": parse_line(mine_lines[divergence], program_image),
            "reference": parse_line(ref_lines[divergence], program_image)
        },

        "root_cause_instruction": None,

        "execution_context": []
    }

    if divergence > 0:
        result["root_cause_instruction"] = {
            "line_number": divergence,
            "implementation_trace": parse_line(mine_lines[divergence - 1], program_image),
            "reference": parse_line(ref_lines[divergence - 1], program_image),
            "note": "Instruction causing incorrect state transition"
        }

    for i in range(start, min(end, len(mine_lines), len(ref_lines))):
        result["execution_context"].append({
            "line_number": i + 1,
            "implementation_trace": parse_line(mine_lines[i], program_image),
            "reference": parse_line(ref_lines[i], program_image),
            "states_match": mine_lines[i] == ref_lines[i]
        })

    LOGGER.info("Comparison complete: DIVERGENCE_DETECTED at line=%d", divergence + 1)
    return result


if __name__ == "__main__":
    script_dir = Path(__file__).resolve().parent
    project_root = script_dir.parent

    parser = argparse.ArgumentParser(
        description="Run local mos6502, generate py65 reference trace, compare, and emit JSON diagnostics."
    )
    parser.add_argument(
        "--binary",
        type=Path,
        default=project_root / "cmake-build-debug" / "mos6502",
        help="Path to mos6502 binary.",
    )
    parser.add_argument(
        "--program",
        type=Path,
        default=project_root / "tests" / "functional" / "6502_functional_test.bin",
        help="Path to the functional test binary.",
    )
    parser.add_argument(
        "--report",
        type=Path,
        default=script_dir / "execution_trace_diagnostic_report.json",
        help="Path to write diagnostic JSON report.",
    )
    parser.add_argument(
        "--reference-trace",
        type=Path,
        default=script_dir / "reference_trace.bin",
        help="Path to cache the py65 reference trace (binary format). If the file exists it will be reused.",
    )
    parser.add_argument(
        "--benchmark",
        action="store_true",
        default=False,
        help="Run performance benchmarks. Skipped by default.",
    )
    parser.add_argument(
        "--log-level",
        default="INFO",
        choices=["DEBUG", "INFO", "WARNING", "ERROR"],
        help="Runtime logging verbosity.",
    )
    args = parser.parse_args()

    configure_logging(getattr(logging, args.log_level))

    LOGGER.info("Diagnostics start")
    LOGGER.info("Binary path: %s", args.binary.resolve())
    LOGGER.info("Program path: %s", args.program.resolve())
    LOGGER.info("Report path: %s", args.report.resolve())
    LOGGER.info("Reference trace path: %s", args.reference_trace.resolve())
    LOGGER.info("Benchmarking: %s", args.benchmark)

    reference_trace_path = args.reference_trace.resolve()
    if reference_trace_path.exists() and reference_trace_path.stat().st_size >= TRACE_RECORD_SIZE:
        LOGGER.info("Loading cached reference trace from %s", reference_trace_path)
        reference_trace_lines = read_reference_trace_binary(reference_trace_path)
        LOGGER.info("Loaded %d reference trace lines", len(reference_trace_lines))
    else:
        reference_trace_lines, _ = generate_py65_reference_trace(args.program.resolve())
        reference_trace_path.parent.mkdir(parents=True, exist_ok=True)
        write_reference_trace_binary(reference_trace_path, reference_trace_lines)
        LOGGER.info("Saved reference trace to %s", reference_trace_path)

    reference_performance = benchmark_py65_reference(args.program.resolve()) if args.benchmark else None

    mode_reports = {}
    instruction_count = len(reference_trace_lines)
    for mode in EMULATION_MODES:
        LOGGER.info("Processing mode=%s", mode)
        try:
            local_trace_lines, _ = run_local_implementation(
                args.binary.resolve(),
                mode,
                trace=True,
                instruction_count=instruction_count,
            )
            local_performance = None
            if args.benchmark:
                _, local_performance = run_local_implementation(
                    args.binary.resolve(),
                    mode,
                    trace=False,
                    instruction_count=instruction_count,
                )
            comparison = compare_execution_traces(local_trace_lines, reference_trace_lines, args.program.resolve())
            mode_reports[mode] = {
                "status": comparison.get("status", "UNKNOWN"),
                "local_trace_line_count": len(local_trace_lines),
                "reference_trace_line_count": len(reference_trace_lines),
                "performance": local_performance,
                "comparison": comparison,
            }
        except Exception as exc:
            LOGGER.exception("Mode processing failed for mode=%s", mode)
            mode_reports[mode] = {
                "status": "EXECUTION_FAILED",
                "local_trace_line_count": 0,
                "reference_trace_line_count": len(reference_trace_lines),
                "performance": None,
                "error": str(exc),
            }

    all_modes_match_reference = all(
        mode_report.get("status") == "IDENTICAL_EXECUTION"
        for mode_report in mode_reports.values()
    )

    report = {
        "status": "ALL_MODES_IDENTICAL" if all_modes_match_reference else "MODE_DIVERGENCE_DETECTED",
        "emulation_modes_checked": list(EMULATION_MODES),
        "all_modes_match_reference": all_modes_match_reference,
        "reference_performance": reference_performance,
        "results_by_mode": mode_reports,
    }

    args.report.parent.mkdir(parents=True, exist_ok=True)
    with open(args.report, "w", encoding="utf-8") as f:
        json.dump(report, f, indent=4)

    LOGGER.info("Diagnostics complete (status=%s)", report["status"])
    print(f"Diagnostic report written: {args.report.resolve()}")
