#ifndef GRAMSTRUCT_H_INCLUDED
#define GRAMSTRUCT_H_INCLUDED

#include <stdio.h>

typedef signed char token_t;

typedef struct Rule {
	size_t token_count;
	token_t* tokens;
} Rule;

typedef struct Definition {
	size_t rule_count[2];
	Rule* rules[2]; // [0]: cheap, [1]: costly
} Definition;

typedef struct Grammar {
	Definition* definitions;
} Grammar;

// placeholder declarations so fuzzer core compiles independently
#ifndef GRAMMAR_H_INCLUDED
Grammar grammar;
token_t start;
#endif

#endif