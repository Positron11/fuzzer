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

// type definitions
typedef size_t depth_t;

// hashable rule structure
typedef struct Definition {
	char key[32]; // uthash hashtable key
	size_t rule_count[2]; // count of expansion options
	char** rules[2]; // expansion options ([0]: cheap, [1]: costly)
	UT_hash_handle hh;
} Definition;

enum depth_lock_states {unlocked, locking, locked}; // depth lock states

// declare functions
void fuzzer(Definition* grammar, depth_t min_depth, depth_t max_depth);
char* get_rule(Definition* rule, int cost);
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
	depth_t min_depth = argc > 1 ? strtod(argv[1], 0) : 2;
	depth_t max_depth = argc > 1 ? (argc > 2 ? strtod(argv[2], 0) : min_depth) : 4;
	depth_t runs = argc > 3 ? strtod(argv[3], 0) : 1;
	
	// main loop
	for (size_t i = 0; i < runs; i++) {
		fuzzer(grammar, min_depth, max_depth);
	}

	return EXIT_SUCCESS;
}

// fuzzing function
void fuzzer(Definition* grammar, depth_t min_depth, depth_t max_depth) {
	// [TODO) dynamically allocate stack and output memory
	// declare stack and output
	char* stack = malloc(2097152 * sizeof(char));
	char* output = malloc(2097152 * sizeof(char));

	// set array pointers for efficient concatenation
	char* stack_ptr = stack;
	char* out_ptr = output;

	OVERRWRITE(stack, stack_ptr, START_TOKEN, strlen(START_TOKEN)); // initialize stack

	// recursion limit variables
	int cost = 0;
	depth_t current_depth = 0;
	int recursion_lock_status = 0;

	// declare segment store
	char* segment = malloc(2097152 * sizeof(char));

	// while stack not empty
	while (STACK_LEN > 0) {
		int is_terminal = stack[0] != '<'; // determine if first symbol indicates non/terminal

		// conditionally slice segment
		size_t segment_len = strcspn(stack, is_terminal ? "<" : ">") + !is_terminal;
		slice(segment, stack, 0, segment_len);

		size_t buffer_len = STACK_LEN - segment_len; // get buffer length
		OVERRWRITE(stack, stack_ptr, &stack[segment_len], buffer_len); // overrwrite stack with buffer

		if (is_terminal) { // if segment comprised of terminals
			out_ptr = append(out_ptr, segment, segment_len); // append terminals to output
			continue;
		}

		if (strcmp(segment, DEPTH_LOCK_TOKEN) == 0) { // if segment is depthlock token
			recursion_lock_status = unlocked;
			current_depth = 0;
			continue;
		}
			
		Definition* definition = get_definition(grammar, segment);

		cost = current_depth < min_depth ? HIGH_COST : (current_depth >= max_depth ? LOW_COST : RAND_COST); // calculate cost based on depth
		cost = definition->rule_count[1] ? cost : LOW_COST; // and force low if definition not recursive

		if (cost == HIGH_COST) {
			if (recursion_lock_status == unlocked) recursion_lock_status = locking; // if in locking state promote to locked
			current_depth++;
		}

		char* rule = get_rule(definition, cost);

		if (recursion_lock_status == locking) {
			stack_ptr = prepend(stack, stack_ptr, DEPTH_LOCK_TOKEN, STACK_LEN, strlen(DEPTH_LOCK_TOKEN)); // prepend depthlock token to stack
			recursion_lock_status = locked;
		}

		stack_ptr = prepend(stack, stack_ptr, rule, STACK_LEN, strlen(rule)); // prepend rule to stack
	}

	printf("%s\n", output); // print output
}

// get rule from definition
char* get_rule(Definition* definition, int cost) {
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
	memmove(&target[source_len], target, (target_len + 1) * sizeof(char)); // move existing contents to make space for source
	memcpy(target, source, source_len); // copy source into created space
	return target + source_len + target_len;
}