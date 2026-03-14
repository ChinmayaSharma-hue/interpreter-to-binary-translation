---
layout: default
title: Home
---

# Going from a simple interpreter to a dynamic binary translator

I was interested in understanding how QEMU and other emulators work, so I spent a lot of time trying to read and understand
dynamic binary translators. This ended up being futile as a lot of these projects were very mature and hard to get into without understanding
the basic tenets of dynamic binary translation.

So I decided to learn and implement the very basic components of guest binary execution. I chose a very simple architecture for this,
the MOS 6502, which has a handful of instructions, and the decoding is also quite easy as each operation + addressing mode is given its
own opcode.

This was the plan,
1. writing a tiny interpreter for a simple self-designed ISA
2. writing a CHIP-8 interpreter
3. writing a dynamic binary translator for MOS 6502, 
    1. implementing a basic fetch-decode-execute interpreter
    2. implementing a direct threaded implementation with precoding
    3. benchmark the interpreter against the optimized versions of binary exectution, find out what exactly gives the performance boost
    4. implementing a binary translator, directly from MOS 6502 to x86
    5. implement translation blocks with caching
    6. invent a tiny IR and implement translation from IR to host
    7. benchmark the IR exection against direct translation

## Writing a tiny interpreter for a simple self-designed ISA
A very trashy ISA that is sufficient to understand interpreter semantics.
1. Add,
    1. `ADD r[x], r[y], r[z]`, where the sum of `r[y]` and `r[z]` are stored in `r[x]`
    2. `ADD r[x], y, z`, where the sum of immediate values `y` and `z` are stored in `r[x]`
2. Jump,
    1. `JMP r[x]`, where the instruction pointer register is set to the value stored in `r[x]`
    2. `JMPCOND r[x]`, where the instruction pointer register is set to the value stored in `r[x]` if the flag register is set
3. Compare,
    1. `CMPGREATER r[x], r[y]`, which sets the flag register if `r[x]` is greater than `r[y]`
    2. `CMPEQ r[x], r[y]`, which sets the flag register if `r[x]` is equal to `r[y]`
4. Load/Store,
    1. `LD r[x], [y]` where the 32-bit value from `[y]` gets stored in the register `r[x]`,
    2. `STR [y], r[x]`, where the 32-bit value from `r[x]` gets stored in the memory address `[y]`.

The encoding,
1. `ADD`
    1. Register-Register Mode,
        1. opcode, `00`
        2. mode, `0`
        3. operands, three 3-bit register addressing values (starting from `000` to `111`)
    2. Register-Immediate Mode,
        1. opcode, `00`
        2. mode, `1`
        3. operands, two 3-bit register addressing values, and one 7-bit operand
2. `JMP`,
    1. Unconditional Jump,
        1. opcode, `01`
        2. mode, `0`
        3. operands, one 3-bit register addressing value
    2. Conditional Jump,
        1. opcode, `01`
        2. mode, `1`
        3. operands, one 3-bit register addressing value
3. `CMP`,
    1. Greater than compare,
        1. opcode, `10`
        2. mode, `0`
        3. operands, two 3-bit register addressing values
    2. Equal to compare,
        1. opcode: `10`
        2. mode: `1`
        3. operands, two 3-bit register addressing values
4. `LD/STR`
    1. Load,
        1. opcode, `11`
        2. mode, `0`
        3. operands, two 3-bit register addressing values
    2. Store,
        1. opcode, `11`
        2. mode, `1`
        3. operands, two 3-bit register addressing values

### Writing a CHIP-8 interpreter
 Going from the custom ISA to CHIP-8 is not a big deal, all I have to do now is to understand the instruction set and implement it in the same way.
 So I didn't do anything flashy,
 ```c 
 typedef struct {
    uint8_t registers[16];
    uint16_t stack[16];
    uint8_t memory[4096];
    uint8_t display[32][64];
    uint8_t keypad[16];
    uint8_t delay_timer;
    uint8_t sound_timer;
    uint16_t index_register;
    uint16_t program_counter;
    uint8_t stack_pointer;
} chip8_t;
 ```
and the execution,
```c 
while (true) {
    const uint16_t address = chip.program_counter;
    const uint16_t instruction = chip.memory[address] << 8 | chip.memory[address + 1];
    chip.program_counter += 2;
    execute(instruction, &chip);

    if (timer_tick_60hz()) {
        if (chip.delay_timer > 0) chip.delay_timer--;
        if (chip.sound_timer > 0) chip.sound_timer--;
    }
}
```

the structure of the `execute` function,
```c
void execute(const uint16_t instruction, chip8_t *state) {
    switch (instruction & 0xF000) {
        case <opcode>: {
            // opcode implementation
            break;
        }
        default:
            printf("Unknown instruction\n");
    }
}
```

Since this part was just to get familiar with reading unfamiliar instruction sets and implement them in the simplest way possible,
this implementation was just enough.
