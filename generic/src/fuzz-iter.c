#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fuzzer.h"
#include "gstruct.h"
#include "fuzzutils.h"

void fuzzer(Grammar* grammar, depth_t max_depth);

// wrapper function
void fuzz(Grammar* grammar, depth_t max_depth) {
	fuzzer(grammar, max_depth);
}

// main fuzzer function
void fuzzer(Grammar* grammar, depth_t max_depth) {
	token_t* stack = malloc(32768 * sizeof(token_t));
	stack[0] = START; // initialize stack with start token

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
		
		Definition* definition = &grammar->definitions[token - START];
		Rule* rule = get_rule(definition, current_depth >= max_depth);

		if (current_depth < max_depth) stepwise_token_count[current_depth++] = rule->token_count; // if not in cheap mode, append token count to stepwise array
		else stepwise_token_count[current_depth - 1] += (rule->token_count) - 1; // otherwise if in cheap mode, add to current token count

		// substitute token with rule in stack 
		memmove(stack + rule->token_count, stack + 1, stack_len * sizeof(token_t)); // shift stack to make space for rule
		memcpy(stack, rule->tokens, rule->token_count * sizeof(token_t)); // copy rule into created space
		stack_len += rule->token_count - 1;
	}
}
