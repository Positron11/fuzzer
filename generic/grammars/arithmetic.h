#ifndef GRAMMAR_H_INCLUDED
#define GRAMMAR_H_INCLUDED

#include <limits.h>
#include "grammar.h"

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
			(Rule) { .token_count=3, .tokens=(token_t[]) {term, 43, expr} },
			(Rule) { .token_count=3, .tokens=(token_t[]) {term, 45, expr} },
		},
	} },
	[term - start] = (Definition) {.rule_count={1, 2}, .rules={
		(Rule []) {
			(Rule) { .token_count=1, .tokens=(token_t[]) {factor} },
		},
		(Rule []) {
			(Rule) { .token_count=3, .tokens=(token_t[]) {factor, 42, term} },
			(Rule) { .token_count=3, .tokens=(token_t[]) {factor, 47, term} },
		},
	} },
	[factor - start] = (Definition) {.rule_count={2, 3}, .rules={
		(Rule []) {
			(Rule) { .token_count=3, .tokens=(token_t[]) {integer, 46, integer} },
			(Rule) { .token_count=1, .tokens=(token_t[]) {integer} },
		},
		(Rule []) {
			(Rule) { .token_count=2, .tokens=(token_t[]) {43, factor} },
			(Rule) { .token_count=2, .tokens=(token_t[]) {45, factor} },
			(Rule) { .token_count=3, .tokens=(token_t[]) {40, expr, 41} },
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
			(Rule) { .token_count=1, .tokens=(token_t[]) {48} },
			(Rule) { .token_count=1, .tokens=(token_t[]) {49} },
			(Rule) { .token_count=1, .tokens=(token_t[]) {50} },
			(Rule) { .token_count=1, .tokens=(token_t[]) {51} },
			(Rule) { .token_count=1, .tokens=(token_t[]) {52} },
			(Rule) { .token_count=1, .tokens=(token_t[]) {53} },
			(Rule) { .token_count=1, .tokens=(token_t[]) {54} },
			(Rule) { .token_count=1, .tokens=(token_t[]) {55} },
			(Rule) { .token_count=1, .tokens=(token_t[]) {56} },
			(Rule) { .token_count=1, .tokens=(token_t[]) {57} },
		},
	} },
} };

#endif
