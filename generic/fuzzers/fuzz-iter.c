#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "../headers/grammar.h"

typedef size_t depth_t;

void fuzzer(Grammar* grammar, depth_t max_depth);
Rule* get_rule(Definition* definition, int cost);

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
	token_t* stack = malloc(32768 * sizeof(token_t));
	stack[0] = start; // initialize stack with start token

	size_t stack_len = 1;

	// depth state variables
	depth_t stepwise_token_count[1024];
	depth_t current_depth = 0;

	// while stack not empty...
	while (stack_len > 0) {
		token_t token = stack[0]; // get first token
		if (current_depth > 0 && current_depth < max_depth) stepwise_token_count[current_depth - 1]--; // if not in cheap mode, decrement latest stepwise token count

		// if token is terminal write to stdout
		if (token >= 0) {// get first token
			memmove(stack, stack + 1, --stack_len * sizeof(token_t)); // remove token from stack
			putchar(token);

			if (current_depth > 0 && current_depth >= max_depth) stepwise_token_count[current_depth - 1]--; // if haven't already, decrement latest stepwise token count
			while (stepwise_token_count[current_depth - 1] == 0) current_depth--; // roll back to nearest incomplete node
			
			continue;
		}
		
		Definition* definition = &grammar->definitions[token - start];
		Rule* rule = get_rule(definition, current_depth >= max_depth);

		if (current_depth < max_depth) stepwise_token_count[current_depth++] = rule->token_count; // if not in cheap mode, append token count to stepwise array
		else stepwise_token_count[current_depth - 1] += (rule->token_count) - 1; // otherwise if in cheap mode, add to current token count

		// substitute token with rule in stack 
		memmove(stack + rule->token_count, stack + 1, stack_len * sizeof(token_t)); // shift stack to make space for rule
		memcpy(stack, rule->tokens, rule->token_count * sizeof(token_t)); // copy rule into created space
		stack_len += rule->token_count - 1;
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