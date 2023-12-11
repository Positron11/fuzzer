#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "grammars/arithmetic.h" // change to appropriate compiled grammar

#define STACK_LEN stack_ptr - stack // (jank, but I'll keep it for now)

typedef size_t depth_t;

void fuzzer(Grammar* grammar, depth_t max_depth);
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

	// set options from command line arguments
	int max_depth = argc > 2 ? strtod(argv[2], 0) : 10;
	
	fuzzer(&grammar, max_depth);

	return EXIT_SUCCESS;
}

// main fuzzer function
void fuzzer(Grammar* grammar, depth_t max_depth) {
	token_t* stack = malloc(2097152 * sizeof(token_t));
	token_t* stack_ptr = stack; // declare chaser pointers for efficient stack modifications
	*(stack_ptr++) = start; // initialize stack with start token

	// depth state variables
	depth_t stepwise_token_count[128] = {};
	depth_t current_depth = 0;

	// while stack not empty...
	while (STACK_LEN > 0) {
		token_t token = stack[0]; // get first token
		if (current_depth > 0 && current_depth < max_depth) stepwise_token_count[current_depth - 1]--; // if not in cheap mode, decrement latest stepwise token count
		
		// remove first token from stack
		int buffer_len = STACK_LEN - 1;
		stack_ptr = stack;
		stack_ptr = append(stack_ptr, &stack[1], buffer_len);

		// if token is terminal write to stdout
		if (token >= 0) {
			putchar(token);
			if (current_depth > 0 && current_depth >= max_depth) stepwise_token_count[current_depth - 1]--; // if haven't already, decrement latest stepwise token count
			while (stepwise_token_count[current_depth - 1] == 0) current_depth--; // roll back to nearest incomplete node
			continue;
		}
		
		Definition* definition = &grammar->definitions[token - start];
		Rule* rule = get_rule(definition, current_depth >= max_depth);

		if (current_depth < max_depth) stepwise_token_count[current_depth++] = rule->token_count; // if not in cheap mode, append token count to stepwise array
		else stepwise_token_count[current_depth - 1] += (rule->token_count) - 1; // otherwise if in cheap mode, add to current token count

		stack_ptr = prepend(stack, stack_ptr, rule->tokens, STACK_LEN, rule->token_count); // prepend rule to stack 
	}
}

// get rule from definition
Rule* get_rule(Definition* definition, int cheap) {
	size_t cheap_count = definition->rule_count[0];
	size_t exp_count = definition->rule_count[1];
	
	// force cheap if no expensive rules
	if (!exp_count) cheap = 1;

	if (cheap) {
		return &definition->rules[0][rand() % cheap_count];
	
	} else {
		// choose from cumulative rule set
		size_t choice = rand() % (cheap_count + exp_count);
		size_t cost = choice >= cheap_count;
		if (cost) choice -= cheap_count;
		return &definition->rules[cost][choice];
	}
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
