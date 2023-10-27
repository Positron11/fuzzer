#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <limits.h>

#define DEPTHLOCK_TOKEN INT_MIN
#define START_TOKEN DEPTHLOCK_TOKEN + 1

// expansion cost definitions
#define LOW_COST 0
#define HIGH_COST 1
#define RAND_COST rand() % 2

// depth lock states
enum depth_lock_states {unlocked, locking, locked};

// overrwrite stack function-like macro
#define OVERWRITE(ptr, zero, len, new)			\
	memcpy(zero, new, len*sizeof(int));			\
	ptr = zero + len;

// append to stack function-like macro
#define APPEND(len, ptr, new)					\
	memcpy(ptr, new, len*sizeof(int));			\
	ptr += len;

// rule structure
typedef struct Rule {
	size_t token_count;
	int const* tokens;
} Rule;

// definition structure
typedef struct Definition {
	char const* name;
	size_t rule_count[2];
	Rule* rules[2]; // [0]: cheap, [1]: costly
} Definition;

// grammar structure
typedef struct Grammar {
	size_t def_count;
	Definition* definitions;
} Grammar;

// function declarations
void fuzzer(Grammar* grammar, unsigned int min_depth, unsigned int max_depth);
Rule* get_rule(Grammar* grammar, int def, int cost);

int main(int argc, char const *argv[]) {
	srand((unsigned) time(0)); // initialize random

	// define grammar
	enum special {
		start = START_TOKEN, 
		phone, 
		area, 
		number, 
		digit,
	};
	Grammar grammar = { .def_count=5, .definitions=(Definition []) {
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
void fuzzer(Grammar* grammar, unsigned int min_depth, unsigned int max_depth) {
	// declare stack and output
	int stack[524288];
	int output[524288];

	stack[0] = START_TOKEN; // set initial stack value to start token

	// set array pointers for efficient concatenation
	int* stack_ptr = &stack[1];
	int* out_ptr = output;

	// recursion limit variables
	unsigned int cost = 0;
	unsigned int depth = 0;
	unsigned int depth_lock = 0;

	// while stack not empty
	while ((stack_ptr - stack) > 0) {
		int token = stack[0]; // get token

		// get buffer
		int buffer_len = (stack_ptr - stack) - 1;
		int buffer[buffer_len];
		memcpy(buffer, stack + 1, buffer_len*sizeof(int));
		
		if (token < 1) { // if first token in stack indicates nonterminal...
			if (token == DEPTHLOCK_TOKEN) { // if current token is depth lock token...
				depth = 0;
				OVERWRITE(stack_ptr, stack, buffer_len, buffer); // overwrite stack with buffer
				depth_lock = unlocked;

			} else { // ...otherwise if current token is expandable
				// set cost based on recursion limits
				if (depth < min_depth) cost = HIGH_COST;
				else if (depth >= max_depth) cost = LOW_COST;
				else cost = RAND_COST;

				cost = (((grammar->definitions)[-(START_TOKEN) + (token)]).rule_count)[1] ? cost : LOW_COST; // force low cost if def not recursive

				// increment depth if high cost (recursive) expansion
				if (cost == HIGH_COST) {
					if (depth_lock == unlocked) depth_lock = locking;
					depth++;
				}

				// overwrite stack with random(?) rule
				Rule* rule = get_rule(grammar, token, cost);
				OVERWRITE(stack_ptr, stack, rule->token_count, (rule->tokens));

				if (depth_lock == locking) { // if in locking stage...
					*(stack_ptr++) = DEPTHLOCK_TOKEN; // ...append lock token to stack
					depth_lock = locked;
				}

				APPEND(buffer_len, stack_ptr, buffer); // append buffer to stack
			}
		
		} else { // ...otherwise if first token in stack is terminal
			memcpy(out_ptr++, (int []) {token}, sizeof(int)); // append token to output

			OVERWRITE(stack_ptr, stack, buffer_len, buffer); // overwrite stack with buffer
		}
	}
	
	// print output
	for (size_t i = 0; i < out_ptr - output; i++) printf("%c", output[i]);
	printf("\n");
}

// get rule by key
Rule* get_rule(Grammar* grammar, int def, int cost) {
	Definition definition = (grammar->definitions)[-(START_TOKEN) + (def)];
	return &((definition.rules)[cost])[rand() % (definition.rule_count)[cost]];
}
