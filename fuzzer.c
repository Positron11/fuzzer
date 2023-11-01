#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "uthash.h"

// token definitions
#define START_TOKEN "<start>"
#define DEPTH_LOCK_TOKEN "<dlock>"

// expansion cost definitions
#define LOW_COST 0
#define HIGH_COST 1
#define RAND_COST rand() % 2

// computed definitions
#define STACK_LEN stack_ptr - stack

// overrwrite string using append function-like macro
#define OVERRWRITE(target, target_ptr, source, len)						\
	target_ptr = target;												\
	target_ptr = append(target_ptr, source, len);

// depth lock states
enum depth_lock_states {unlocked, locking, locked};

// hashable rule structure
typedef struct Definition {
	char const key[32]; // uthash hashtable key
	size_t const rule_count[2]; // count of expansion options
	char const** rules[2]; // expansion options ([0]: cheap, [1]: costly)
	UT_hash_handle hh;
} Definition;

// declare functions
void fuzzer(Definition const* grammar, unsigned int min_depth, unsigned int max_depth);
char const* get_rule(Definition const* rule, unsigned int cost);
Definition const* get_definition(Definition const* grammar, char* key);
char* append(char* target_ptr, char const source[], size_t len);
void slice(char target[], char const source[], size_t start, size_t end);

int main(int argc, char const *argv[]) {
	srand((unsigned) time(0)); // initialize random

	Definition const* grammar = {0}; // declare grammar as pointers to rules

	// define grammar rules
	Definition start = { .key=START_TOKEN, .rule_count={1,0}, .rules={ 
		(char const*[]) {"<phone>"},
	} };
	Definition phone = { .key="<phone>", .rule_count={2,0}, .rules={ 
		(char const*[]) {"<area><number>-<number>", "<number>-<number>"},
	} };	
	Definition area = { .key="<area>", .rule_count={1,0}, .rules={ 
		(char const*[]) {"(+<digit><digit>)"},
	} };
	Definition number = { .key="<number>", .rule_count={1,1}, .rules={ 
		(char const*[]) {"<digit>"},
		(char const*[]) {"<digit><number>"},
	} };
	Definition digit = { .key="<digit>", .rule_count={10,0}, .rules={ 
		(char const*[]) {"0", "1", "2", "3", "4", "5", "6", "7", "8", "9"},
	} };
	
	// add rules to grammar
	HASH_ADD_INT(grammar, key, &start);
	HASH_ADD_INT(grammar, key, &phone);
	HASH_ADD_INT(grammar, key, &area);
	HASH_ADD_INT(grammar, key, &number);
	HASH_ADD_INT(grammar, key, &digit);

	// set options from command line if possible otherwise default
	unsigned int min_depth = argc > 1 ? strtod(argv[1], 0) : 2;
	unsigned int max_depth = argc > 1 ? (argc > 2 ? strtod(argv[2], 0) : min_depth) : 4;
	unsigned int runs = argc > 3 ? strtod(argv[3], 0) : 1;
	
	// main loop
	for (size_t i = 0; i < runs; i++) {
		fuzzer(grammar, min_depth, max_depth);
	}

	return EXIT_SUCCESS;
}

// fuzzing function
void fuzzer(Definition const* grammar, unsigned int min_depth, unsigned int max_depth) {
	// [TODO) dynamically allocate stack and output memory
	// declare stack and output
	char* stack = malloc(2097152 * sizeof(char));
	char* output = malloc(2097152 * sizeof(char));

	// set array pointers for efficient concatenation
	char* stack_ptr = stack;
	char* out_ptr = output;

	// initialize stack
	OVERRWRITE(stack, stack_ptr, START_TOKEN, strlen(START_TOKEN));

	// recursion limit variables
	unsigned int cost = 0;
	unsigned int depth = 0;
	unsigned int depth_lock = 0;

	// declare token, nonterms, and buffer stores
	char* token = malloc(2097152 * sizeof(char));
	char* nonterms = malloc(2097152 * sizeof(char));
	char* buffer = malloc(2097152 * sizeof(char));

	// while stack not empty
	while (STACK_LEN > 0) {
		if (stack[0] == '<') { // if first token in stack indicates nonterminal...
			// slice token and (remaining) buffer out of stack
			size_t token_len = strcspn(stack, ">") + 1;
			slice(token, stack, 0, token_len);
			slice(buffer, stack, token_len, STACK_LEN);
			size_t buffer_len = STACK_LEN - token_len; // save buffer length for later

			if (strcmp(token, DEPTH_LOCK_TOKEN) == 0) { // if current token is depth lock token...
				depth = 0;
				depth_lock = unlocked;

				OVERRWRITE(stack, stack_ptr, buffer, buffer_len); // write buffer to stack

			} else { // ...otherwise if current token is expandable
				Definition const* definition = get_definition(grammar, token); // get definition

				// calculate cost based on depth and force low if definition not recursive
				unsigned int cost = depth < min_depth ? HIGH_COST : (depth >= max_depth ? LOW_COST : RAND_COST);
				cost = definition->rule_count[1] ? cost : LOW_COST;

				// increment depth if high cost (recursive) expansion
				if (cost == HIGH_COST) {
					if (depth_lock == unlocked) depth_lock = locking;
					depth++;
				}

				char const* rule = get_rule(definition, cost); // get rule

				OVERRWRITE(stack, stack_ptr, rule, strlen(rule)); // write random(?) expansion to stack

				if (depth_lock == locking) { // if in locking stage...
					stack_ptr = append(stack_ptr, DEPTH_LOCK_TOKEN, 7); // ...append lock token to stack
					depth_lock = locked;
				}

				stack_ptr = append(stack_ptr, buffer, buffer_len); // append buffer to stack
			}

		} else { // ...otherwise if first token in stack is terminal
			// slice leading nonterminal tokens and (remaining) buffer out of stack
			size_t term_len = strcspn(stack, "<");
			slice(nonterms, stack, 0, term_len);
			slice(buffer, stack, term_len, STACK_LEN);

			size_t buffer_len = STACK_LEN - term_len; // save buffer length for later

			// append terminals to output and write buffer to stack
			out_ptr = append(out_ptr, nonterms, term_len);
			OVERRWRITE(stack, stack_ptr, buffer, buffer_len);
		}
	}

	printf("%s\n", output); // print output
}

// get rule from definition
char const* get_rule(Definition const* definition, unsigned int cost) {
	// [TODO) use a faster random number generator (SFMT?)
	size_t choice = rand() % definition->rule_count[cost]; // random value between 0 and rule count for given cost
	return definition->rules[cost][choice];
}

// get definition from grammar
Definition const* get_definition(Definition const* grammar, char key[]) {
	Definition* definition = {0};
	HASH_FIND_INT(grammar, key, definition);
	return definition;
}

// get string segment
void slice(char target[], char const source[], size_t start, size_t end) {
	// target = realloc(target, ((end - start) + 1) * sizeof(char));
	memcpy(target, source + start, end - start);
	target[end - start] = '\0';
}

// append to traced string
char* append(char* target_ptr, char const source[], size_t len) {
	memcpy(target_ptr, source, len + 1);
	return target_ptr + len;
}