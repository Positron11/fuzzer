# Fuzzer

This repository contains a collection of simple depth-limited grammar fuzzers implemented (at least, eventually) in C. For a brief overview of what fuzzing is and a development log up to (hopefully) the most recent updates, read the [devlog](https://positron11.github.io/fuzzer/#whats-fuzzing).

The repository contains two types of fuzzers:

1. **Generic fuzzers:** fuzzers directly implemented in C, that take a defined grammar structure as input
2. **Compiled fuzzers:** python scripts that generate grammar-specific C fuzzer code, given a JSON grammar

 ## Common Features 

Every fuzzer variant has the ability to control the maximum and minimum expansion depth.

Additionally, all fuzzers here are seeded, and achieve seeded output parity with each other, ie. produce the same output for a given seed, provided `max_depth = min_depth` (I'm working towards getting rid of this condition, it's actually rather trivial). 

## Compilation and Usage

### Generic Fuzzers

View the source for generic fuzzers in `generic/`. Directory contains two classes (**str**ing and **tok**en) and two subtypes (**iter**ative and **rec**u**r**sive). Compile to obtain fuzzer executables - the command line options passable to the executables are as follows:

1. `fuzzer <seed> <min and max depth>`
2. `fuzzer <seed> <min depth> <max depth>`
3. `fuzzer <seed> <min depth> <max depth> <runs>` 

### Grammar Compilation

For both token fuzzers, a grammar compilation utility is included in `generic/utilities/gcompiler.py`, which takes a JSON grammar as input and prints a compiled header file to `stdout`. Use like so:

```bash
cd generic/utilities/
python gcompiler.py path-to-json-grammar > path-to-header-file
```

To use this grammar, change the import statement near the beginning of the fuzzer source file (default: `\#include "grammars/arithmetic.h"`). A precompiled grammar has been included in `generic/utilities/grammars/`.

(It might've been more elegant to include the grammar with the `-include` flag but removing the import statement floods my IDE with red lines.)

### Compiled Fuzzers

View the source for compilers in `compiled/compilers`/ and generate C source and executables with `compiled/main.py` (due to the nature of python imports, it is imperative that `compiled/main.py` be run from within `compiled/`). The command line options passable to the executables are as follows:

1. `fuzzer <seed> <min and max depth>`
2. `fuzzer <seed> <min depth> <max depth>`

## Credits

The generic string fuzzer uses [uthash](https://troydhanson.github.io/uthash/) by Troy D. Hanson.
