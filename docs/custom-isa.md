---
layout: default
title: Custom ISA
permalink: /custom-isa/
---

# Writing a tiny interpreter for a simple self-designed ISA

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

[Back to Home]({{ '/' | relative_url }})
[writing a CHIP-8 interpreter]({{ '/chip-8/' | relative_url }})
