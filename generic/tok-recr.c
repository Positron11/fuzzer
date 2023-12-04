#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "grammars/arithmetic.h" // change to appropriate compiled grammar

#define LOW_COST 0
#define HIGH_COST 1
#define RAND_COST rand() % 2

#define BUFFER_LEN (token_count - 1) // (jank, but I'll keep it for now)

typedef size_t depth_t;

void fuzzer(Grammar* grammar, depth_t min_depth, depth_t max_depth, depth_t depth, token_t* tokens, size_t token_count);
Rule* get_rule(Definition* definition, int cost);

int main(int argc, char *argv[]) {
	if (argc <= 1) {
		puts("Error: seed argument required");
		return EXIT_FAILURE;
	}
	
	int seed = strtod(argv[1], 0);
	srand((unsigned) seed); // initialize random

	// set options from command line if possible otherwise default
	int min_depth = argc > 2 ? strtod(argv[2], 0) : 0;
	int max_depth = argc > 2 ? strtod(argv[argc - 1], 0) : 10;
	
	fuzzer(&grammar, min_depth, max_depth, 0, (token_t[]) {start}, 1);

	return EXIT_SUCCESS;
}

// main fuzzer function
void fuzzer(Grammar* grammar, depth_t min_depth, depth_t max_depth, depth_t current_depth, token_t* tokens, size_t token_count) {
	token_t token = tokens[0]; // get first token

	// if token is terminal print to stdout
	if (token >= 0) {
		putchar(token);

	} else {
		Definition* definition = &grammar->definitions[token - start];

		int rule_cost = current_depth < min_depth ? HIGH_COST : (current_depth >= max_depth ? LOW_COST : RAND_COST); // calculate cost based on depth...
		rule_cost = definition->rule_count[1] ? rule_cost : LOW_COST; // ...and force low if definition not recursive
		
		Rule* rule = get_rule(definition, rule_cost);

		fuzzer(grammar, min_depth, max_depth, current_depth + 1, rule->tokens, rule->token_count); // fuzz the rule's expansion
	}
	
	if (BUFFER_LEN) { // if buffer exists...
		token_t buffer[BUFFER_LEN];
		memcpy(buffer, tokens + 1, BUFFER_LEN*sizeof(token_t));

		fuzzer(grammar, min_depth, max_depth, current_depth, buffer, BUFFER_LEN); // ...fuzz the buffer
	}
}

// get rule from definition
Rule* get_rule(Definition* definition, int cost) {
	return &((definition->rules)[cost])[rand() % (definition->rule_count)[cost]];
}
