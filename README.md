# Fuzzer

A collection of simple depth-limited grammar fuzzers implemented in C.

## Generic Fuzzers

**Description:** Generic fuzzer programs implemented in C, that take a defined grammar structure as input. 

View the source for generic fuzzers in `generic/`. Directory contains two classes (**str**ing and **tok**en) and two subtypes (**iter**ative and **rec**u**r**sive). Compile to obtain fuzzer executables - the command line options passable to the executables are as follows:

1. `fuzzer <min and max depth>`
2. `fuzzer <min depth> <max depth>`
3. `fuzzer <min depth> <max depth> <runs>` 

**TODO:** create python script to compile JSON grammar into C grammar struct for use with generic fuzzers.

## Compiled Fuzzers

**Description:** Python scripts that generate grammar-specific C fuzzer code, given a JSON grammar. 

View the source for compilers in `compiled/compilers`/ and generate C source and executables with `compiled/main.py` (due to the nature of python imports, it is imperative that `compiled/main.py` be run from within `compiled/`). The command line options passable to the executables are as follows:

1. `fuzzer <min and max depth>`
2. `fuzzer <min depth> <max depth>`

## Credits

The generic string fuzzer uses [uthash](https://troydhanson.github.io/uthash/) by Troy D. Hanson.
