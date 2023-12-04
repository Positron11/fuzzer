#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <limits.h>

#define LOW_COST 0
#define HIGH_COST 1
#define RAND_COST rand() % 2

#define STACK_LEN stack_ptr - stack // (jank, but I'll keep it for now)

// append function augmenter
#define OVERWRITE(target, target_ptr, source, len)		\
	target_ptr = target;								\
	target_ptr = append(target_ptr, source, len);

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

enum depth_lock_states {unlocked, locking, locked};
enum nonterminals {start = SCHAR_MIN, expr, term, factor, integer, digit};

void fuzzer(Grammar* grammar, depth_t min_depth, depth_t max_depth);
Rule* get_rule(Definition* definition, int cost);
token_t* append(token_t* target_ptr, token_t source[], size_t len);
token_t* prepend(token_t target[], token_t* target_ptr, token_t source[], size_t target_len, size_t source_len);

int main(int argc, char *argv[]) {
	if (argc <= 1) {
		puts("Error: seed argument required");
		return EXIT_FAILURE;
	}
	
	int seed = strtod(argv[1], 0);
	srand((unsigned) seed); // initialize random

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
				(Rule) { .token_count=3, .tokens=(token_t[]){integer, '.', integer} },
				(Rule) { .token_count=1, .tokens=(token_t[]){integer} },
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

	// set options from command line if possible otherwise set to defaults
	int min_depth = argc > 2 ? strtod(argv[2], 0) : 0;
	int max_depth = argc > 2 ? strtod(argv[argc - 1], 0) : 10;
	
	fuzzer(&grammar, min_depth, max_depth);

	return EXIT_SUCCESS;
}

// main fuzzer function
void fuzzer(Grammar* grammar, depth_t min_depth, depth_t max_depth) {
	token_t* stack = malloc(2097152 * sizeof(token_t));

	// declare chaser pointers for efficient stack modifications
	token_t* stack_ptr = stack;

	*(stack_ptr++) = start; // initialize stack with start token

	// recursion limit variables
	int recursion_lock_state = 0;
	int rule_cost = 0;

	// depth state variables
	depth_t stepwise_token_count[128] = {};
	depth_t current_depth = 0;

	int counter = 0;

	// while stack not empty...
	while (STACK_LEN > 0) {
		token_t token = stack[0]; // get first token
		if (current_depth > 0 && current_depth < max_depth) stepwise_token_count[current_depth - 1]--; // if not in cheap mode, decrement latest stepwise token count
		
		int buffer_len = STACK_LEN - 1; // get buffer length
		OVERWRITE(stack, stack_ptr, &stack[1], buffer_len); // overrwrite stack with buffer

		// if token is terminal append token to output
		if (token >= 0) {
			putchar(token);
			if (current_depth > 0 && current_depth >= max_depth) stepwise_token_count[current_depth - 1]--; // if haven't already, decrement latest stepwise token count
			while (stepwise_token_count[current_depth - 1] == 0) current_depth--; // roll back to nearest incomplete node
			continue;
		}
		
		Definition* definition = &grammar->definitions[token - start];

		rule_cost = current_depth < min_depth ? HIGH_COST : (current_depth >= max_depth ? LOW_COST : RAND_COST); // calculate cost based on depth...
		rule_cost = definition->rule_count[1] ? rule_cost : LOW_COST; // ...and force low if definition not recursive

		Rule* rule = get_rule(definition, rule_cost);

		if (current_depth < max_depth) stepwise_token_count[current_depth++] = rule->token_count; // if not in cheap mode, append token count to stepwise array
		else stepwise_token_count[current_depth - 1] += (rule->token_count) - 1; // otherwise if in cheap mode, add to current token count

		stack_ptr = prepend(stack, stack_ptr, rule->tokens, STACK_LEN, rule->token_count); // prepend rule to stack 
	}
}

// get rule from definition
Rule* get_rule(Definition* definition, int cost) {
	return &definition->rules[cost][rand() % (definition->rule_count)[cost]];
}

// append to token array
token_t* append(token_t* target_ptr, token_t source[], size_t len) {
	memmove(target_ptr, source, len * sizeof(token_t));
	return target_ptr + len;
}

// prepend to token array
token_t* prepend(token_t target[], token_t* target_ptr, token_t source[], size_t target_len, size_t source_len) {
	memmove(&target[source_len], target, target_len * sizeof(token_t)); // move existing contents to make space for source
	memcpy(target, source, source_len); // copy source into created space
	return target + source_len + target_len;
}
