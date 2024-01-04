#ifndef GSTRUCT_H_INCLUDED
#define GSTRUCT_H_INCLUDED

#include <stdio.h>
#include <limits.h>

#define START SCHAR_MIN

typedef signed char token_t;

typedef struct Rule {
	size_t token_count;
	token_t* tokens;
} Rule;

typedef struct Definition {
	size_t rule_count[2];
	Rule* rules[2]; // [0]: cheap, [1]: expensive
} Definition;

typedef struct Grammar {
	Definition* definitions;
} Grammar;

#endif