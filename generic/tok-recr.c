#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <limits.h>

#define LOW_COST 0
#define HIGH_COST 1
#define RAND_COST rand() % 2

#define BUFFER_LEN (token_count - 1) // (jank, but I'll keep it for now)

typedef signed char token_t;
typedef size_t depth_t;

typedef struct Rule {
	size_t token_count;
	token_t* tokens;
} Rule;

typedef struct Definition {
	size_t rule_count[2];
	Rule* rules[2]; // [0]: cheap, [1]: costly
} Definition;

typedef struct Grammar {
	size_t def_count;
	Definition* definitions;
} Grammar;

enum nonterminals {start = SCHAR_MIN, expr, term, factor, integer, digit};

void fuzzer(Grammar* grammar, depth_t min_depth, depth_t max_depth, depth_t depth, token_t* tokens, size_t token_count);
Rule* get_rule(Definition* definition, int cost);

int main(int argc, char *argv[]) {
	srand((unsigned) time(0)); // initialize random

	Grammar grammar = { .def_count=5, .definitions=(Definition []) {
		(Definition) {.rule_count={1, 0}, .rules={ // start
			(Rule []) {
				(Rule) { .token_count=1, .tokens=(token_t[]) {expr} }
			}
		} },
		(Definition) {.rule_count={1, 2}, .rules={ // expr
			(Rule []) {
				(Rule) { .token_count=1, .tokens=(token_t[]){term} }
			},
			(Rule []) {
				(Rule) { .token_count=3, .tokens=(token_t[]){term, '+', expr} },
				(Rule) { .token_count=3, .tokens=(token_t[]){term, '-', expr} }
			}
		} },
		(Definition) {.rule_count={1, 2}, .rules={ // term
			(Rule []) {
				(Rule) { .token_count=1, .tokens=(token_t[]){factor} }
			},
			(Rule []) {
				(Rule) { .token_count=3, .tokens=(token_t[]){factor, '*', term} },
				(Rule) { .token_count=3, .tokens=(token_t[]){factor, '/', term} }
			}
		} },
		(Definition) {.rule_count={2, 3}, .rules={ // factor
			(Rule []) {
				(Rule) { .token_count=1, .tokens=(token_t[]){integer} },
				(Rule) { .token_count=3, .tokens=(token_t[]){integer, '.', integer} },
			},
			(Rule []) {
				(Rule) { .token_count=2, .tokens=(token_t[]){'+', factor} },
				(Rule) { .token_count=2, .tokens=(token_t[]){'-', factor} },
				(Rule) { .token_count=3, .tokens=(token_t[]){'(', expr, ')'} }
			}
		} },
		(Definition) {.rule_count={1, 1}, .rules={ // integer
			(Rule []) {
				(Rule) { .token_count=1, .tokens=(token_t[]){digit} },
			},
			(Rule []) {
				(Rule) { .token_count=2, .tokens=(token_t[]){digit, integer} }
			}
		} },
		(Definition) {.rule_count={10, 0}, .rules={ // digit
			(Rule []) {
				(Rule) { .token_count=1, .tokens=(token_t[]){'0'} },
				(Rule) { .token_count=1, .tokens=(token_t[]){'1'} },
				(Rule) { .token_count=1, .tokens=(token_t[]){'2'} },
				(Rule) { .token_count=1, .tokens=(token_t[]){'3'} },
				(Rule) { .token_count=1, .tokens=(token_t[]){'4'} },
				(Rule) { .token_count=1, .tokens=(token_t[]){'5'} },
				(Rule) { .token_count=1, .tokens=(token_t[]){'6'} },
				(Rule) { .token_count=1, .tokens=(token_t[]){'7'} },
				(Rule) { .token_count=1, .tokens=(token_t[]){'8'} },
				(Rule) { .token_count=1, .tokens=(token_t[]){'9'} },
			}
		} }
	} };

	// set options from command line if possible otherwise default
	depth_t min_depth = argc > 1 ? strtod(argv[1], 0) : 2;
	depth_t max_depth = argc > 1 ? (argc > 2 ? strtod(argv[2], 0) : min_depth) : 4;
	
	fuzzer(&grammar, min_depth, max_depth, 0, (token_t[]) {start}, 1);

	return EXIT_SUCCESS;
}

// main fuzzer function
void fuzzer(Grammar* grammar, depth_t min_depth, depth_t max_depth, depth_t current_depth, token_t* tokens, size_t token_count) {
	token_t token = tokens[0]; // get first token

	// if token is terminal print to stdout
	if (token >= 0) {
		putchar(token);

	} else {
		Definition* definition = &grammar->definitions[token - start];

		int rule_cost = current_depth < min_depth ? HIGH_COST : (current_depth >= max_depth ? LOW_COST : RAND_COST); // calculate cost based on depth...
		rule_cost = definition->rule_count[1] ? rule_cost : LOW_COST; // ...and force low if definition not recursive
		
		Rule* rule = get_rule(definition, rule_cost);

		fuzzer(grammar, min_depth, max_depth, current_depth + 1, rule->tokens, rule->token_count); // fuzz the rule's expansion
	}
	
	if (BUFFER_LEN) { // if buffer exists...
		token_t buffer[BUFFER_LEN];
		memcpy(buffer, tokens + 1, BUFFER_LEN*sizeof(token_t));

		fuzzer(grammar, min_depth, max_depth, current_depth, buffer, BUFFER_LEN); // ...fuzz the buffer
	}
}

// get rule from definition
Rule* get_rule(Definition* definition, int cost) {
	return &((definition->rules)[cost])[rand() % (definition->rule_count)[cost]];
}
