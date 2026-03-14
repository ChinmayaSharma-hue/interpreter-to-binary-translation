# mos6502-binary-translation

## Usage

Build:

```bash
cmake -S . -B cmake-build-debug
cmake --build cmake-build-debug --target mos6502
```

Run emulator (default mode: `FETCH_DECODE_EXECUTE`):

```bash
./cmake-build-debug/mos6502
```

Run with explicit mode:

```bash
./cmake-build-debug/mos6502 --mode DIRECT_THREADED
```

Run with a custom program binary:

```bash
./cmake-build-debug/mos6502 --program tests/functional/6502_functional_test.bin
```

Generate diagnostic report for both modes:

```bash
python3 tools/generate_execution_diagnostics.py
```
