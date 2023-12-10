#ifndef CSLIB_H_INCLUDED
#define CSLIB_H_INCLUDED

#include <stdio.h>
#include <limits.h>

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

enum nonterminals {start = SCHAR_MIN, expr, term, factor, integer, digit};

Grammar grammar = {.definitions=(Definition []) {
	[start - start] = (Definition) {.rule_count={1, 0}, .rules={
		(Rule []) {
			(Rule) { .token_count=1, .tokens=(token_t[]) {expr} },
		},
	} },
	[expr - start] = (Definition) {.rule_count={1, 2}, .rules={
		(Rule []) {
			(Rule) { .token_count=1, .tokens=(token_t[]) {term} },
		},
		(Rule []) {
			(Rule) { .token_count=3, .tokens=(token_t[]) {term, '+', expr} },
			(Rule) { .token_count=3, .tokens=(token_t[]) {term, '-', expr} },
		},
	} },
	[term - start] = (Definition) {.rule_count={1, 2}, .rules={
		(Rule []) {
			(Rule) { .token_count=1, .tokens=(token_t[]) {factor} },
		},
		(Rule []) {
			(Rule) { .token_count=3, .tokens=(token_t[]) {factor, '*', term} },
			(Rule) { .token_count=3, .tokens=(token_t[]) {factor, '/', term} },
		},
	} },
	[factor - start] = (Definition) {.rule_count={2, 3}, .rules={
		(Rule []) {
			(Rule) { .token_count=3, .tokens=(token_t[]) {integer, '.', integer} },
			(Rule) { .token_count=1, .tokens=(token_t[]) {integer} },
		},
		(Rule []) {
			(Rule) { .token_count=2, .tokens=(token_t[]) {'+', factor} },
			(Rule) { .token_count=2, .tokens=(token_t[]) {'-', factor} },
			(Rule) { .token_count=3, .tokens=(token_t[]) {'(', expr, ')'} },
		},
	} },
	[integer - start] = (Definition) {.rule_count={1, 1}, .rules={
		(Rule []) {
			(Rule) { .token_count=1, .tokens=(token_t[]) {digit} },
		},
		(Rule []) {
			(Rule) { .token_count=2, .tokens=(token_t[]) {digit, integer} },
		},
	} },
	[digit - start] = (Definition) {.rule_count={10, 0}, .rules={
		(Rule []) {
			(Rule) { .token_count=1, .tokens=(token_t[]) {'0'} },
			(Rule) { .token_count=1, .tokens=(token_t[]) {'1'} },
			(Rule) { .token_count=1, .tokens=(token_t[]) {'2'} },
			(Rule) { .token_count=1, .tokens=(token_t[]) {'3'} },
			(Rule) { .token_count=1, .tokens=(token_t[]) {'4'} },
			(Rule) { .token_count=1, .tokens=(token_t[]) {'5'} },
			(Rule) { .token_count=1, .tokens=(token_t[]) {'6'} },
			(Rule) { .token_count=1, .tokens=(token_t[]) {'7'} },
			(Rule) { .token_count=1, .tokens=(token_t[]) {'8'} },
			(Rule) { .token_count=1, .tokens=(token_t[]) {'9'} },
		},
	} },
} };

#endif
