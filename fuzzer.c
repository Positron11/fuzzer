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

// depth lock states
enum depth_lock_states {unlocked, locking, locked};

// string slice function-like macro
#define SLICE(target, source, start, end)					\
	char target[(end - start) + 1];							\
	memcpy(target, source + start, end - start);			\
	target[end - start] = '\0';

// append to traced string function-like macro
#define APPEND(target_ptr, source, len)						\
	memcpy(target_ptr, source, len + 1);					\
	target_ptr += len;

// overrwrite string using append function-like macro
#define OVERRWRITE(target, target_ptr, source, len)			\
	target_ptr = target;									\
	APPEND(target_ptr, source, len);

// hashable rule structure
typedef struct Rule {
	char const key[32]; // uthash hashtable key
	size_t const expansion_count[2]; // count of expansion options
	char const** expansions[2]; // expansion options ([0]: cheap, [1]: costly)
	UT_hash_handle hh;
} Rule;

// declare functions
void fuzzer(Rule const* grammar, unsigned int min_depth, unsigned int max_depth);
char const* get_expansion(Rule const* rule, unsigned int cost);
Rule const* get_rule(Rule const* grammar, char* key);

int main(int argc, char const *argv[]) {
	srand((unsigned) time(0)); // initialize random

	Rule const* grammar = {0}; // declare grammar as pointers to rules

	// define grammar rules
	Rule start = { .key=START_TOKEN, .expansion_count={1,0}, .expansions={ 
		(char const*[]) {"<phone>"},
	} };
	Rule phone = { .key="<phone>", .expansion_count={2,0}, .expansions={ 
		(char const*[]) {"<area><number>-<number>", "<number>-<number>"},
	} };	
	Rule area = { .key="<area>", .expansion_count={1,0}, .expansions={ 
		(char const*[]) {"(+<digit><digit>)"},
	} };
	Rule number = { .key="<number>", .expansion_count={1,1}, .expansions={ 
		(char const*[]) {"<digit>"},
		(char const*[]) {"<digit><number>"},
	} };
	Rule digit = { .key="<digit>", .expansion_count={10,0}, .expansions={ 
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
void fuzzer(Rule const* grammar, unsigned int min_depth, unsigned int max_depth) {
	// [TODO) dynamically allocate stack and output memory
	// declare stack and output
	char stack[2097152] = START_TOKEN;
	char output[2097152];

	// set array pointers for efficient concatenation
	char* stack_ptr = &stack[strlen(stack)];
	char* out_ptr = output;

	// set array "start" values to null
	stack[STACK_LEN] = '\0';
	output[0] = '\0';

	// recursion limit variables
	unsigned int cost = 0;
	unsigned int depth = 0;
	unsigned int depth_lock = 0;

	// while stack not empty
	while (STACK_LEN > 0) {
		if (stack[0] == '<') { // if first token in stack indicates nonterminal...
			// slice token and (remaining) buffer out of stack
			size_t token_len = strcspn(stack, ">") + 1;
			SLICE(token, stack, 0, token_len);
			SLICE(buffer, stack, token_len, STACK_LEN);

			size_t buffer_len = STACK_LEN - token_len; // save buffer length for later

			if (strcmp(token, DEPTH_LOCK_TOKEN) == 0) { // if current token is depth lock token...
				depth = 0;
				OVERRWRITE(stack, stack_ptr, buffer, buffer_len); // write buffer to stack
				depth_lock = unlocked;

			} else { // ...otherwise if current token is expandable
				Rule const* rule = get_rule(grammar, token); // get rule

				// set cost based on recursion limits
				if (depth < min_depth) cost = HIGH_COST;
				else if (depth >= max_depth) cost = LOW_COST;
				else cost = RAND_COST;

				cost = rule->expansion_count[1] ? cost : LOW_COST; // force low cost if rule not recursive

				// increment depth if high cost (recursive) expansion
				if (cost == HIGH_COST) {
					if (depth_lock == unlocked) depth_lock = locking;
					depth++;
				}

				char const* expansion = get_expansion(rule, cost);
				OVERRWRITE(stack, stack_ptr, expansion, strlen(expansion)); // write random(?) expansion to stack

				if (depth_lock == locking) { // if in locking stage...
					APPEND(stack_ptr, DEPTH_LOCK_TOKEN, 7); // ...append lock token to stack
					depth_lock = locked;
				}

				APPEND(stack_ptr, buffer, buffer_len); // append buffer to stack
			}

		} else { // ...otherwise if first token in stack is terminal
			// slice leading nonterminal tokens and (remaining) buffer out of stack
			size_t term_len = strcspn(stack, "<");
			SLICE(nonterms, stack, 0, term_len);
			SLICE(buffer, stack, term_len, STACK_LEN);

			size_t buffer_len = STACK_LEN - term_len; // save buffer length for later

			// append nonterminals to output and write buffer to stack
			APPEND(out_ptr, nonterms, term_len);
			OVERRWRITE(stack, stack_ptr, buffer, buffer_len);
		}
	}

	printf("%s\n", output); // print output
}

// get expansion from rule
char const* get_expansion(Rule const* rule, unsigned int cost) {
	// [TODO) use a faster random number generator (SFMT?)
	size_t choice = rand() % rule->expansion_count[cost]; // random value between 0 and expansions count for given cost
	return rule->expansions[cost][choice];
}

// get rule from grammar
Rule const* get_rule(Rule const* grammar, char key[]) {
	Rule* rule = {0};
	HASH_FIND_INT(grammar, key, rule);
	return rule;
}