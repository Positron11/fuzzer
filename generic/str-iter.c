#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "libraries/uthash.h"

#define START_TOKEN "<start>"
#define START_TOKEN_LEN 7
#define DEPTH_LOCK_TOKEN "<dlock>"
#define DEPTH_LOCK_TOKEN_LEN 7

#define LOW_COST 0
#define HIGH_COST 1
#define RAND_COST rand() % 2

#define STACK_LEN stack_ptr - stack //  (jank, but I'll keep it for now)

// append function augmenter
#define OVERRWRITE(target, target_ptr, source, len)			\
	target_ptr = target;									\
	target_ptr = append(target_ptr, source, len);

typedef size_t depth_t;

typedef struct Definition {
	char key[32];
	size_t rule_count[2];
	char** rules[2]; // [0]: cheap, [1]: costly
	UT_hash_handle hh;
} Definition;

enum depth_lock_states {unlocked, locking, locked};

void fuzzer(Definition* grammar, depth_t min_depth, depth_t max_depth);
char* get_rule(Definition* rule, int cost);
Definition* get_definition(Definition* grammar, char* key);
char* append(char* target_ptr, char source[], size_t len);
void slice(char target[], char source[], size_t start, size_t end);
char* prepend(char target[], char* target_ptr, char source[], size_t target_len, size_t source_len);

int main(int argc, char *argv[]) {
	srand((unsigned) time(0)); // initialize random

	Definition* grammar = {0};

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
	
	HASH_ADD_INT(grammar, key, &start);
	HASH_ADD_INT(grammar, key, &phone);
	HASH_ADD_INT(grammar, key, &area);
	HASH_ADD_INT(grammar, key, &number);
	HASH_ADD_INT(grammar, key, &digit);

	// set options from command line if possible otherwise default
	depth_t min_depth = argc > 1 ? strtod(argv[1], 0) : 2;
	depth_t max_depth = argc > 1 ? (argc > 2 ? strtod(argv[2], 0) : min_depth) : 4;
	depth_t runs = argc > 3 ? strtod(argv[3], 0) : 1;
	
	for (size_t i = 0; i < runs; i++) fuzzer(grammar, min_depth, max_depth);

	return EXIT_SUCCESS;
}

// main fuzzing function
void fuzzer(Definition* grammar, depth_t min_depth, depth_t max_depth) {
	char* stack = malloc(2097152 * sizeof(char));

	// set chaser pointers for efficient concatenation
	char* stack_ptr = stack;

	OVERRWRITE(stack, stack_ptr, START_TOKEN, START_TOKEN_LEN); // initialize stack with start token

	// recursion limit variables
	int recursion_lock_status = 0;
	depth_t current_depth = 0;
	int cost = 0;

	char* segment = malloc(2097152 * sizeof(char)); // declare segment store

	// while stack not empty...
	while (STACK_LEN > 0) {
		int is_terminal = stack[0] != '<'; // determine if first symbol indicates non/terminal

		// conditionally slice active segment
		size_t segment_len = strcspn(stack, is_terminal ? "<" : ">") + !is_terminal;
		slice(segment, stack, 0, segment_len);

		size_t buffer_len = STACK_LEN - segment_len; // get buffer length
		OVERRWRITE(stack, stack_ptr, &stack[segment_len], buffer_len); // overrwrite stack with buffer

		// if segment comprised of terminals append terminals to output
		if (is_terminal) {
			printf(segment);
			continue;
		}

		// if segment is depthlock token reset recursion lock vars
		if (strcmp(segment, DEPTH_LOCK_TOKEN) == 0) {
			recursion_lock_status = unlocked;
			current_depth = 0;
			continue;
		}
			
		Definition* definition = get_definition(grammar, segment);

		cost = current_depth < min_depth ? HIGH_COST : (current_depth >= max_depth ? LOW_COST : RAND_COST); // calculate cost based on depth...
		cost = definition->rule_count[1] ? cost : LOW_COST; // ...and force low if definition not recursive

		if (cost == HIGH_COST) {
			if (recursion_lock_status == unlocked) recursion_lock_status = locking;
			current_depth++;
		}

		char* rule = get_rule(definition, cost);

		if (recursion_lock_status == locking) {
			stack_ptr = prepend(stack, stack_ptr, DEPTH_LOCK_TOKEN, STACK_LEN, DEPTH_LOCK_TOKEN_LEN); // prepend depthlock token to stack
			recursion_lock_status = locked;
		}

		stack_ptr = prepend(stack, stack_ptr, rule, STACK_LEN, strlen(rule)); // prepend rule to stack
	}
}

// get rule from definition
char* get_rule(Definition* definition, int cost) {
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