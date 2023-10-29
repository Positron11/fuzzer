#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <limits.h>

// token definitions
#define DEPTHLOCK_TOKEN -100
#define START_TOKEN -100 + 1

// cost definitions
#define LOW_COST 0
#define HIGH_COST 1
#define RAND_COST rand() % 2

// computed definitions
#define STACK_LEN stack_ptr - stack

// append function-like macro
#define APPEND(target_ptr, source, len)					\
	memcpy(target_ptr, source, len * sizeof(int));		\
	target_ptr += len;

// overrwrite function-like macro
#define OVERWRITE(target, target_ptr, source, len)		\
	target_ptr = target;								\
	APPEND(target_ptr, source, len);

// depth lock states
enum depth_lock_states {unlocked, locking, locked};

// rule structure
typedef struct Rule {
	size_t const token_count;
	int const* tokens;
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

// function declarations
void fuzzer(Grammar const* grammar, unsigned int min_depth, unsigned int max_depth);
Rule const* get_rule(Definition const* definition, int cost);

int main(int argc, char const *argv[]) {
	srand((unsigned) time(0)); // initialize random

	// generate nonterminal tokens
	enum special {start = START_TOKEN, phone, area, number, digit};

	// define grammar
	Grammar const grammar = { .def_count=5, .definitions=(Definition []) {
		(Definition) { .name="start", .rule_count={1, 0}, .rules={
			(Rule []) {
				(Rule) { .token_count=1, .tokens=(int const[]) {phone} }
			}
		} },
		(Definition) { .name="phone", .rule_count={2, 0}, .rules={
			(Rule []) {
				(Rule) { .token_count=3, .tokens=(int const[]){number, '-', number} },
				(Rule) { .token_count=4, .tokens=(int const[]){area, number, '-', number} }
			}
		} },
		(Definition) { .name="area", .rule_count={1, 0}, .rules={
			(Rule []) {
				(Rule) { .token_count=5, .tokens=(int const[]){'(', '+', digit, digit, ')'} }
			}
		} },
		(Definition) { .name="number", .rule_count={1, 1}, .rules={
			(Rule []) {
				(Rule) { .token_count=1, .tokens=(int const[]){digit} },
			},
			(Rule []) {
				(Rule) { .token_count=2, .tokens=(int const[]){number, digit} }
			}
		} },
		(Definition) { .name="digit", .rule_count={10, 0}, .rules={
			(Rule []) {
				(Rule) { .token_count=1, .tokens=(int const[]){'0'} },
				(Rule) { .token_count=1, .tokens=(int const[]){'1'} },
				(Rule) { .token_count=1, .tokens=(int const[]){'2'} },
				(Rule) { .token_count=1, .tokens=(int const[]){'3'} },
				(Rule) { .token_count=1, .tokens=(int const[]){'4'} },
				(Rule) { .token_count=1, .tokens=(int const[]){'5'} },
				(Rule) { .token_count=1, .tokens=(int const[]){'6'} },
				(Rule) { .token_count=1, .tokens=(int const[]){'7'} },
				(Rule) { .token_count=1, .tokens=(int const[]){'8'} },
				(Rule) { .token_count=1, .tokens=(int const[]){'9'} },
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
	int stack[262144];
	int output[262144];

	stack[0] = START_TOKEN; // initialize stack

	// declare chaser pointers for efficient stack modifications
	int* stack_ptr = &stack[1];
	int* out_ptr = output;

	// recursion limit variables
	unsigned int cost = 0;
	unsigned int depth = 0;
	unsigned int depth_lock = 0;

	while (STACK_LEN > 0) {
		int token = stack[0]; // get token
		
		// get buffer
		int buffer_len = STACK_LEN - 1;
		int buffer[buffer_len];
		memcpy(buffer, stack + 1, buffer_len * sizeof(int));
		
		if (token < 0) { // if token is nonterminal...
			if (token == DEPTHLOCK_TOKEN) { // if token is depthlock token...
				// reset depth lock vars
				depth = 0;
				depth_lock = unlocked;

				OVERWRITE(stack, stack_ptr, buffer, buffer_len); // overrwrite stack with buffer

			} else { // ...otherwise if token is rule
				Definition const* definition = &grammar->definitions[-(START_TOKEN - token)]; // get definition
				
				// calculate cost based on depth and force low if definition not recursive
				unsigned int cost = depth < min_depth ? HIGH_COST : (depth >= max_depth ? LOW_COST : RAND_COST);
				cost = definition->rule_count[1] ? cost : LOW_COST;
				
				// increment depth if high cost (recursive) expansion
				if (cost == HIGH_COST) {
					if (depth_lock == unlocked) depth_lock = locking;
					depth++;
				}

				Rule const* rule = get_rule(definition, cost); // get rule

				OVERWRITE(stack, stack_ptr, rule->tokens, rule->token_count); // overrwrite stack with rule tokens

				if (depth_lock == locking) { // if in locking stage...
					*(stack_ptr++) = DEPTHLOCK_TOKEN; // ...append lock token to stack
					depth_lock = locked;
				}

				APPEND(stack_ptr, buffer, buffer_len); // append buffer to stack
			}

		} else { // ...otherwise if token is terminal
			*(out_ptr++) = token; // append token to output

			OVERWRITE(stack, stack_ptr, buffer, buffer_len); // overrwrite stack with buffer
		}
	}

	for (size_t i = 0; i < out_ptr - output; i++) putchar(output[i]);
}

// get rule from definition
Rule const* get_rule(Definition const* definition, int cost) {
	return &definition->rules[cost][rand() % (definition->rule_count)[cost]];
}