---
layout: default
title: Fetch-Decode-Execute
permalink: /fetch-decode-execute/
---

### Problem Context
A binary compiled for one instruction set needs to be executed in a machine with a different instruction set.

### Minimal Solution
A process that maintains a state corresponding to the architecture of the machine for which the binary was compiled for,
and handlers for each opcode in the original instruction set. The instructions in the binary are fetched, decoded and executed.

In a loop,

![img.png](img.png)

### Implementation

For MOS 6502,
```
function cpu_step(cpu_state):
    pc_before ← cpu_state.program_counter
    opcode    ← memory[cpu_state.program_counter]
    cpu_state.program_counter ← cpu_state.program_counter + 1

    if opcode is JMP_ABSOLUTE:
        lo     ← memory[cpu_state.program_counter]
        hi     ← memory[cpu_state.program_counter + 1]
        target ← (hi << 8) | lo
        if target == pc_before:
            halt

    execute(opcode, cpu_state)
```

This follows the pattern of,
- fetching the instruction opcode from memory using the program counter
- decoding is not necessary here as MOS 6502 has granular opcode pattern where the opcodes for each combination of [operation, mode] is different
- executing the instruction 

The execution is done by maintaining a table of opcode handlers, with the handler mapped to an opcode occupying the corresponding index in the table.
```
function execute(opcode, cpu_state):
    opcode_handler_table[opcode](cpu_state)
```