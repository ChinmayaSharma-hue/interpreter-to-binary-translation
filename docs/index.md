---
layout: default
title: Home
permalink: /
---

# Dynamic Binary Translation

### Introduction

This is a series of posts where I try to understand the components of dynamic binary translation, by going from a simple interpreter, and 
coming across performance and correctness issues and finding the minimal solutions for these. The hope is that eventually we get to a state that
is very similar to the implementations of dynamic binary translators.

The following topics will be addressed,
- [Fetch-Decode-Execute]({{ '/fetch-decode-execute/' | relative_url }})
- [Direct Threading]({{ '/direct-threading/' | relative_url }})

### Terminology

#### Guest and Host Architectures

The guest architecture is the instruction set architecture of the binary which is to be executed.
The host architecture is the architecture of the underlying machine on which the binary needs to run on.

Emulation is the execution of a binary compiled for a guest architecture on a host architecture when the two are different.

#### Instruction
A binary encoding (series of bytes) that represents an operation in the guest ISA.

Each instruction contains,
- Opcode, that denotes the operation
- Mode, that denotes a specific mode of operation, e.g., addressing mode which decides how the operand is used to retrieve data from the memory
- Operand, the data to operate on in the instruction

#### Instruction Handler
A piece of host code that implements the semantics of a single guest instruction.

An instruction handler does the following.
- Reads the mode of operation and the operands
- Performs state transition on the machine state based on the operation, mode and its operands

#### Machine State
The complete state of the emulated guest machine.

This will include,
- Registers
- Program Counter
- Flags
- Memory

#### Precoding
An intermediate decoded version of the instruction that helps in an optimized execution of the emulation.
The process of precoding typically involves reading the raw instruction bytes and populating a pre-defined structure.