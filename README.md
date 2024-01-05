# Fuzzer

This repository contains a collection of simple depth-limited grammar fuzzers implemented in C. The repository contains two classes of fuzzers:

1. **Generic fuzzers:** fuzzers directly implemented in C, that take a defined grammar structure as input
2. **Compiled fuzzers:** python scripts that generate grammar-specific C fuzzer code, given a JSON grammar

 ## Common Features 

1. Control over the maximum expansion depth.
2. Seeded output parity, ie. produce the same output for a given seed.

## Compilation

A makefile is profided in each class's directory, which builds to a `build/` directory in the class subdirectory. General compilation is:

```bash
make [iterative|recursive] grammar=tinyc
```

Intermediate source files (prefixed with `_`) are removed after each build, comment out `rm build/_*` in the makefile to preserve. See makefile contents for more detail. 

Grammars are sourced from the `grammars/` directory in the base directory. Grammars are expected to be provided in the ANTLR `.g4` format. Examples can be found at https://github.com/antlr/grammars-v4/.

## Usage

All generated executables take a required seed argument and an optional max depth (default=10) argument:

```bash
gi_fuzz-tinyc 7 128
```

## Credits

The `tojson.py` and `ebnftosimple.py` utilities, and the `compute_cost()` method (and all its dependencies) in `gramutils.py` are the work of https://github.com/vrthra/.
