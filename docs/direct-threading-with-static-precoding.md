---
layout: default
title: Interpreter for CHIP-8
permalink: /direct-threading-with-static-precoding/
---

### Problem Context

The fetch-decode-execute implementation follows interpreter semantics, and the execution of one instruction in the 
guest architecture involves execution of tens of instructions in the host.

The branches in a central dispatch loop pattern,
- branch to interpreter routine
- register indirect branch to return from the interpreter routine
- branch that terminates the loop

The goal is to find a solution that removes these branches.

### Minimal Solution
The overhead can be recovered by reducing the number of branches. 
- **Removing the function call overhead**, the opcode handlers do not need to be implemented as functions, they can be implemented as labels with gotos within each label to jump between the labels without needing a central dispatch loop.
- **Jumping to the label address**, if the instruction handlers are implemented as labels, after executing the instruction, a branch to the next instruction handler needs to happen, the goto for that instruction must be computed
- **Computation of goto address**, the computation of the address to which a goto must occur in each instruction will involve finding out what the opcode of the next instruction is and then finding the mapping for that opcode to the label address
- **Removing the overhead of computing goto address at runtime**, to reduce the runtime overhead of finding the mapping from opcode to the corresponding label address, the set of instructions can be statically precoded into a structure and added to a code cache, from which the instructions can be executed by jumping to the corresponding label address that is already computed prior to execution

### Implementation

To remove the overhead of computing goto address at runtime, 

Translation in a loop,

![img_1.png](img_1.png)

Execution,

![img_2.png](img_2.png)

1. Precoding can be used to convert the raw instructions into the following struct,
    ```c
     threaded_instr:
        handler         
        mode            
        operand         
        spc_byte_offset 
    ```
    - The `handler` pointer points to the label address of the opcode implementation
    - The `mode` helps in switching between the modes within the same operation
    - The `operand` is retrieved directly from the precoding and not from the machine memory during execution
    - The `spc_byte_offet` is used for incrementing the source program counter to maintain the correct value for the source program counter
2. Caching the precoded instructions in a code cache,
   ```c
    cpu_state:
        accumulator
        index_x, index_y
        status_register
        stack_pointer
        program_counter
        memory[65536]
        code_cache[CODE_CACHE_CAPACITY]  
        translation_map[MEMORY_SIZE]    
        cache_length
    ```
    - The `code_cache` is used to store the threaded instructions
    - The translation map is used to store the mapping from source program counter to the corresponding index in the code cache which contains the precoded instruction
    - The `cache_length` is used to maintain a running index of the translated program counter
3. Static translation of instructions before execution to populate the code cache,
   ```c
   function translate(cpu_state, dispatch_table):
    spc ← cpu_state.program_counter   // source program counter
    tpc ← 0                           // target (cache) index

    while tpc < CODE_CACHE_CAPACITY:
        opcode ← memory[spc]

        if opcode is not valid:  break
        if spc + instruction_length[opcode] > MEMORY_SIZE:  break

        threaded_instr ←
            handler        : dispatch_table[opcode].label_address
            mode           : dispatch_table[opcode].addressing_mode
            operand        : bytes at memory[spc+1 .. spc+length]   
            spc_byte_offset: instruction_length[opcode]

        code_cache[tpc]        ← threaded_instr
        translation_map[spc]   ← &code_cache[tpc]   

        tpc ← tpc + 1
        spc ← spc + instruction_length[opcode]
   ```
   The translation is done until either one of the following is true,
   - The code cache is full
   - The opcode of the instruction being translated is not valid, therefore it is not an instruction
4. Execution of instructions in the code cache,
   ```c 
   function run(cpu_state):
    if code_cache is empty:
        translate(cpu_state, dispatch_table)

    if translation_map[cpu_state.program_counter] is NULL:
        error "no translation exists for this PC"
        halt

    instruction ← translation_map[cpu_state.program_counter]
    goto instruction->handler

    ---

    label LDA:
        // perform load accumulator ...
        cpu_state.program_counter ← cpu_state.program_counter + instruction->spc_byte_offset
        next ← translation_map[cpu_state.program_counter]
        goto next->handler         // ← no central loop, direct jump to next handler

    label JMP:
        // compute jump target ...
        cpu_state.program_counter ← target
        next ← translation_map[cpu_state.program_counter]
        goto next->handler

    // all other labels follow the same tail-jump pattern
   ```
   The static translation happens first, and then the first threaded instruction is obtained, with goto to the corresponding label address.