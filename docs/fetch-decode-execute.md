---
layout: default
title: Fetch-Decode-Execute
permalink: /custom-isa/
---

### Problem Context
A binary compiled for one instruction set needs to be executed in a machine with a different instruction set.

### Minimal Solution
A process that maintains a state corresponding to the architecture of the machine for which the binary was compiled for,
and handlers for each opcode in the original instruction set. The instructions in the binary are fetched, decoded and executed.

![img.png](img.png)

### Implementation

