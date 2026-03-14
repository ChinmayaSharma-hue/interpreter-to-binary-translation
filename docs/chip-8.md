---
layout: default
title: writing a CHIP-8 interpreter
permalink: /chip-8/
---

# Writing a CHIP-8 interpreter

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

Since this part was just to get familiar with reading unfamiliar instruction sets and implement them in the simplest way possible, this implementation was just enough.
