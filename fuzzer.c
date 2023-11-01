#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <limits.h>

// cost definitions
#define LOW_COST 0
#define HIGH_COST 1
#define RAND_COST rand() % 2

// computed definitions
#define STACK_LEN stack_ptr - stack

// overrwrite function-like macro
#define OVERWRITE(target, target_ptr, source, len)		\
	target_ptr = target;								\
	target_ptr = append(target_ptr, source, len);

// type definitions
typedef signed char token_t;
typedef size_t depth_t;

// rule structure
typedef struct Rule {
	size_t token_count;
	token_t* tokens;
} Rule;

// definition structure
typedef struct Definition {
	char* name;
	size_t rule_count[2];
	Rule* rules[2]; // [0]: cheap, [1]: costly
} Definition;

// grammar structure
typedef struct Grammar {
	size_t def_count;
	Definition* definitions;
} Grammar;

// enumeratedants
enum depth_lock_states {unlocked, locking, locked};
enum nonterminals {start = SCHAR_MIN, phone, area, number, digit, depthlock};

// function declarations
void fuzzer(Grammar* grammar, depth_t min_depth, depth_t max_depth);
Rule* get_rule(Definition* definition, int cost);
token_t* append(token_t* target_ptr, token_t source[], size_t len);
token_t* prepend(token_t target[], token_t* target_ptr, token_t source[], size_t target_len, size_t source_len);

int main(int argc, char *argv[]) {
	srand((unsigned) time(0)); // initialize random

	// define grammar
	Grammar grammar = { .def_count=5, .definitions=(Definition []) {
		(Definition) { .name="start", .rule_count={1, 0}, .rules={
			(Rule []) {
				(Rule) { .token_count=1, .tokens=(token_t[]) {phone} }
			}
		} },
		(Definition) { .name="phone", .rule_count={2, 0}, .rules={
			(Rule []) {
				(Rule) { .token_count=3, .tokens=(token_t[]){number, '-', number} },
				(Rule) { .token_count=4, .tokens=(token_t[]){area, number, '-', number} }
			}
		} },
		(Definition) { .name="area", .rule_count={1, 0}, .rules={
			(Rule []) {
				(Rule) { .token_count=5, .tokens=(token_t[]){'(', '+', digit, digit, ')'} }
			}
		} },
		(Definition) { .name="number", .rule_count={1, 1}, .rules={
			(Rule []) {
				(Rule) { .token_count=1, .tokens=(token_t[]){digit} },
			},
			(Rule []) {
				(Rule) { .token_count=2, .tokens=(token_t[]){digit, number} }
			}
		} },
		(Definition) { .name="digit", .rule_count={10, 0}, .rules={
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
	depth_t min_depth = argc > 1 ? strtod(argv[1], 0) : 2;
	depth_t max_depth = argc > 1 ? (argc > 2 ? strtod(argv[2], 0) : min_depth) : 4;
	depth_t runs = argc > 3 ? strtod(argv[3], 0) : 1;
	
	// main loop
	for (size_t i = 0; i < runs; i++) {
		fuzzer(&grammar, min_depth, max_depth);
	}

	return EXIT_SUCCESS;
}

// fuzzer function
void fuzzer(Grammar* grammar, depth_t min_depth, depth_t max_depth) {
	// declare stack and output
	token_t* stack = malloc(2097152 * sizeof(token_t));
	token_t* output = malloc(2097152 * sizeof(token_t));

	// declare chaser pointers for efficient stack modifications
	token_t* stack_ptr = stack;
	token_t* out_ptr = output;

	*(stack_ptr++) = start; // initialize stack

	// recursion limit variables
	int rule_cost = 0;
	depth_t current_depth = 0;
	int recursion_lock_state = 0;

	// while stack not emmpty
	while (STACK_LEN > 0) {
		token_t token = stack[0]; // get token
		
		int buffer_len = STACK_LEN - 1; // get buffer length
		OVERWRITE(stack, stack_ptr, &stack[1], buffer_len); // overrwrite stack with buffer

		if (token >= 0) { // if token is terminal
			*(out_ptr++) = token; // append token to output6
			continue;
		}
		
		if (token == depthlock) { // if token is depthlock token
			recursion_lock_state = unlocked;
			current_depth = 0;
			continue;	
		} 
		
		Definition* definition = &grammar->definitions[token - start];
		
		rule_cost = current_depth < min_depth ? HIGH_COST : (current_depth >= max_depth ? LOW_COST : RAND_COST); // calculate cost based on depth
		rule_cost = definition->rule_count[1] ? rule_cost : LOW_COST; // force low if definition not recursive
		
		if (rule_cost == HIGH_COST) {
			if (recursion_lock_state == unlocked) recursion_lock_state = locking; // if in locking state promote to locked
			current_depth++;
		}

		Rule* rule = get_rule(definition, rule_cost);

		if (recursion_lock_state == locking) {
			stack_ptr = prepend(stack, stack_ptr, (token_t []) {depthlock}, STACK_LEN, 1); // prepend depthlock token to stack
			recursion_lock_state = locked;
		}

		stack_ptr = prepend(stack, stack_ptr, rule->tokens, STACK_LEN, rule->token_count); // prepend rule to stack
	}

	for (size_t i = 0; i < out_ptr - output; i++) putchar(output[i]); // print output
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