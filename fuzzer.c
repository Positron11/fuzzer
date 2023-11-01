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

// rule structure
typedef struct Rule {
	size_t const token_count;
	signed char const* tokens;
} Rule;

// definition structure
typedef struct Definition {
	char const* name;
	size_t const rule_count[2];
	Rule const* rules[2]; // [0]: cheap, [1]: costly
} Definition;

// grammar structure
typedef struct Grammar {
	size_t const def_count;
	Definition const* definitions;
} Grammar;

enum depth_lock_states {unlocked, locking, locked}; // depth lock states
enum nonterminals {start = SCHAR_MIN, phone, area, number, digit, depthlock}; // generate nonterminal tokens

// function declarations
void fuzzer(Grammar const* grammar, unsigned int min_depth, unsigned int max_depth);
Rule const* get_rule(Definition const* definition, int cost);
signed char* append(signed char* target_ptr, signed char const source[], size_t len);

int main(int argc, char const *argv[]) {
	srand((unsigned) time(0)); // initialize random

	// define grammar
	Grammar const grammar = { .def_count=5, .definitions=(Definition []) {
		(Definition) { .name="start", .rule_count={1, 0}, .rules={
			(Rule []) {
				(Rule) { .token_count=1, .tokens=(signed char const[]) {phone} }
			}
		} },
		(Definition) { .name="phone", .rule_count={2, 0}, .rules={
			(Rule []) {
				(Rule) { .token_count=3, .tokens=(signed char const[]){number, '-', number} },
				(Rule) { .token_count=4, .tokens=(signed char const[]){area, number, '-', number} }
			}
		} },
		(Definition) { .name="area", .rule_count={1, 0}, .rules={
			(Rule []) {
				(Rule) { .token_count=5, .tokens=(signed char const[]){'(', '+', digit, digit, ')'} }
			}
		} },
		(Definition) { .name="number", .rule_count={1, 1}, .rules={
			(Rule []) {
				(Rule) { .token_count=1, .tokens=(signed char const[]){digit} },
			},
			(Rule []) {
				(Rule) { .token_count=2, .tokens=(signed char const[]){digit, number} }
			}
		} },
		(Definition) { .name="digit", .rule_count={10, 0}, .rules={
			(Rule []) {
				(Rule) { .token_count=1, .tokens=(signed char const[]){'0'} },
				(Rule) { .token_count=1, .tokens=(signed char const[]){'1'} },
				(Rule) { .token_count=1, .tokens=(signed char const[]){'2'} },
				(Rule) { .token_count=1, .tokens=(signed char const[]){'3'} },
				(Rule) { .token_count=1, .tokens=(signed char const[]){'4'} },
				(Rule) { .token_count=1, .tokens=(signed char const[]){'5'} },
				(Rule) { .token_count=1, .tokens=(signed char const[]){'6'} },
				(Rule) { .token_count=1, .tokens=(signed char const[]){'7'} },
				(Rule) { .token_count=1, .tokens=(signed char const[]){'8'} },
				(Rule) { .token_count=1, .tokens=(signed char const[]){'9'} },
			}
		} }
	} };

	// set options from command line if possible otherwise default
	unsigned int min_depth = argc > 1 ? strtod(argv[1], 0) : 2;
	unsigned int max_depth = argc > 1 ? (argc > 2 ? strtod(argv[2], 0) : min_depth) : 4;
	unsigned int runs = argc > 3 ? strtod(argv[3], 0) : 1;
	
	// main loop
	for (size_t i = 0; i < runs; i++) {
		fuzzer(&grammar, min_depth, max_depth);
	}

	return EXIT_SUCCESS;
}

// fuzzer function
void fuzzer(Grammar const* grammar, unsigned int min_depth, unsigned int max_depth) {
	// declare stack and output
	signed char* stack = malloc(2097152 * sizeof(signed char));
	signed char* output = malloc(2097152 * sizeof(signed char));

	// declare chaser pointers for efficient stack modifications
	signed char* stack_ptr = stack;
	signed char* out_ptr = output;

	*(stack_ptr++) = start; // initialize stack

	// recursion limit variables
	unsigned int cost = 0;
	unsigned int depth = 0;
	unsigned int recursion_lock_state = 0;

	// while stack not emmpty
	while (STACK_LEN > 0) {
		signed char token = stack[0]; // get token
		
		int buffer_len = STACK_LEN - 1; // get buffer length
		
		if (token < 0) { // if token is nonterminal...
			if (token == depthlock) { // if token is depthlock token...
				OVERWRITE(stack, stack_ptr, &stack[1], buffer_len); // overrwrite stack with buffer
				
				// reset depth lock vars
				recursion_lock_state = unlocked;
				depth = 0;				

			} else { // ...otherwise if token is rule
				Definition const* definition = &grammar->definitions[-(start - token)]; // get definition
				
				// calculate cost based on depth and force low if definition not recursive
				unsigned int cost = depth < min_depth ? HIGH_COST : (depth >= max_depth ? LOW_COST : RAND_COST);
				cost = definition->rule_count[1] ? cost : LOW_COST;
				
				// increment depth if high cost (recursive) expansion
				if (cost == HIGH_COST) {
					if (recursion_lock_state == unlocked) recursion_lock_state = locking;
					depth++;
				}

				Rule const* rule = get_rule(definition, cost); // get rule

				// calculate buffer shift and shift buffer up to make room for rule
				size_t shift = rule->token_count + (recursion_lock_state == locking);
				memmove(&stack[shift], &stack[1], buffer_len * sizeof(signed char));
				
				OVERWRITE(stack, stack_ptr, rule->tokens, rule->token_count); // overrwrite stack with rule tokens

				if (recursion_lock_state == locking) { // if in locking stage...
					*(stack_ptr++) = depthlock; // ...append lock token to stack
					recursion_lock_state = locked;
				}

				stack_ptr = stack + shift + buffer_len; // move stack pointer to end of stack
			}

		} else { // ...otherwise if token is terminal
			*(out_ptr++) = token; // append token to output
			
			OVERWRITE(stack, stack_ptr, &stack[1], buffer_len); // overrwrite stack with buffer
		}
	}

	for (size_t i = 0; i < out_ptr - output; i++) putchar(output[i]); // print output
}

// get rule from definition
Rule const* get_rule(Definition const* definition, int cost) {
	return &definition->rules[cost][rand() % (definition->rule_count)[cost]];
}

// append function
signed char* append(signed char* target_ptr, signed char const source[], size_t len) {
	memmove(target_ptr, source, len * sizeof(signed char));
	return target_ptr + len;
}