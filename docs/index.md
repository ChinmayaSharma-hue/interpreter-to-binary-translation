---
layout: default
title: Home
permalink: /
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

## Sections

- [writing a tiny interpreter for a simple self-designed ISA]({{ '/custom-isa/' | relative_url }})
- [writing a CHIP-8 interpreter]({{ '/chip-8/' | relative_url }})
