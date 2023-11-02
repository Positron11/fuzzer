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

// hashable rule structure
typedef struct Definition {
	char key[32]; // uthash hashtable key
	size_t rule_count[2]; // count of expansion options
	char** rules[2]; // expansion options ([0]: cheap, [1]: costly)
	UT_hash_handle hh;
} Definition;

enum depth_lock_states {unlocked, locking, locked}; // depth lock states

// declare functions
void fuzzer(Definition* grammar, unsigned int min_depth, unsigned int max_depth);
char* get_rule(Definition* rule, unsigned int cost);
Definition* get_definition(Definition* grammar, char* key);
char* append(char* target_ptr, char source[], size_t len);
void slice(char target[], char source[], size_t start, size_t end);
char* prepend(char target[], char* target_ptr, char source[], size_t target_len, size_t source_len);

int main(int argc, char *argv[]) {
	srand((unsigned) time(0)); // initialize random

	Definition* grammar = {0}; // declare grammar as pointers to rules

	// define grammar rules
	Definition start = { .key=START_TOKEN, .rule_count={1,0}, .rules={ 
		(char*[]) {"<phone>"},
	} };
	Definition phone = { .key="<phone>", .rule_count={2,0}, .rules={ 
		(char*[]) {"<area><number>-<number>", "<number>-<number>"},
	} };	
	Definition area = { .key="<area>", .rule_count={1,0}, .rules={ 
		(char*[]) {"(+<digit><digit>)"},
	} };
	Definition number = { .key="<number>", .rule_count={1,1}, .rules={ 
		(char*[]) {"<digit>"},
		(char*[]) {"<digit><number>"},
	} };
	Definition digit = { .key="<digit>", .rule_count={10,0}, .rules={ 
		(char*[]) {"0", "1", "2", "3", "4", "5", "6", "7", "8", "9"},
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
void fuzzer(Definition* grammar, unsigned int min_depth, unsigned int max_depth) {
	// [TODO) dynamically allocate stack and output memory
	// declare stack and output
	char* stack = malloc(2097152 * sizeof(char));
	char* output = malloc(2097152 * sizeof(char));

	// set array pointers for efficient concatenation
	char* stack_ptr = stack;
	char* out_ptr = output;

	OVERRWRITE(stack, stack_ptr, START_TOKEN, strlen(START_TOKEN)); // initialize stack

	// recursion limit variables
	unsigned int cost = 0;
	unsigned int depth = 0;
	unsigned int recursion_lock_status = 0;

	// declare token, terminals, and buffer stores
	char* tokens = malloc(2097152 * sizeof(char));

	// while stack not empty
	while (STACK_LEN > 0) {
		if (stack[0] == '<') { // if first token in stack indicates nonterminal...
			// slice token out of stack
			size_t token_len = strcspn(stack, ">") + 1;
			slice(tokens, stack, 0, token_len);
			
			size_t buffer_len = STACK_LEN - token_len; // get buffer length
			OVERRWRITE(stack, stack_ptr, &stack[token_len], buffer_len); // write buffer to stack

			if (strcmp(tokens, DEPTH_LOCK_TOKEN) == 0) { // if current token is depth lock token...
				// reset depth lock vars
				recursion_lock_status = unlocked;
				depth = 0;

			} else { // ...otherwise if current token is expandable
				Definition* definition = get_definition(grammar, tokens); // get definition

				// calculate cost based on depth and force low if definition not recursive
				unsigned int cost = depth < min_depth ? HIGH_COST : (depth >= max_depth ? LOW_COST : RAND_COST);
				cost = definition->rule_count[1] ? cost : LOW_COST;

				// increment depth if high cost (recursive) expansion
				if (cost == HIGH_COST) {
					if (recursion_lock_status == unlocked) recursion_lock_status = locking;
					depth++;
				}

				char* rule = get_rule(definition, cost); // get rule

				if (recursion_lock_status == locking) { // if in locking stage...
					stack_ptr = prepend(stack, stack_ptr, DEPTH_LOCK_TOKEN, STACK_LEN, strlen(DEPTH_LOCK_TOKEN)); // ...prepend lock token to stack
					recursion_lock_status = locked;
				}

				stack_ptr = prepend(stack, stack_ptr, rule, STACK_LEN, strlen(rule)); // prepend rule to stack
			}

		} else { // ...otherwise if first token in stack is terminal
			// slice leading nonterminal tokens out of stack
			size_t terminals_len = strcspn(stack, "<");
			slice(tokens, stack, 0, terminals_len);

			// append terminals to output and write buffer to stack
			out_ptr = append(out_ptr, tokens, terminals_len);
			size_t buffer_len = STACK_LEN - terminals_len;  // get buffer length
			OVERRWRITE(stack, stack_ptr, &stack[terminals_len], buffer_len);
		}
	}

	printf("%s\n", output); // print output
}

// get rule from definition
char* get_rule(Definition* definition, unsigned int cost) {
	// [TODO) use a faster random number generator (SFMT?)
	size_t choice = rand() % definition->rule_count[cost]; // random value between 0 and rule count for given cost
	return definition->rules[cost][choice];
}

// get definition from grammar
Definition* get_definition(Definition* grammar, char key[]) {
	Definition* definition = {0};
	HASH_FIND_INT(grammar, key, definition);
	return definition;
}

// get string segment
void slice(char target[], char source[], size_t start, size_t end) {
	// target = realloc(target, ((end - start) + 1) * sizeof(char));
	memcpy(target, source + start, (end - start) * sizeof(char));
	target[end - start] = '\0';
}

// append to traced string
char* append(char* target_ptr, char source[], size_t len) {
	memmove(target_ptr, source, (len + 1) * sizeof(char));
	return target_ptr + len;
}

// prepend to traced string
char* prepend(char target[], char* target_ptr, char source[], size_t target_len, size_t source_len) {
	memmove(&target[source_len], target, (target_len + 1) * sizeof(char));
	memcpy(target, source, source_len);
	return target + source_len + target_len;
}