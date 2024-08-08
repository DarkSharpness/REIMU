# Extension

## Overview

This project is designed for better scalability and readability (hope so).

The simulator mainly consists of $4$ main parts:

- Config parser
- Frontend (Assembler)
- Midend (Linker)
- Backend (Interpreter)

### Config parser

The config parser determines the simulator's configuration, significantly impacting all other components.

### Major components

The frontend, midend, and backend are almost independent of each other. They communicate through the `layout` class, which serves as the abstract data holder for each preceding stage.

This structure aims to enhance modularity and maintainability, making it easier to extend and understand the codebase.

## Examples

### Extending with new pseudo instruction

TODO

### Extending with new instruction set

TODO
